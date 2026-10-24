#pragma once
#include "UniqueFd.h"
#include <vulkan/vulkan.h>
#include <array>
#include <memory>

// Dedicated device-local allocation and binary semaphores shared with CUDA.
// All CUDA/NVENC imports must be destroyed before this object and its VkDevice.
class VulkanMemoryExporter {
public:
    enum class Kind { Image, Buffer };
    static std::unique_ptr<VulkanMemoryExporter> Create(VkPhysicalDevice physicalDevice,
        VkDevice device, uint32_t width, uint32_t height, Kind kind);
    ~VulkanMemoryExporter();
    VulkanMemoryExporter(const VulkanMemoryExporter&) = delete;
    VulkanMemoryExporter& operator=(const VulkanMemoryExporter&) = delete;
    UniqueFd ExportMemoryFd() const;
    UniqueFd ExportReadyFd() const;
    UniqueFd ExportDoneFd() const;

    Kind Type() const { return kind_; }
    VkImage Image() const { return image_; }
    VkBuffer Buffer() const { return buffer_; }
    VkDeviceSize AllocationSize() const { return allocationSize_; }
    uint32_t Width() const { return width_; }
    uint32_t Height() const { return height_; }
    VkSemaphore Ready() const { return ready_; }
    VkSemaphore Done() const { return done_; }
    const std::array<uint8_t, VK_UUID_SIZE>& DeviceUuid() const { return uuid_; }
private:
    VulkanMemoryExporter() = default;
    void Initialize(VkPhysicalDevice physicalDevice, VkDevice device, uint32_t width, uint32_t height, Kind kind);
    UniqueFd ExportSemaphoreFd(VkSemaphore semaphore) const;
    Kind kind_ = Kind::Image;
    VkDevice device_{};
    VkImage image_{};
    VkBuffer buffer_{};
    VkDeviceMemory memory_{};
    VkDeviceSize allocationSize_{};
    VkSemaphore ready_{}, done_{};
    uint32_t width_{}, height_{};
    std::array<uint8_t, VK_UUID_SIZE> uuid_{};
    PFN_vkGetMemoryFdKHR getMemoryFd_{};
    PFN_vkGetSemaphoreFdKHR getSemaphoreFd_{};
};
