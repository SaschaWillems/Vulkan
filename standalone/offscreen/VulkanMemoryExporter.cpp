#include "VulkanMemoryExporter.h"
#include <algorithm>
#include <cerrno>
#include <fcntl.h>
#include <stdexcept>
#include <string>
#include <system_error>

namespace {
void Check(VkResult result, const char* operation) {
    if (result != VK_SUCCESS)
        throw std::runtime_error(std::string(operation) + " failed (VkResult " + std::to_string(result) + ")");
}
#define VK_CHECK(expression) Check((expression), #expression)
template<typename T> T Info(VkStructureType type) { T result{}; result.sType = type; return result; }
UniqueFd CloseOnExec(int fd) {
    UniqueFd owned(fd);
    if (fd < 0 || fcntl(fd, F_SETFD, FD_CLOEXEC) < 0)
        throw std::system_error(errno, std::generic_category(), "Export Vulkan FD");
    return owned;
}
}

std::unique_ptr<VulkanMemoryExporter> VulkanMemoryExporter::Create(VkPhysicalDevice physicalDevice,
    VkDevice device, uint32_t width, uint32_t height, Kind kind) {
    auto resource = std::unique_ptr<VulkanMemoryExporter>(new VulkanMemoryExporter());
    resource->Initialize(physicalDevice, device, width, height, kind);
    return resource;
}

void VulkanMemoryExporter::Initialize(VkPhysicalDevice physicalDevice, VkDevice device,
    uint32_t width, uint32_t height, Kind kind) {
    if (!width || !height) throw std::invalid_argument("Export dimensions must be positive");
    device_ = device; kind_ = kind; width_ = width; height_ = height;
    getMemoryFd_ = reinterpret_cast<PFN_vkGetMemoryFdKHR>(vkGetDeviceProcAddr(device_, "vkGetMemoryFdKHR"));
    getSemaphoreFd_ = reinterpret_cast<PFN_vkGetSemaphoreFdKHR>(vkGetDeviceProcAddr(device_, "vkGetSemaphoreFdKHR"));
    if (!getMemoryFd_ || !getSemaphoreFd_) throw std::runtime_error("External-memory/semaphore FD extensions unavailable");
    auto id = Info<VkPhysicalDeviceIDProperties>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES);
    auto properties = Info<VkPhysicalDeviceProperties2>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2);
    properties.pNext = &id;
    vkGetPhysicalDeviceProperties2(physicalDevice, &properties);
    std::copy_n(id.deviceUUID, VK_UUID_SIZE, uuid_.begin());

    auto semaphoreInfo = Info<VkPhysicalDeviceExternalSemaphoreInfo>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_SEMAPHORE_INFO);
    semaphoreInfo.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;
    auto semaphoreProperties = Info<VkExternalSemaphoreProperties>(VK_STRUCTURE_TYPE_EXTERNAL_SEMAPHORE_PROPERTIES);
    vkGetPhysicalDeviceExternalSemaphoreProperties(physicalDevice, &semaphoreInfo, &semaphoreProperties);
    if (!(semaphoreProperties.externalSemaphoreFeatures & VK_EXTERNAL_SEMAPHORE_FEATURE_EXPORTABLE_BIT))
        throw std::runtime_error("Opaque-FD semaphores are not exportable on this Vulkan device");

    VkMemoryRequirements requirements{};
    if (kind == Kind::Image) {
        auto external = Info<VkPhysicalDeviceExternalImageFormatInfo>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_IMAGE_FORMAT_INFO);
        external.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
        auto query = Info<VkPhysicalDeviceImageFormatInfo2>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2);
        query.pNext = &external;
        query.format = VK_FORMAT_R8G8B8A8_UNORM;
        query.type = VK_IMAGE_TYPE_2D;
        query.tiling = VK_IMAGE_TILING_OPTIMAL;
        query.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        auto exportProperties = Info<VkExternalImageFormatProperties>(VK_STRUCTURE_TYPE_EXTERNAL_IMAGE_FORMAT_PROPERTIES);
        auto supported = Info<VkImageFormatProperties2>(VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2);
        supported.pNext = &exportProperties;
        VK_CHECK(vkGetPhysicalDeviceImageFormatProperties2(physicalDevice, &query, &supported));
        if (!(exportProperties.externalMemoryProperties.externalMemoryFeatures & VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT))
            throw std::runtime_error("RGBA color-attachment image memory cannot be exported");
        if (width > supported.imageFormatProperties.maxExtent.width ||
            height > supported.imageFormatProperties.maxExtent.height ||
            !(supported.imageFormatProperties.sampleCounts & VK_SAMPLE_COUNT_1_BIT))
            throw std::runtime_error("Exported color attachment exceeds device image limits");
        auto exportImage = Info<VkExternalMemoryImageCreateInfo>(VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO);
        exportImage.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
        auto image = Info<VkImageCreateInfo>(VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO);
        image.pNext = &exportImage;
        image.imageType = VK_IMAGE_TYPE_2D;
        image.format = query.format;
        image.extent = {width, height, 1};
        image.mipLevels = image.arrayLayers = 1;
        image.samples = VK_SAMPLE_COUNT_1_BIT;
        image.tiling = query.tiling;
        image.usage = query.usage;
        image.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        image.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        VK_CHECK(vkCreateImage(device_, &image, nullptr, &image_));
        vkGetImageMemoryRequirements(device_, image_, &requirements);
    } else {
        auto query = Info<VkPhysicalDeviceExternalBufferInfo>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_BUFFER_INFO);
        query.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        query.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
        auto supported = Info<VkExternalBufferProperties>(VK_STRUCTURE_TYPE_EXTERNAL_BUFFER_PROPERTIES);
        vkGetPhysicalDeviceExternalBufferProperties(physicalDevice, &query, &supported);
        if (!(supported.externalMemoryProperties.externalMemoryFeatures & VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT))
            throw std::runtime_error("Device-local buffer memory cannot be exported");
        auto external = Info<VkExternalMemoryBufferCreateInfo>(VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO);
        external.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
        auto buffer = Info<VkBufferCreateInfo>(VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO);
        buffer.pNext = &external;
        buffer.size = VkDeviceSize{width} * height * 4;
        buffer.usage = query.usage;
        buffer.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VK_CHECK(vkCreateBuffer(device_, &buffer, nullptr, &buffer_));
        vkGetBufferMemoryRequirements(device_, buffer_, &requirements);
    }
    VkPhysicalDeviceMemoryProperties memoryProperties{};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
    uint32_t memoryType = memoryProperties.memoryTypeCount;
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
        if ((requirements.memoryTypeBits & (uint32_t{1} << i)) &&
            (memoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
            memoryType = i;
            break;
        }
    }
    if (memoryType == memoryProperties.memoryTypeCount) throw std::runtime_error("No device-local export memory type");
    auto dedicated = Info<VkMemoryDedicatedAllocateInfo>(VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO);
    dedicated.image = image_;
    dedicated.buffer = buffer_;
    auto exportMemory = Info<VkExportMemoryAllocateInfo>(VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO);
    exportMemory.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
    exportMemory.pNext = &dedicated;
    auto allocation = Info<VkMemoryAllocateInfo>(VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO);
    allocation.pNext = &exportMemory;
    allocation.allocationSize = allocationSize_ = requirements.size;
    allocation.memoryTypeIndex = memoryType;
    VK_CHECK(vkAllocateMemory(device_, &allocation, nullptr, &memory_));
    if (image_) VK_CHECK(vkBindImageMemory(device_, image_, memory_, 0));
    else VK_CHECK(vkBindBufferMemory(device_, buffer_, memory_, 0));

    auto externalSemaphore = Info<VkExportSemaphoreCreateInfo>(VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO);
    externalSemaphore.handleTypes = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;
    auto semaphore = Info<VkSemaphoreCreateInfo>(VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO);
    semaphore.pNext = &externalSemaphore;
    VK_CHECK(vkCreateSemaphore(device_, &semaphore, nullptr, &ready_));
    VK_CHECK(vkCreateSemaphore(device_, &semaphore, nullptr, &done_));
}

VulkanMemoryExporter::~VulkanMemoryExporter() {
    if (done_) vkDestroySemaphore(device_, done_, nullptr);
    if (ready_) vkDestroySemaphore(device_, ready_, nullptr);
    if (buffer_) vkDestroyBuffer(device_, buffer_, nullptr);
    if (image_) vkDestroyImage(device_, image_, nullptr);
    if (memory_) vkFreeMemory(device_, memory_, nullptr);
}
UniqueFd VulkanMemoryExporter::ExportMemoryFd() const {
    auto info = Info<VkMemoryGetFdInfoKHR>(VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR);
    info.memory = memory_;
    info.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
    int fd = -1;
    VK_CHECK(getMemoryFd_(device_, &info, &fd));
    return CloseOnExec(fd);
}
UniqueFd VulkanMemoryExporter::ExportSemaphoreFd(VkSemaphore semaphore) const {
    auto info = Info<VkSemaphoreGetFdInfoKHR>(VK_STRUCTURE_TYPE_SEMAPHORE_GET_FD_INFO_KHR);
    info.semaphore = semaphore;
    info.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;
    int fd = -1;
    VK_CHECK(getSemaphoreFd_(device_, &info, &fd));
    return CloseOnExec(fd);
}
UniqueFd VulkanMemoryExporter::ExportReadyFd() const { return ExportSemaphoreFd(ready_); }
UniqueFd VulkanMemoryExporter::ExportDoneFd() const { return ExportSemaphoreFd(done_); }
