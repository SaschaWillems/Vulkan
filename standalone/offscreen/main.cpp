// The portability-subset name is still in Vulkan's provisional header.
#ifndef VK_ENABLE_BETA_EXTENSIONS
#define VK_ENABLE_BETA_EXTENSIONS
#endif
// Vulkan 1.1 / C++20 offscreen renderer and cross-platform WebRTC server.
// See README.md for SDK setup, build, browser connection, and tests.
#include "PbrRenderer.h"
#include "VideoConfig.h"
#ifdef OFFSCREEN_STREAMING
#include "EncoderFactory.h"
#include "WebRTCStreamer.h"
#include <rtc/global.hpp>
#endif

#include <algorithm>
#include <charconv>
#include <chrono>
#include <csignal>
#include <memory>
#include <span>
#include <string_view>
#include <thread>
#include <type_traits>

#include <vulkan/vulkan.h>
#ifdef OFFSCREEN_HAS_NVENC
#include "VulkanMemoryExporter.h"
#endif

#include <array>
#include <atomic>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void vkCheck(VkResult result, const char* expression) {
    if (result != VK_SUCCESS) {
        throw std::runtime_error(std::string(expression) + " failed (VkResult " +
                                 std::to_string(result) + ")");
    }
}
#define VK_CHECK(expression) vkCheck((expression), #expression)

// Enumeration counts can change between calls; retry rather than accepting truncation.
template <typename T, typename Query>
std::vector<T> enumerate(Query query) {
    for (;;) {
        uint32_t count = 0;
        VK_CHECK(query(&count, static_cast<T*>(nullptr)));
        if (count == 0) {
            return {};
        }
        std::vector<T> items(count);
        const VkResult result = query(&count, items.data());
        if (result == VK_INCOMPLETE) {
            continue;
        }
        VK_CHECK(result);
        items.resize(count);
        return items;
    }
}

bool hasExtension(const std::vector<VkExtensionProperties>& extensions, const char* name) {
    for (const auto& extension : extensions) {
        if (std::strcmp(extension.extensionName, name) == 0) {
            return true;
        }
    }
    return false;
}

template <typename T>
T structure(VkStructureType type) {
    T value{};
    value.sType = type;
    return value;
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT* data, void* userData) {
    const bool error = (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) != 0;
    if (error) {
        static_cast<std::atomic_uint32_t*>(userData)->fetch_add(1, std::memory_order_relaxed);
    }
    // C stdio keeps this C callback free of C++ stream exceptions.
    std::fprintf(stderr, "[validation %s] %s\n", error ? "error" : "warning", data->pMessage);
    return VK_FALSE;
}

class OffscreenRenderer {
public:
    const uint32_t width, height;
    explicit OffscreenRenderer(VideoResolution resolution)
        : width(resolution.width), height(resolution.height), pixelBytes(VkDeviceSize{width} * height * 4) {}
    OffscreenRenderer(const OffscreenRenderer&) = delete;
    OffscreenRenderer& operator=(const OffscreenRenderer&) = delete;

    // Step 9: Reverse dependency order, also safe after any partial initialization.
    // Initialization happens in Initialize(), so this destructor runs if any step throws.
    ~OffscreenRenderer() {
        if (device_ != VK_NULL_HANDLE) {
            // Also covers exceptions after submission but before a successful fence wait.
            (void)vkDeviceWaitIdle(device_);
            if (mapped_ != nullptr) vkUnmapMemory(device_, stagingMemory_);
            if (fence_) vkDestroyFence(device_, fence_, nullptr);
            if (commandPool_) vkDestroyCommandPool(device_, commandPool_, nullptr);
            // Destroying the pool implicitly frees its command buffers.
            if (stagingBuffer_) vkDestroyBuffer(device_, stagingBuffer_, nullptr);
            if (stagingMemory_) vkFreeMemory(device_, stagingMemory_, nullptr);
            if (framebuffer_) vkDestroyFramebuffer(device_, framebuffer_, nullptr);
            scene_.reset();
            if (renderPass_) vkDestroyRenderPass(device_, renderPass_, nullptr);
            if (imageView_) vkDestroyImageView(device_, imageView_, nullptr);
            // Bound resources must be destroyed before their allocations are freed.
            if (image_ && !sharedImage_) vkDestroyImage(device_, image_, nullptr);
            if (imageMemory_) vkFreeMemory(device_, imageMemory_, nullptr);
#ifdef OFFSCREEN_HAS_NVENC
            sharedFrame_.reset();
#endif
            vkDestroyDevice(device_, nullptr);
        }
        if (debugMessenger_ && destroyDebugMessenger_) {
            destroyDebugMessenger_(instance_, debugMessenger_, nullptr);
        }
        if (instance_) vkDestroyInstance(instance_, nullptr);
    }

    void Initialize(bool allowGpuCapture = false, bool preferGpuBuffer = false,
                    const std::filesystem::path& model = {}, bool spin = false) {
        requestGpuCapture_ = allowGpuCapture;
        preferGpuBuffer_ = preferGpuBuffer;
        createInstance();
        createDevice();
        spin_=spin;
        scene_=std::make_unique<PbrRenderer>();
        scene_->Initialize(instance_,physicalDevice_,device_,queue_,queueFamily_,width,height,model);
        createTarget();
        createRenderPassAndFramebuffer();
        scene_->CreatePipeline(renderPass_);
    }

    void ConfigureCapture(bool gpu) {
        gpuCapture_ = gpu;
#ifdef OFFSCREEN_HAS_NVENC
        if (gpu && !sharedFrame_) throw std::logic_error("No Vulkan export resource for GPU capture");
#else
        if (gpu) throw std::logic_error("GPU capture was not compiled");
#endif
        if (!gpu) createReadbackBuffer();
        recordCommands();
    }

#ifdef OFFSCREEN_STREAMING
    VulkanMemoryExporter* Exporter() const {
#ifdef OFFSCREEN_HAS_NVENC
        return sharedFrame_.get();
#else
        return nullptr;
#endif
    }
#endif

    std::span<const uint8_t> RenderFrame(float seconds, CameraPose camera) {
        // The previous frame's fence completed before rewriting uniforms or command buffers.
        scene_->PrepareFrame(camera,seconds,spin_);
        const VkCommandBuffer commands = firstFrame_ ? firstCommands_ : commands_;
        VK_CHECK(vkResetCommandBuffer(commands,0));
        recordFrame(commands,firstFrame_);
        VK_CHECK(vkResetFences(device_, 1, &fence_));
        auto submit = structure<VkSubmitInfo>(VK_STRUCTURE_TYPE_SUBMIT_INFO);
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &commands;
#ifdef OFFSCREEN_HAS_NVENC
        const VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        const VkSemaphore ready = sharedFrame_ ? sharedFrame_->Ready() : VK_NULL_HANDLE;
        const VkSemaphore done = sharedFrame_ ? sharedFrame_->Done() : VK_NULL_HANDLE;
        if (gpuCapture_) {
            if (!firstFrame_) {
                submit.waitSemaphoreCount = 1;
                submit.pWaitSemaphores = &done;
                submit.pWaitDstStageMask = &waitStage;
            }
            submit.signalSemaphoreCount = 1;
            submit.pSignalSemaphores = &ready;
        }
#endif
        VK_CHECK(vkQueueSubmit(queue_, 1, &submit, fence_));
        VK_CHECK(vkWaitForFences(device_, 1, &fence_, VK_TRUE, std::numeric_limits<uint64_t>::max()));
        if (!gpuCapture_ && !stagingCoherent_) {
            auto range = structure<VkMappedMemoryRange>(VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE);
            range.memory = stagingMemory_;
            // Whole-allocation mapping/invalidation satisfies nonCoherentAtomSize alignment.
            range.size = VK_WHOLE_SIZE;
            VK_CHECK(vkInvalidateMappedMemoryRanges(device_, 1, &range));
        }
        if (validationErrors_.load(std::memory_order_relaxed) != 0)
            throw std::runtime_error("Vulkan validation reported errors");
        firstFrame_ = false;
        if (gpuCapture_) return {};
        return {static_cast<const uint8_t*>(mapped_), static_cast<size_t>(pixelBytes)};
    }

    void SavePpm() const {
        std::ofstream output("output.ppm", std::ios::binary | std::ios::trunc);
        if (!output) throw std::runtime_error("Cannot open output.ppm for writing");
        output << "P6\n" << width << ' ' << height << "\n255\n";
        const auto* pixels = static_cast<const uint8_t*>(mapped_);
        std::vector<char> row(static_cast<size_t>(width) * 3);
        for (uint32_t y = 0; y < height; ++y) {
            for (uint32_t x = 0; x < width; ++x) {
                const auto offset = (static_cast<size_t>(y) * width + x) * 4;
                std::memcpy(row.data() + x * 3, pixels + offset, 3);
            }
            output.write(row.data(), static_cast<std::streamsize>(row.size()));
        }
        output.close();
        if (!output) throw std::runtime_error("Failed to write output.ppm");
        std::cout << "Wrote output.ppm (" << width << 'x' << height << ")\n";
    }

private:
    static constexpr VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
    const VkDeviceSize pixelBytes;

    std::atomic_uint32_t validationErrors_{0};
    VkInstance instance_{};
    VkDebugUtilsMessengerEXT debugMessenger_{};
    PFN_vkDestroyDebugUtilsMessengerEXT destroyDebugMessenger_{};
    VkPhysicalDevice physicalDevice_{};
    VkPhysicalDeviceMemoryProperties memoryProperties_{};
    uint32_t queueFamily_{};
    VkDevice device_{};
    VkQueue queue_{};
    std::unique_ptr<PbrRenderer> scene_;
    bool spin_ = false;
    bool requestGpuCapture_ = false, preferGpuBuffer_ = false;
    bool sharedImage_ = false, gpuCapture_ = false, firstFrame_ = true;
#ifdef OFFSCREEN_HAS_NVENC
    bool exportSupported_ = false;
    std::unique_ptr<VulkanMemoryExporter> sharedFrame_;
#endif
    VkImage image_{};
    VkDeviceMemory imageMemory_{};
    VkImageView imageView_{};
    VkRenderPass renderPass_{};
    VkFramebuffer framebuffer_{};
    VkBuffer stagingBuffer_{};
    VkDeviceMemory stagingMemory_{};
    bool stagingCoherent_{};
    VkCommandPool commandPool_{};
    VkCommandBuffer commands_{}, firstCommands_{};
    VkFence fence_{};
    void* mapped_{};

    // Step 1: Portability enumeration exposes MoltenVK devices without creating a surface.
    void createInstance() {
        const auto available = enumerate<VkExtensionProperties>([](auto* count, auto* values) {
            return vkEnumerateInstanceExtensionProperties(nullptr, count, values);
        });
        std::vector<const char*> extensions;
        auto info = structure<VkInstanceCreateInfo>(VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO);
        // Older Linux distro headers predate portability enumeration; native drivers do not need it.
#ifdef VK_KHR_portability_enumeration
        if (hasExtension(available, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME)) {
            extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
            info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
        }
#ifdef __APPLE__
        else {
            throw std::runtime_error("Vulkan loader lacks VK_KHR_portability_enumeration");
        }
#endif
#elif defined(__APPLE__)
        throw std::runtime_error("Vulkan headers lack VK_KHR_portability_enumeration; install a recent Vulkan SDK");
#endif
        auto application = structure<VkApplicationInfo>(VK_STRUCTURE_TYPE_APPLICATION_INFO);
        application.pApplicationName = "Offscreen glTF viewer";
        // Vulkan 1.1 also satisfies portability_subset's properties2 dependency.
        application.apiVersion = VK_API_VERSION_1_1;
        info.pApplicationInfo = &application;

        auto debug = structure<VkDebugUtilsMessengerCreateInfoEXT>(
            VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT);
        debug.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debug.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debug.pfnUserCallback = debugCallback;
        debug.pUserData = &validationErrors_;
#ifndef NDEBUG
        const auto layers = enumerate<VkLayerProperties>([](auto* count, auto* values) {
            return vkEnumerateInstanceLayerProperties(count, values);
        });
        constexpr const char* validationLayer = "VK_LAYER_KHRONOS_validation";
        bool found = false;
        for (const auto& layer : layers) {
            found |= std::strcmp(layer.layerName, validationLayer) == 0;
        }
        if (!found || !hasExtension(available, VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
            throw std::runtime_error("Debug builds require the Vulkan SDK validation layer "
                                     "and VK_EXT_debug_utils; install them or build Release");
        }
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        info.enabledLayerCount = 1;
        info.ppEnabledLayerNames = &validationLayer;
        info.pNext = &debug; // Include instance creation/destruction in validation output.
#endif
        info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        info.ppEnabledExtensionNames = extensions.data();
        VK_CHECK(vkCreateInstance(&info, nullptr, &instance_));
#ifndef NDEBUG
        const auto createDebugMessenger = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(instance_, "vkCreateDebugUtilsMessengerEXT"));
        destroyDebugMessenger_ = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(instance_, "vkDestroyDebugUtilsMessengerEXT"));
        if (!createDebugMessenger || !destroyDebugMessenger_) {
            throw std::runtime_error("Cannot load VK_EXT_debug_utils entry points");
        }
        VK_CHECK(createDebugMessenger(instance_, &debug, nullptr, &debugMessenger_));
#endif
    }

    // Step 2: A graphics queue suffices for rendering and transfer; no present queue is needed.
    void createDevice() {
        auto devices = enumerate<VkPhysicalDevice>([this](auto* count, auto* values) {
            return vkEnumeratePhysicalDevices(instance_, count, values);
        });
#ifdef OFFSCREEN_HAS_NVENC
        if (requestGpuCapture_) {
            const auto isNvidia = [](VkPhysicalDevice device) {
                VkPhysicalDeviceProperties properties{};
                vkGetPhysicalDeviceProperties(device, &properties);
                return properties.vendorID == 0x10de;
            };
            std::stable_sort(devices.begin(), devices.end(), [&](auto a, auto b) {
                return isNvidia(a) > isNvidia(b);
            });
        }
#endif
        for (VkPhysicalDevice candidate : devices) {
            VkPhysicalDeviceProperties properties{};
            vkGetPhysicalDeviceProperties(candidate, &properties);
            if (properties.apiVersion < VK_API_VERSION_1_1) continue;
            if (width > properties.limits.maxFramebufferWidth || height > properties.limits.maxFramebufferHeight ||
                width > properties.limits.maxViewportDimensions[0] || height > properties.limits.maxViewportDimensions[1])
                continue;
            VkFormatProperties formatProperties{};
            vkGetPhysicalDeviceFormatProperties(candidate, format, &formatProperties);
            constexpr VkFormatFeatureFlags required = VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT |
                                                       VK_FORMAT_FEATURE_TRANSFER_SRC_BIT;
            if ((formatProperties.optimalTilingFeatures & required) != required) continue;
            VkImageFormatProperties imageProperties{};
            const VkResult supported = vkGetPhysicalDeviceImageFormatProperties(
                candidate, format, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                0, &imageProperties);
            if (supported == VK_ERROR_FORMAT_NOT_SUPPORTED) continue;
            VK_CHECK(supported);
            if (imageProperties.maxExtent.width < width ||
                imageProperties.maxExtent.height < height ||
                !(imageProperties.sampleCounts & VK_SAMPLE_COUNT_1_BIT)) continue;

            uint32_t count = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(candidate, &count, nullptr);
            std::vector<VkQueueFamilyProperties> families(count);
            vkGetPhysicalDeviceQueueFamilyProperties(candidate, &count, families.data());
            for (uint32_t i = 0; i < count; ++i) {
                if (families[i].queueCount > 0 && (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
                    physicalDevice_ = candidate;
                    queueFamily_ = i;
                    std::cout << "Device: " << properties.deviceName << '\n';
                    break;
                }
            }
            if (physicalDevice_) break;
        }
        if (!physicalDevice_) {
            throw std::runtime_error("No Vulkan 1.1 graphics device supports the " + std::to_string(width) +
                                     "x" + std::to_string(height) + " offscreen target");
        }
        const auto available = enumerate<VkExtensionProperties>([this](auto* count, auto* values) {
            return vkEnumerateDeviceExtensionProperties(physicalDevice_, nullptr, count, values);
        });
        std::vector<const char*> extensions;
        // Required whenever advertised, including portability drivers on platforms other than macOS.
        if (hasExtension(available, VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME)) {
            extensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
        }
#ifdef OFFSCREEN_HAS_NVENC
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(physicalDevice_, &properties);
        exportSupported_ = requestGpuCapture_ && properties.vendorID == 0x10de &&
            hasExtension(available, VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME) &&
            hasExtension(available, VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME);
        if (exportSupported_) {
            extensions.push_back(VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME);
            extensions.push_back(VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME);
        }
#endif
        const float priority = 1.0f;
        auto queueInfo = structure<VkDeviceQueueCreateInfo>(VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO);
        queueInfo.queueFamilyIndex = queueFamily_;
        queueInfo.queueCount = 1;
        queueInfo.pQueuePriorities = &priority;
        auto info = structure<VkDeviceCreateInfo>(VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO);
        info.queueCreateInfoCount = 1;
        info.pQueueCreateInfos = &queueInfo;
        info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        info.ppEnabledExtensionNames = extensions.data();
        VK_CHECK(vkCreateDevice(physicalDevice_, &info, nullptr, &device_));
        vkGetDeviceQueue(device_, queueFamily_, 0, &queue_);
        vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &memoryProperties_);
    }

    uint32_t memoryType(uint32_t bits, VkMemoryPropertyFlags required,
                        VkMemoryPropertyFlags preferred = 0) const {
        uint32_t fallback = std::numeric_limits<uint32_t>::max();
        for (uint32_t i = 0; i < memoryProperties_.memoryTypeCount; ++i) {
            const auto flags = memoryProperties_.memoryTypes[i].propertyFlags;
            if ((bits & (uint32_t{1} << i)) && (flags & required) == required) {
                if ((flags & preferred) == preferred) return i;
                if (fallback == std::numeric_limits<uint32_t>::max()) fallback = i;
            }
        }
        if (fallback != std::numeric_limits<uint32_t>::max()) return fallback;
        throw std::runtime_error("No compatible Vulkan memory type");
    }

    // Step 3: Prefer a directly exported optimal-tiled image for CUDA/NVENC.
    void createTarget() {
#ifdef OFFSCREEN_HAS_NVENC
        if (exportSupported_) {
            if (!preferGpuBuffer_) {
                try { sharedFrame_ = VulkanMemoryExporter::Create(physicalDevice_, device_, width, height,
                                                                 VulkanMemoryExporter::Kind::Image); }
                catch (const std::exception& error) { std::cerr << "Image export: " << error.what() << '\n'; }
            }
            if (!sharedFrame_) {
                try { sharedFrame_ = VulkanMemoryExporter::Create(physicalDevice_, device_, width, height,
                                                                 VulkanMemoryExporter::Kind::Buffer); }
                catch (const std::exception& error) { std::cerr << "Buffer export: " << error.what() << '\n'; }
            }
            if (sharedFrame_ && sharedFrame_->Type() == VulkanMemoryExporter::Kind::Image) {
                image_ = sharedFrame_->Image();
                sharedImage_ = true;
            }
        }
#endif
        if (!sharedImage_) {
            auto info = structure<VkImageCreateInfo>(VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO);
            info.imageType = VK_IMAGE_TYPE_2D;
            info.format = format;
            info.extent = {width, height, 1};
            info.mipLevels = 1;
            info.arrayLayers = 1;
            info.samples = VK_SAMPLE_COUNT_1_BIT;
            info.tiling = VK_IMAGE_TILING_OPTIMAL;
            info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
            info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            VK_CHECK(vkCreateImage(device_, &info, nullptr, &image_));
            VkMemoryRequirements requirements{};
            vkGetImageMemoryRequirements(device_, image_, &requirements);
            auto allocation = structure<VkMemoryAllocateInfo>(VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO);
            allocation.allocationSize = requirements.size;
            allocation.memoryTypeIndex = memoryType(requirements.memoryTypeBits,
                                                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            VK_CHECK(vkAllocateMemory(device_, &allocation, nullptr, &imageMemory_));
            VK_CHECK(vkBindImageMemory(device_, image_, imageMemory_, 0));
        }
        auto view = structure<VkImageViewCreateInfo>(VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO);
        view.image = image_;
        view.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view.format = format;
        view.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        VK_CHECK(vkCreateImageView(device_, &view, nullptr, &imageView_));
    }

    void createRenderPassAndFramebuffer() {
        // Step 4: Explicit barriers acquire ownership and transition before this pass.
        VkAttachmentDescription attachment{};
        attachment.format = format;
        attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        const VkAttachmentReference color{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &color;
        const VkAttachmentReference depthReference{1,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
        subpass.pDepthStencilAttachment=&depthReference;
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        // Synchronize the preceding frame's copy before discarding/reusing the image.
        dependency.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        auto info = structure<VkRenderPassCreateInfo>(VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO);
        VkAttachmentDescription depth=attachment;
        depth.format=scene_->DepthFormat(); depth.storeOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED; depth.finalLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        const VkAttachmentDescription attachments[]={attachment,depth};
        info.attachmentCount = 2;
        info.pAttachments = attachments;
        info.subpassCount = 1;
        info.pSubpasses = &subpass;
        info.dependencyCount = 1;
        info.pDependencies = &dependency;
        VK_CHECK(vkCreateRenderPass(device_, &info, nullptr, &renderPass_));

        // Step 5: The framebuffer binds the image view to this single render-pass attachment.
        auto framebuffer = structure<VkFramebufferCreateInfo>(VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO);
        framebuffer.renderPass = renderPass_;
        const VkImageView views[]={imageView_,scene_->DepthView()};
        framebuffer.attachmentCount = 2;
        framebuffer.pAttachments = views;
        framebuffer.width = width;
        framebuffer.height = height;
        framebuffer.layers = 1;
        VK_CHECK(vkCreateFramebuffer(device_, &framebuffer, nullptr, &framebuffer_));
    }

    void createReadbackBuffer() {
        auto info = structure<VkBufferCreateInfo>(VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO);
        info.size = pixelBytes;
        info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VK_CHECK(vkCreateBuffer(device_, &info, nullptr, &stagingBuffer_));
        VkMemoryRequirements requirements{};
        vkGetBufferMemoryRequirements(device_, stagingBuffer_, &requirements);
        const uint32_t type = memoryType(requirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        stagingCoherent_ = (memoryProperties_.memoryTypes[type].propertyFlags &
                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0;
        auto allocation = structure<VkMemoryAllocateInfo>(VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO);
        allocation.allocationSize = requirements.size;
        allocation.memoryTypeIndex = type;
        VK_CHECK(vkAllocateMemory(device_, &allocation, nullptr, &stagingMemory_));
        VK_CHECK(vkBindBufferMemory(device_, stagingBuffer_, stagingMemory_, 0));
        VK_CHECK(vkMapMemory(device_, stagingMemory_, 0, VK_WHOLE_SIZE, 0, &mapped_));
    }

    void recordCommands() {
        auto pool = structure<VkCommandPoolCreateInfo>(VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO);
        pool.queueFamilyIndex = queueFamily_;
        pool.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        VK_CHECK(vkCreateCommandPool(device_, &pool, nullptr, &commandPool_));
        auto allocation = structure<VkCommandBufferAllocateInfo>(VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO);
        allocation.commandPool = commandPool_;
        allocation.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocation.commandBufferCount = 2;
        VkCommandBuffer buffers[2]{};
        VK_CHECK(vkAllocateCommandBuffers(device_, &allocation, buffers));
        firstCommands_ = buffers[0]; commands_ = buffers[1];
        auto fence = structure<VkFenceCreateInfo>(VK_STRUCTURE_TYPE_FENCE_CREATE_INFO);
        VK_CHECK(vkCreateFence(device_, &fence, nullptr, &fence_));
    }

    void recordFrame(VkCommandBuffer commands, bool first) {
        auto begin = structure<VkCommandBufferBeginInfo>(VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO);
        VK_CHECK(vkBeginCommandBuffer(commands, &begin));
        bool directImage = false;
        VkBuffer destination = stagingBuffer_;
#ifdef OFFSCREEN_HAS_NVENC
        directImage = gpuCapture_ && sharedImage_;
        if (gpuCapture_ && !directImage) destination = sharedFrame_->Buffer();
        if (gpuCapture_ && !directImage && !first) {
            auto acquire = structure<VkBufferMemoryBarrier>(VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER);
            acquire.srcQueueFamilyIndex = VK_QUEUE_FAMILY_EXTERNAL;
            acquire.dstQueueFamilyIndex = queueFamily_;
            acquire.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            acquire.buffer = destination;
            acquire.size = VK_WHOLE_SIZE;
            vkCmdPipelineBarrier(commands, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 0, 0, nullptr, 1, &acquire, 0, nullptr);
        }
#endif
        auto acquireImage = structure<VkImageMemoryBarrier>(VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER);
        acquireImage.image = image_;
        acquireImage.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        acquireImage.oldLayout = directImage && !first ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_UNDEFINED;
        acquireImage.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        acquireImage.srcQueueFamilyIndex = directImage && !first ? VK_QUEUE_FAMILY_EXTERNAL : VK_QUEUE_FAMILY_IGNORED;
        acquireImage.dstQueueFamilyIndex = directImage && !first ? queueFamily_ : VK_QUEUE_FAMILY_IGNORED;
        acquireImage.srcAccessMask = directImage ? 0 : VK_ACCESS_TRANSFER_READ_BIT;
        acquireImage.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        vkCmdPipelineBarrier(commands, directImage ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT : VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &acquireImage);
        VkClearValue clear[2]{};
        clear[0].color={{0.04f,0.06f,0.10f,1}}; clear[1].depthStencil={1,0};
        auto render = structure<VkRenderPassBeginInfo>(VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO);
        render.renderPass = renderPass_;
        render.framebuffer = framebuffer_;
        render.renderArea = {{0, 0}, {width, height}};
        render.clearValueCount = 2;
        render.pClearValues = clear;
        vkCmdBeginRenderPass(commands, &render, VK_SUBPASS_CONTENTS_INLINE);
        scene_->Draw(commands);
        vkCmdEndRenderPass(commands);

#ifdef OFFSCREEN_HAS_NVENC
        if (directImage) {
            auto release = structure<VkImageMemoryBarrier>(VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER);
            release.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            release.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            release.newLayout = VK_IMAGE_LAYOUT_GENERAL;
            release.srcQueueFamilyIndex = queueFamily_;
            release.dstQueueFamilyIndex = VK_QUEUE_FAMILY_EXTERNAL;
            release.image = image_;
            release.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
            vkCmdPipelineBarrier(commands, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &release);
            VK_CHECK(vkEndCommandBuffer(commands));
            return;
        }
#endif
        auto imageBarrier = structure<VkImageMemoryBarrier>(VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER);
        imageBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        imageBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        imageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageBarrier.image = image_;
        imageBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        vkCmdPipelineBarrier(commands, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier);

        VkBufferImageCopy copy{};
        // Zero row length/image height means tightly packed width*4-byte rows.
        // bufferOffset=0 is aligned; the optimal image's opaque row pitch is never mapped.
        copy.bufferRowLength = 0;
        copy.bufferImageHeight = 0;
        copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        copy.imageExtent = {width, height, 1};
        vkCmdCopyImageToBuffer(commands, image_, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               destination, 1, &copy);

#ifdef OFFSCREEN_HAS_NVENC
        if (gpuCapture_) {
            auto release = structure<VkBufferMemoryBarrier>(VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER);
            release.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            release.srcQueueFamilyIndex = queueFamily_;
            release.dstQueueFamilyIndex = VK_QUEUE_FAMILY_EXTERNAL;
            release.buffer = destination;
            release.size = VK_WHOLE_SIZE;
            vkCmdPipelineBarrier(commands, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                  0, 0, nullptr, 1, &release, 0, nullptr);
        } else
#endif
        {
            auto hostBarrier = structure<VkBufferMemoryBarrier>(VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER);
            hostBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            hostBarrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
            hostBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            hostBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            hostBarrier.buffer = stagingBuffer_;
            hostBarrier.size = VK_WHOLE_SIZE;
            vkCmdPipelineBarrier(commands, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT,
                                  0, 0, nullptr, 1, &hostBarrier, 0, nullptr);
        }
        VK_CHECK(vkEndCommandBuffer(commands));
    }

};

#ifdef OFFSCREEN_STREAMING
volatile std::sig_atomic_t stopRequested = 0;
void HandleSignal(int) { stopRequested = 1; }
#endif

uint32_t ParseUnsigned(std::string_view value, uint32_t maximum) {
    uint32_t parsed = 0;
    const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
    if (result.ec != std::errc{} || result.ptr != value.data() + value.size() || parsed > maximum)
        throw std::invalid_argument("Invalid numeric argument: " + std::string(value));
    return parsed;
}

void TestRendering(OffscreenRenderer& renderer) {
    const auto pixels = renderer.RenderFrame(0, {});
    const std::vector<uint8_t> baseline(pixels.begin(), pixels.end());
    const auto changed = [&baseline](std::span<const uint8_t> frame) {
        size_t differences = 0;
        for (size_t i = 0; i < frame.size(); ++i) differences += frame[i] != baseline[i];
        return differences;
    };
    if (changed(renderer.RenderFrame(1.0f, {})) < 2000)
        throw std::runtime_error("Animation did not change the rendered frame");
    if (changed(renderer.RenderFrame(0, {0.8f, 0.4f})) < 2000)
        throw std::runtime_error("Camera orbit did not change the rendered frame");
    if (changed(renderer.RenderFrame(0, {0, 0, 2})) < 2000)
        throw std::runtime_error("Camera zoom did not change the rendered frame");
    CameraPose panned;
    panned.target={0.3f,0.15f,0};
    if (changed(renderer.RenderFrame(0,panned))<2000)
        throw std::runtime_error("Camera pan did not change the rendered frame");
    if (changed(renderer.RenderFrame(0, {})) != 0)
        throw std::runtime_error("Uniform/framebuffer reuse was not deterministic");
    std::cout << "Animation, orbit, pan, zoom, and framebuffer reuse passed\n";
}

} // namespace

int main(int argc, char** argv) {
    try {
        bool snapshot = false, renderTest = false, spin = false;
        std::filesystem::path model;
        uint32_t frameLimit = 0;
        auto resolution = ResolutionPreset("2k");
        uint32_t fps = 60, bitrate = 0;
        std::string_view codecOption = "auto";
        bool gpuBuffer = false;
#ifdef OFFSCREEN_STREAMING
        WebRTCStreamer::Options options;
        EncoderPreference encoderPreference = EncoderPreference::Automatic;
#endif
        for (int i = 1; i < argc; ++i) {
            const std::string_view arg(argv[i]);
            const auto next = [&]() -> std::string_view {
                if (++i >= argc) throw std::invalid_argument("Missing value for " + std::string(arg));
                return argv[i];
            };
            if (arg == "--snapshot") snapshot = true;
            else if (arg == "--model") model=next();
            else if (arg == "--spin") spin=true;
            else if (arg == "--render-test") renderTest = true;
            else if (arg == "--help") {
                std::cout << "Usage: offscreen_renderer [--snapshot | --render-test]\n"
                             "       offscreen_renderer [--frames N] [--port 8080] [--bind 127.0.0.1]\n"
                             "                          [--ice-server stun:host:port]\n"
                             "                          [--encoder auto|hardware|software] [--nvenc-input image|buffer]\n"
                             "                          [--resolution 2k|4k|8k] [--fps 1..60] [--bitrate BPS]\n"
                             "                          [--codec auto|h264|av1] [--model path.gltf|path.glb] [--spin]\n"
                             "Default: 2k (2560x1440), 60 FPS, 16 Mbps; 8k auto-selects AV1.\n";
                return 0;
            } else if (arg == "--frames") {
                frameLimit = ParseUnsigned(next(), 100000000);
                if (!frameLimit) throw std::invalid_argument("--frames must be positive");
            } else if (arg == "--resolution") resolution = ResolutionPreset(next());
            else if (arg == "--fps") {
                fps = ParseUnsigned(next(), 60);
                if (!fps) throw std::invalid_argument("--fps must be between 1 and 60");
            } else if (arg == "--bitrate") {
                bitrate = ParseUnsigned(next(), 100000000);
                if (bitrate < 1000) throw std::invalid_argument("--bitrate must be between 1000 and 100000000 bps");
            } else if (arg == "--codec") {
                codecOption = next();
                if (codecOption != "auto" && codecOption != "h264" && codecOption != "av1")
                    throw std::invalid_argument("--codec expects auto, h264, or av1");
            }
#ifdef OFFSCREEN_STREAMING
            else if (arg == "--port") options.port = static_cast<uint16_t>(ParseUnsigned(next(), 65535));
            else if (arg == "--bind") options.bindAddress = next();
            else if (arg == "--ice-server") options.iceServers.emplace_back(next());
            else if (arg == "--encoder") {
                const auto value = next();
                if (value == "auto") encoderPreference = EncoderPreference::Automatic;
                else if (value == "hardware") encoderPreference = EncoderPreference::Hardware;
                else if (value == "software") encoderPreference = EncoderPreference::Software;
                else throw std::invalid_argument("--encoder expects auto, hardware, or software");
            }
#ifdef OFFSCREEN_HAS_NVENC
            else if (arg == "--nvenc-input") {
                const auto value = next();
                if (value != "image" && value != "buffer") throw std::invalid_argument("--nvenc-input expects image or buffer");
                gpuBuffer = value == "buffer";
            }
#endif
#endif
            else throw std::invalid_argument("Unknown option: " + std::string(arg));
        }
#ifndef OFFSCREEN_STREAMING
        if (frameLimit) throw std::invalid_argument("Streaming is disabled in this build");
        snapshot = !renderTest;
#endif
        if ((snapshot && renderTest) || ((snapshot || renderTest) && frameLimit))
            throw std::invalid_argument("Snapshot, render test, and frame limit are separate modes");
        OffscreenRenderer renderer(resolution);
        bool allowGpuCapture = false;
#ifdef OFFSCREEN_HAS_NVENC
        allowGpuCapture = !snapshot && !renderTest && encoderPreference != EncoderPreference::Software;
#endif
        renderer.Initialize(allowGpuCapture, gpuBuffer,model,spin || renderTest);
        if (snapshot || renderTest) renderer.ConfigureCapture(false);
        if (renderTest) { TestRendering(renderer); return 0; }
        if (snapshot) { renderer.RenderFrame(0, {}); renderer.SavePpm(); return 0; }
#ifdef OFFSCREEN_STREAMING
        const auto codec = codecOption == "av1" || (codecOption == "auto" && resolution.width > 4096)
            ? VideoCodec::AV1 : VideoCodec::H264;
        if (!bitrate) bitrate = resolution.bitrate;
        options.codec = codec;
        options.h264Level = H264Level(resolution.width, resolution.height, fps, bitrate);
        auto encoder = CreateVideoEncoder({resolution.width, resolution.height, fps, bitrate, codec},
                                          encoderPreference, renderer.Exporter());
        renderer.ConfigureCapture(encoder->UsesGpuFrames());
        rtc::InitLogger(rtc::LogLevel::Warning);
        auto camera = std::make_shared<CameraState>();
        WebRTCStreamer streamer(camera, options);
        std::signal(SIGINT, HandleSignal);
        std::signal(SIGTERM, HandleSignal);
        std::cout << encoder->BackendName() << ' ' << CodecName(encoder->Codec()) << " ready ("
                  << resolution.width << 'x' << resolution.height << ", " << fps << " FPS, " << bitrate << " bps). Signaling: ws://" << options.bindAddress << ':' << streamer.Port()
                  << "\nOpen index.html and connect. Ctrl+C stops.\n" << std::flush;
        // On implementations where high_resolution_clock aliases a wall clock, use its
        // monotonic counterpart so clock corrections cannot stall frame pacing.
        using Clock = std::conditional_t<std::chrono::high_resolution_clock::is_steady,
            std::chrono::high_resolution_clock, std::chrono::steady_clock>;
        const auto framePeriod = std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(1.0 / fps));
        const auto start = Clock::now();
        auto nextFrame = start;
        auto reportTime = start;
        uint64_t frames = 0, encodedFrames = 0, reportFrames = 0, skippedSlots = 0;
        while (!stopRequested && (!frameLimit || frames < frameLimit)) {
            std::this_thread::sleep_until(nextFrame);
            streamer.Poll();
            const auto captureTime = Clock::now() - start;
            const double seconds = std::chrono::duration<double>(captureTime).count();
            const double rotationPeriod = 2 * std::numbers::pi_v<double> / 0.65;
            const auto rgba = renderer.RenderFrame(static_cast<float>(std::fmod(seconds, rotationPeriod)), camera->Load());
            if (streamer.ConsumeKeyFrameRequest()) encoder->RequestKeyFrame();
            // Bounded runs always encode, making the smoke test exercise the complete capture path.
            if (encoder->UsesGpuFrames() || frameLimit || streamer.HasViewers()) {
                const auto h264 = encoder->UsesGpuFrames() ? encoder->EncodeGpuFrame() : encoder->EncodeFrame(rgba.data());
                if (!h264.empty()) {
                    ++encodedFrames;
                    streamer.SendFrame(h264, std::chrono::duration<double>(captureTime));
                }
            }
            ++frames;
            nextFrame += framePeriod;
            const auto now = Clock::now();
            if (now > nextFrame) {
                const auto missed = (now - nextFrame) / framePeriod + 1;
                skippedSlots += static_cast<uint64_t>(missed);
                nextFrame += framePeriod * missed;
            }
            if (now - reportTime >= std::chrono::seconds(5)) {
                const double elapsed = std::chrono::duration<double>(now - reportTime).count();
                std::cout << "Render FPS: " << static_cast<double>(frames - reportFrames) / elapsed
                          << ", encoded: " << encodedFrames << ", missed slots: " << skippedSlots << '\n' << std::flush;
                reportTime = now;
                reportFrames = frames;
            }
        }
        std::cout << "Stopped after " << frames << " frames, " << encodedFrames << " encoded, "
                  << skippedSlots << " missed slots\n";
        if (frameLimit && encodedFrames == 0) throw std::runtime_error("Encoder produced no frames");
#endif
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
