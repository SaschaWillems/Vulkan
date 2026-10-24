// Vulkan 1.1 / C++20 offscreen renderer and cross-platform WebRTC server.
// See README.md for SDK setup, build, browser connection, and tests.
#include "Scene.h"
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

// The portability-subset name is still in Vulkan's provisional header.
#ifndef VK_ENABLE_BETA_EXTENSIONS
#define VK_ENABLE_BETA_EXTENSIONS
#endif
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

// Generated with glslangValidator -V --target-env vulkan1.1 -Os, then spirv-val.
// #version 450
// layout(binding=0) uniform Scene { mat4 mvp; } scene;
// layout(location=0) out vec3 color;
// void main() {
//   const vec3 positions[3] = vec3[3](vec3(-0.95,-0.75,0),
//       vec3(0.95,-0.75,0), vec3(0,0.95,0.25));
//   const vec3 colors[3] = vec3[3](vec3(1,0,0),vec3(0,1,0),vec3(0,0,1));
//   gl_Position = scene.mvp * vec4(positions[gl_VertexIndex], 1);
//   color = colors[gl_VertexIndex];
// }
constexpr uint32_t vertexShader[] = {
    0x07230203, 0x00010300, 0x0008000b, 0x0000003d, 0x00000000, 0x00020011, 0x00000001, 0x0006000b,
    0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001,
    0x0008000f, 0x00000000, 0x00000004, 0x6e69616d, 0x00000000, 0x0000000d, 0x00000024, 0x00000034,
    0x00030003, 0x00000002, 0x000001c2, 0x00040005, 0x00000004, 0x6e69616d, 0x00000000, 0x00060005,
    0x0000000b, 0x505f6c67, 0x65567265, 0x78657472, 0x00000000, 0x00060006, 0x0000000b, 0x00000000,
    0x505f6c67, 0x7469736f, 0x006e6f69, 0x00070006, 0x0000000b, 0x00000001, 0x505f6c67, 0x746e696f,
    0x657a6953, 0x00000000, 0x00070006, 0x0000000b, 0x00000002, 0x435f6c67, 0x4470696c, 0x61747369,
    0x0065636e, 0x00070006, 0x0000000b, 0x00000003, 0x435f6c67, 0x446c6c75, 0x61747369, 0x0065636e,
    0x00030005, 0x0000000d, 0x00000000, 0x00040005, 0x00000011, 0x6e656353, 0x00000065, 0x00040006,
    0x00000011, 0x00000000, 0x0070766d, 0x00040005, 0x00000013, 0x6e656373, 0x00000065, 0x00060005,
    0x00000024, 0x565f6c67, 0x65747265, 0x646e4978, 0x00007865, 0x00050005, 0x00000027, 0x65646e69,
    0x6c626178, 0x00000065, 0x00040005, 0x00000034, 0x6f6c6f63, 0x00000072, 0x00050005, 0x0000003a,
    0x65646e69, 0x6c626178, 0x00000065, 0x00030047, 0x0000000b, 0x00000002, 0x00050048, 0x0000000b,
    0x00000000, 0x0000000b, 0x00000000, 0x00050048, 0x0000000b, 0x00000001, 0x0000000b, 0x00000001,
    0x00050048, 0x0000000b, 0x00000002, 0x0000000b, 0x00000003, 0x00050048, 0x0000000b, 0x00000003,
    0x0000000b, 0x00000004, 0x00030047, 0x00000011, 0x00000002, 0x00040048, 0x00000011, 0x00000000,
    0x00000005, 0x00050048, 0x00000011, 0x00000000, 0x00000007, 0x00000010, 0x00050048, 0x00000011,
    0x00000000, 0x00000023, 0x00000000, 0x00040047, 0x00000013, 0x00000021, 0x00000000, 0x00040047,
    0x00000013, 0x00000022, 0x00000000, 0x00040047, 0x00000024, 0x0000000b, 0x0000002a, 0x00040047,
    0x00000034, 0x0000001e, 0x00000000, 0x00020013, 0x00000002, 0x00030021, 0x00000003, 0x00000002,
    0x00030016, 0x00000006, 0x00000020, 0x00040017, 0x00000007, 0x00000006, 0x00000004, 0x00040015,
    0x00000008, 0x00000020, 0x00000000, 0x0004002b, 0x00000008, 0x00000009, 0x00000001, 0x0004001c,
    0x0000000a, 0x00000006, 0x00000009, 0x0006001e, 0x0000000b, 0x00000007, 0x00000006, 0x0000000a,
    0x0000000a, 0x00040020, 0x0000000c, 0x00000003, 0x0000000b, 0x0004003b, 0x0000000c, 0x0000000d,
    0x00000003, 0x00040015, 0x0000000e, 0x00000020, 0x00000001, 0x0004002b, 0x0000000e, 0x0000000f,
    0x00000000, 0x00040018, 0x00000010, 0x00000007, 0x00000004, 0x0003001e, 0x00000011, 0x00000010,
    0x00040020, 0x00000012, 0x00000002, 0x00000011, 0x0004003b, 0x00000012, 0x00000013, 0x00000002,
    0x00040020, 0x00000014, 0x00000002, 0x00000010, 0x00040017, 0x00000017, 0x00000006, 0x00000003,
    0x0004002b, 0x00000008, 0x00000018, 0x00000003, 0x0004001c, 0x00000019, 0x00000017, 0x00000018,
    0x0004002b, 0x00000006, 0x0000001a, 0xbf733333, 0x0004002b, 0x00000006, 0x0000001b, 0xbf400000,
    0x0004002b, 0x00000006, 0x0000001c, 0x00000000, 0x0006002c, 0x00000017, 0x0000001d, 0x0000001a,
    0x0000001b, 0x0000001c, 0x0004002b, 0x00000006, 0x0000001e, 0x3f733333, 0x0006002c, 0x00000017,
    0x0000001f, 0x0000001e, 0x0000001b, 0x0000001c, 0x0004002b, 0x00000006, 0x00000020, 0x3e800000,
    0x0006002c, 0x00000017, 0x00000021, 0x0000001c, 0x0000001e, 0x00000020, 0x0006002c, 0x00000019,
    0x00000022, 0x0000001d, 0x0000001f, 0x00000021, 0x00040020, 0x00000023, 0x00000001, 0x0000000e,
    0x0004003b, 0x00000023, 0x00000024, 0x00000001, 0x00040020, 0x00000026, 0x00000007, 0x00000019,
    0x00040020, 0x00000028, 0x00000007, 0x00000017, 0x0004002b, 0x00000006, 0x0000002b, 0x3f800000,
    0x00040020, 0x00000031, 0x00000003, 0x00000007, 0x00040020, 0x00000033, 0x00000003, 0x00000017,
    0x0004003b, 0x00000033, 0x00000034, 0x00000003, 0x0006002c, 0x00000017, 0x00000035, 0x0000002b,
    0x0000001c, 0x0000001c, 0x0006002c, 0x00000017, 0x00000036, 0x0000001c, 0x0000002b, 0x0000001c,
    0x0006002c, 0x00000017, 0x00000037, 0x0000001c, 0x0000001c, 0x0000002b, 0x0006002c, 0x00000019,
    0x00000038, 0x00000035, 0x00000036, 0x00000037, 0x00050036, 0x00000002, 0x00000004, 0x00000000,
    0x00000003, 0x000200f8, 0x00000005, 0x0004003b, 0x00000026, 0x00000027, 0x00000007, 0x0004003b,
    0x00000026, 0x0000003a, 0x00000007, 0x00050041, 0x00000014, 0x00000015, 0x00000013, 0x0000000f,
    0x0004003d, 0x00000010, 0x00000016, 0x00000015, 0x0004003d, 0x0000000e, 0x00000025, 0x00000024,
    0x0003003e, 0x00000027, 0x00000022, 0x00050041, 0x00000028, 0x00000029, 0x00000027, 0x00000025,
    0x0004003d, 0x00000017, 0x0000002a, 0x00000029, 0x00050051, 0x00000006, 0x0000002c, 0x0000002a,
    0x00000000, 0x00050051, 0x00000006, 0x0000002d, 0x0000002a, 0x00000001, 0x00050051, 0x00000006,
    0x0000002e, 0x0000002a, 0x00000002, 0x00070050, 0x00000007, 0x0000002f, 0x0000002c, 0x0000002d,
    0x0000002e, 0x0000002b, 0x00050091, 0x00000007, 0x00000030, 0x00000016, 0x0000002f, 0x00050041,
    0x00000031, 0x00000032, 0x0000000d, 0x0000000f, 0x0003003e, 0x00000032, 0x00000030, 0x0003003e,
    0x0000003a, 0x00000038, 0x00050041, 0x00000028, 0x0000003b, 0x0000003a, 0x00000025, 0x0004003d,
    0x00000017, 0x0000003c, 0x0000003b, 0x0003003e, 0x00000034, 0x0000003c, 0x000100fd, 0x00010038,
};

// Fragment shader:
// #version 450
// layout(location=0) in vec3 color;
// layout(location=0) out vec4 outColor;
// void main() { outColor = vec4(color, 1.0); }
constexpr uint32_t fragmentShader[] = {
    0x07230203, 0x00010300, 0x0008000b, 0x00000013, 0x00000000, 0x00020011, 0x00000001, 0x0006000b,
    0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001,
    0x0007000f, 0x00000004, 0x00000004, 0x6e69616d, 0x00000000, 0x00000009, 0x0000000c, 0x00030010,
    0x00000004, 0x00000007, 0x00030003, 0x00000002, 0x000001c2, 0x00040005, 0x00000004, 0x6e69616d,
    0x00000000, 0x00050005, 0x00000009, 0x4374756f, 0x726f6c6f, 0x00000000, 0x00040005, 0x0000000c,
    0x6f6c6f63, 0x00000072, 0x00040047, 0x00000009, 0x0000001e, 0x00000000, 0x00040047, 0x0000000c,
    0x0000001e, 0x00000000, 0x00020013, 0x00000002, 0x00030021, 0x00000003, 0x00000002, 0x00030016,
    0x00000006, 0x00000020, 0x00040017, 0x00000007, 0x00000006, 0x00000004, 0x00040020, 0x00000008,
    0x00000003, 0x00000007, 0x0004003b, 0x00000008, 0x00000009, 0x00000003, 0x00040017, 0x0000000a,
    0x00000006, 0x00000003, 0x00040020, 0x0000000b, 0x00000001, 0x0000000a, 0x0004003b, 0x0000000b,
    0x0000000c, 0x00000001, 0x0004002b, 0x00000006, 0x0000000e, 0x3f800000, 0x00050036, 0x00000002,
    0x00000004, 0x00000000, 0x00000003, 0x000200f8, 0x00000005, 0x0004003d, 0x0000000a, 0x0000000d,
    0x0000000c, 0x00050051, 0x00000006, 0x0000000f, 0x0000000d, 0x00000000, 0x00050051, 0x00000006,
    0x00000010, 0x0000000d, 0x00000001, 0x00050051, 0x00000006, 0x00000011, 0x0000000d, 0x00000002,
    0x00070050, 0x00000007, 0x00000012, 0x0000000f, 0x00000010, 0x00000011, 0x0000000e, 0x0003003e,
    0x00000009, 0x00000012, 0x000100fd, 0x00010038,
};

class OffscreenRenderer {
public:
    static constexpr uint32_t width = 640;
    static constexpr uint32_t height = 480;
    OffscreenRenderer() = default;
    OffscreenRenderer(const OffscreenRenderer&) = delete;
    OffscreenRenderer& operator=(const OffscreenRenderer&) = delete;

    // Step 9: Reverse dependency order, also safe after any partial initialization.
    // Initialization happens in Initialize(), so this destructor runs if any step throws.
    ~OffscreenRenderer() {
        if (device_ != VK_NULL_HANDLE) {
            // Also covers exceptions after submission but before a successful fence wait.
            (void)vkDeviceWaitIdle(device_);
            if (mapped_ != nullptr) vkUnmapMemory(device_, stagingMemory_);
            if (uniformMapped_ != nullptr) vkUnmapMemory(device_, uniformMemory_);
            if (fence_) vkDestroyFence(device_, fence_, nullptr);
            if (commandPool_) vkDestroyCommandPool(device_, commandPool_, nullptr);
            // Destroying the pool implicitly frees its command buffers.
            if (stagingBuffer_) vkDestroyBuffer(device_, stagingBuffer_, nullptr);
            if (stagingMemory_) vkFreeMemory(device_, stagingMemory_, nullptr);
            if (pipeline_) vkDestroyPipeline(device_, pipeline_, nullptr);
            if (fragmentModule_) vkDestroyShaderModule(device_, fragmentModule_, nullptr);
            if (vertexModule_) vkDestroyShaderModule(device_, vertexModule_, nullptr);
            if (pipelineLayout_) vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
            if (descriptorPool_) vkDestroyDescriptorPool(device_, descriptorPool_, nullptr);
            if (descriptorLayout_) vkDestroyDescriptorSetLayout(device_, descriptorLayout_, nullptr);
            if (uniformBuffer_) vkDestroyBuffer(device_, uniformBuffer_, nullptr);
            if (uniformMemory_) vkFreeMemory(device_, uniformMemory_, nullptr);
            if (framebuffer_) vkDestroyFramebuffer(device_, framebuffer_, nullptr);
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

    void Initialize(bool allowGpuCapture = false, bool preferGpuBuffer = false) {
        requestGpuCapture_ = allowGpuCapture;
        preferGpuBuffer_ = preferGpuBuffer;
        createInstance();
        createDevice();
        createTarget();
        createRenderPassAndFramebuffer();
        createUniformBuffer();
        createPipeline();
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
        // Every preceding submission has finished before either mapped allocation is reused.
        const auto mvp = scene::MVP(seconds, camera, static_cast<float>(width) / height);
        std::memcpy(uniformMapped_, mvp.data(), sizeof(mvp));
        if (!uniformCoherent_) {
            auto range = structure<VkMappedMemoryRange>(VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE);
            range.memory = uniformMemory_;
            range.size = VK_WHOLE_SIZE;
            VK_CHECK(vkFlushMappedMemoryRanges(device_, 1, &range));
        }
        VK_CHECK(vkResetFences(device_, 1, &fence_));
        auto submit = structure<VkSubmitInfo>(VK_STRUCTURE_TYPE_SUBMIT_INFO);
        submit.commandBufferCount = 1;
        const VkCommandBuffer commands = firstFrame_ ? firstCommands_ : commands_;
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
        std::array<char, width * 3> row{};
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
    static constexpr VkDeviceSize pixelBytes = VkDeviceSize{width} * height * 4;

    std::atomic_uint32_t validationErrors_{0};
    VkInstance instance_{};
    VkDebugUtilsMessengerEXT debugMessenger_{};
    PFN_vkDestroyDebugUtilsMessengerEXT destroyDebugMessenger_{};
    VkPhysicalDevice physicalDevice_{};
    VkPhysicalDeviceMemoryProperties memoryProperties_{};
    uint32_t queueFamily_{};
    VkDevice device_{};
    VkQueue queue_{};
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
    VkBuffer uniformBuffer_{};
    VkDeviceMemory uniformMemory_{};
    void* uniformMapped_{};
    bool uniformCoherent_{};
    VkDescriptorSetLayout descriptorLayout_{};
    VkDescriptorPool descriptorPool_{};
    VkDescriptorSet descriptorSet_{};
    VkPipelineLayout pipelineLayout_{};
    VkShaderModule vertexModule_{};
    VkShaderModule fragmentModule_{};
    VkPipeline pipeline_{};
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
        application.pApplicationName = "Offscreen triangle";
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
            throw std::runtime_error("No Vulkan 1.1 graphics device supports the offscreen target");
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
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        // Synchronize the preceding frame's copy before discarding/reusing the image.
        dependency.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dependency.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        auto info = structure<VkRenderPassCreateInfo>(VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO);
        info.attachmentCount = 1;
        info.pAttachments = &attachment;
        info.subpassCount = 1;
        info.pSubpasses = &subpass;
        info.dependencyCount = 1;
        info.pDependencies = &dependency;
        VK_CHECK(vkCreateRenderPass(device_, &info, nullptr, &renderPass_));

        // Step 5: The framebuffer binds the image view to this single render-pass attachment.
        auto framebuffer = structure<VkFramebufferCreateInfo>(VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO);
        framebuffer.renderPass = renderPass_;
        framebuffer.attachmentCount = 1;
        framebuffer.pAttachments = &imageView_;
        framebuffer.width = width;
        framebuffer.height = height;
        framebuffer.layers = 1;
        VK_CHECK(vkCreateFramebuffer(device_, &framebuffer, nullptr, &framebuffer_));
    }

    void createUniformBuffer() {
        auto buffer = structure<VkBufferCreateInfo>(VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO);
        buffer.size = sizeof(scene::Matrix);
        buffer.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        buffer.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VK_CHECK(vkCreateBuffer(device_, &buffer, nullptr, &uniformBuffer_));
        VkMemoryRequirements requirements{};
        vkGetBufferMemoryRequirements(device_, uniformBuffer_, &requirements);
        const uint32_t type = memoryType(requirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        uniformCoherent_ = (memoryProperties_.memoryTypes[type].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0;
        auto allocation = structure<VkMemoryAllocateInfo>(VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO);
        allocation.allocationSize = requirements.size;
        allocation.memoryTypeIndex = type;
        VK_CHECK(vkAllocateMemory(device_, &allocation, nullptr, &uniformMemory_));
        VK_CHECK(vkBindBufferMemory(device_, uniformBuffer_, uniformMemory_, 0));
        VK_CHECK(vkMapMemory(device_, uniformMemory_, 0, VK_WHOLE_SIZE, 0, &uniformMapped_));

        VkDescriptorSetLayoutBinding binding{};
        binding.binding = 0;
        binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        binding.descriptorCount = 1;
        binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        auto layout = structure<VkDescriptorSetLayoutCreateInfo>(VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO);
        layout.bindingCount = 1;
        layout.pBindings = &binding;
        VK_CHECK(vkCreateDescriptorSetLayout(device_, &layout, nullptr, &descriptorLayout_));
        const VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1};
        auto pool = structure<VkDescriptorPoolCreateInfo>(VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO);
        pool.maxSets = 1;
        pool.poolSizeCount = 1;
        pool.pPoolSizes = &size;
        VK_CHECK(vkCreateDescriptorPool(device_, &pool, nullptr, &descriptorPool_));
        auto sets = structure<VkDescriptorSetAllocateInfo>(VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO);
        sets.descriptorPool = descriptorPool_;
        sets.descriptorSetCount = 1;
        sets.pSetLayouts = &descriptorLayout_;
        VK_CHECK(vkAllocateDescriptorSets(device_, &sets, &descriptorSet_));
        const VkDescriptorBufferInfo uniform{uniformBuffer_, 0, sizeof(scene::Matrix)};
        auto write = structure<VkWriteDescriptorSet>(VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET);
        write.dstSet = descriptorSet_;
        write.dstBinding = 0;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.pBufferInfo = &uniform;
        vkUpdateDescriptorSets(device_, 1, &write, 0, nullptr);
    }

    template <size_t N>
    void createShader(const uint32_t (&code)[N], VkShaderModule& module) {
        auto info = structure<VkShaderModuleCreateInfo>(VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO);
        info.codeSize = sizeof(code);
        info.pCode = code;
        VK_CHECK(vkCreateShaderModule(device_, &info, nullptr, &module));
    }

    // Step 6: Procedural vertices and an MVP uniform; shaders are embedded SPIR-V.
    void createPipeline() {
        auto layout = structure<VkPipelineLayoutCreateInfo>(VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO);
        layout.setLayoutCount = 1;
        layout.pSetLayouts = &descriptorLayout_;
        VK_CHECK(vkCreatePipelineLayout(device_, &layout, nullptr, &pipelineLayout_));
        createShader(vertexShader, vertexModule_);
        createShader(fragmentShader, fragmentModule_);
        std::array<VkPipelineShaderStageCreateInfo, 2> stages{};
        for (auto& stage : stages) {
            stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            stage.pName = "main";
        }
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = vertexModule_;
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = fragmentModule_;
        auto vertices = structure<VkPipelineVertexInputStateCreateInfo>(
            VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO);
        auto assembly = structure<VkPipelineInputAssemblyStateCreateInfo>(
            VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO);
        assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        const VkViewport viewport{0, 0, static_cast<float>(width), static_cast<float>(height), 0, 1};
        const VkRect2D scissor{{0, 0}, {width, height}};
        auto viewportState = structure<VkPipelineViewportStateCreateInfo>(
            VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO);
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;
        auto raster = structure<VkPipelineRasterizationStateCreateInfo>(
            VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO);
        raster.polygonMode = VK_POLYGON_MODE_FILL;
        raster.cullMode = VK_CULL_MODE_NONE;
        raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        raster.lineWidth = 1.0f;
        auto multisample = structure<VkPipelineMultisampleStateCreateInfo>(
            VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO);
        multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        VkPipelineColorBlendAttachmentState blendAttachment{};
        blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                         VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        auto blend = structure<VkPipelineColorBlendStateCreateInfo>(
            VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO);
        blend.attachmentCount = 1;
        blend.pAttachments = &blendAttachment;
        auto info = structure<VkGraphicsPipelineCreateInfo>(VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO);
        info.stageCount = static_cast<uint32_t>(stages.size());
        info.pStages = stages.data();
        info.pVertexInputState = &vertices;
        info.pInputAssemblyState = &assembly;
        info.pViewportState = &viewportState;
        info.pRasterizationState = &raster;
        info.pMultisampleState = &multisample;
        info.pColorBlendState = &blend;
        info.layout = pipelineLayout_;
        info.renderPass = renderPass_;
        VK_CHECK(vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &info, nullptr, &pipeline_));
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
        VK_CHECK(vkCreateCommandPool(device_, &pool, nullptr, &commandPool_));
        auto allocation = structure<VkCommandBufferAllocateInfo>(VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO);
        allocation.commandPool = commandPool_;
        allocation.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocation.commandBufferCount = 2;
        VkCommandBuffer buffers[2]{};
        VK_CHECK(vkAllocateCommandBuffers(device_, &allocation, buffers));
        firstCommands_ = buffers[0]; commands_ = buffers[1];
        recordFrame(firstCommands_, true);
        recordFrame(commands_, false);
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
        const VkClearValue clear{{{0.04f, 0.06f, 0.10f, 1.0f}}};
        auto render = structure<VkRenderPassBeginInfo>(VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO);
        render.renderPass = renderPass_;
        render.framebuffer = framebuffer_;
        render.renderArea = {{0, 0}, {width, height}};
        render.clearValueCount = 1;
        render.pClearValues = &clear;
        vkCmdBeginRenderPass(commands, &render, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commands, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);
        vkCmdBindDescriptorSets(commands, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout_,
                                0, 1, &descriptorSet_, 0, nullptr);
        vkCmdDraw(commands, 3, 1, 0, 0);
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
    if (changed(renderer.RenderFrame(0, {})) != 0)
        throw std::runtime_error("Uniform/framebuffer reuse was not deterministic");
    std::cout << "Animation, camera orbit, and framebuffer reuse passed\n";
}

} // namespace

int main(int argc, char** argv) {
    try {
        bool snapshot = false, renderTest = false;
        uint32_t frameLimit = 0;
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
            else if (arg == "--render-test") renderTest = true;
            else if (arg == "--help") {
                std::cout << "Usage: offscreen_renderer [--snapshot | --render-test]\n"
                             "       offscreen_renderer [--frames N] [--port 8080] [--bind 127.0.0.1]\n"
                             "                          [--ice-server stun:host:port]\n"
                             "                          [--encoder auto|hardware|software] [--nvenc-input image|buffer]\n"
                             "Default: 640x480, 60 FPS, 4 Mbps H.264; Ctrl+C stops the server.\n";
                return 0;
            } else if (arg == "--frames") {
                frameLimit = ParseUnsigned(next(), 100000000);
                if (!frameLimit) throw std::invalid_argument("--frames must be positive");
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
        OffscreenRenderer renderer;
        bool allowGpuCapture = false;
#ifdef OFFSCREEN_HAS_NVENC
        allowGpuCapture = !snapshot && !renderTest && encoderPreference != EncoderPreference::Software;
#endif
        renderer.Initialize(allowGpuCapture, gpuBuffer);
        if (snapshot || renderTest) renderer.ConfigureCapture(false);
        if (renderTest) { TestRendering(renderer); return 0; }
        if (snapshot) { renderer.RenderFrame(0, {}); renderer.SavePpm(); return 0; }
#ifdef OFFSCREEN_STREAMING
        constexpr uint32_t fps = 60;
        auto encoder = CreateVideoEncoder({OffscreenRenderer::width, OffscreenRenderer::height, fps, 4000000},
                                          encoderPreference, renderer.Exporter());
        renderer.ConfigureCapture(encoder->UsesGpuFrames());
        rtc::InitLogger(rtc::LogLevel::Warning);
        auto camera = std::make_shared<CameraState>();
        WebRTCStreamer streamer(camera, options);
        std::signal(SIGINT, HandleSignal);
        std::signal(SIGTERM, HandleSignal);
        std::cout << encoder->BackendName() << " H.264 ready. Signaling: ws://" << options.bindAddress << ':' << streamer.Port()
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
