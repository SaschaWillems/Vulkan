#include "VulkanContext.h"
#include <array>
#include <cstring>

namespace {
constexpr std::array required{VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
                              VK_KHR_RAY_QUERY_EXTENSION_NAME,
                              VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
                              VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
                              VK_KHR_SPIRV_1_4_EXTENSION_NAME,
                              VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,
                              VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME};
}
RayQuerySupport QueryRayQuerySupport(VkPhysicalDevice physical, uint32_t queueFamily,
                                     std::span<const VkExtensionProperties> extensions) {
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physical, &properties);
    // Raster remains Vulkan 1.1; RT uses the promoted Vulkan 1.2 dependency baseline.
    if (properties.apiVersion < VK_API_VERSION_1_2)
        return {false, "ray queries require Vulkan 1.2+"};
    for (const auto* name : required) {
        bool found = false;
        for (const auto& extension : extensions)
            found |= std::strcmp(extension.extensionName, name) == 0;
        // These dependencies were promoted to 1.2 and need not be advertised separately.
        const bool promoted = std::strcmp(name, VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME) == 0 ||
                              std::strcmp(name, VK_KHR_SPIRV_1_4_EXTENSION_NAME) == 0 ||
                              std::strcmp(name, VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME) == 0 ||
                              std::strcmp(name, VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME) == 0;
        if (!found && !promoted)
            return {false, std::string("missing ") + name};
    }
    VkPhysicalDeviceBufferDeviceAddressFeatures address{};
    address.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    VkPhysicalDeviceAccelerationStructureFeaturesKHR acceleration{};
    acceleration.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    VkPhysicalDeviceRayQueryFeaturesKHR query{};
    query.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
    VkPhysicalDeviceFeatures2 features{};
    features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features.pNext = &address;
    address.pNext = &acceleration;
    acceleration.pNext = &query;
    vkGetPhysicalDeviceFeatures2(physical, &features);
    if (!address.bufferDeviceAddress || !acceleration.accelerationStructure || !query.rayQuery)
        return {false, "bufferDeviceAddress / accelerationStructure / rayQuery feature unavailable"};
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical, &count, nullptr);
    std::vector<VkQueueFamilyProperties> queues(count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical, &count, queues.data());
    if (queueFamily >= count || !(queues[queueFamily].queueFlags & VK_QUEUE_COMPUTE_BIT))
        return {false, "graphics queue does not support compute"};
    if (properties.limits.maxPerStageDescriptorStorageBuffers < 5 ||
        properties.limits.maxPerStageDescriptorStorageImages < 2)
        return {false, "insufficient storage descriptors for ray-query scene"};
    for (const auto format : {VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_R8G8B8A8_UNORM}) {
        VkFormatProperties support{};
        vkGetPhysicalDeviceFormatProperties(physical, format, &support);
        const auto needed = VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_SRC_BIT;
        if ((support.optimalTilingFeatures & needed) != needed)
            return {false, "path tracing storage/transfer image format unsupported"};
    }
    return {true, {}};
}
void RayQueryDeviceFeatures::Enable(VkDeviceCreateInfo& info, std::vector<const char*>& extensions) {
    // On Vulkan 1.2 the BDA, SPIR-V 1.4, float-controls and descriptor-indexing dependencies are core.
    extensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
    extensions.push_back(VK_KHR_RAY_QUERY_EXTENSION_NAME);
    extensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
    address.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    address.bufferDeviceAddress = VK_TRUE;
    acceleration.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    acceleration.accelerationStructure = VK_TRUE;
    query.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
    query.rayQuery = VK_TRUE;
    address.pNext = &acceleration;
    acceleration.pNext = &query;
    info.pNext = &address;
}
