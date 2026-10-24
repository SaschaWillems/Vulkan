#pragma once
#include "GltfLoader.h"
#include <memory>

class AccelerationStructure {
public:
    AccelerationStructure();
    ~AccelerationStructure();
    AccelerationStructure(const AccelerationStructure&) = delete;
    AccelerationStructure& operator=(const AccelerationStructure&) = delete;
    void Initialize(VmaAllocator allocator, VkPhysicalDevice physical, VkDevice device, VkQueue queue,
                    uint32_t family, const GltfScene& scene, VkBuffer vertices, VkBuffer indices);
    void Prepare(const glm::mat4& root);
    void RecordUpdate(VkCommandBuffer commands);
    VkAccelerationStructureKHR Tlas() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
