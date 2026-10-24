#pragma once
#include <vulkan/vulkan.h>
#include <span>
#include <string>
#include <vector>

struct RayQuerySupport {
    bool available = false;
    std::string reason;
};
RayQuerySupport QueryRayQuerySupport(VkPhysicalDevice physical, uint32_t queueFamily,
                                     std::span<const VkExtensionProperties> extensions);
// Owned by the caller until vkCreateDevice returns; only explicitly used features are enabled.
struct RayQueryDeviceFeatures {
    VkPhysicalDeviceBufferDeviceAddressFeatures address{};
    VkPhysicalDeviceAccelerationStructureFeaturesKHR acceleration{};
    VkPhysicalDeviceRayQueryFeaturesKHR query{};
    void Enable(VkDeviceCreateInfo& info, std::vector<const char*>& extensions);
};
