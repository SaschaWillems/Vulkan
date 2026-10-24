#pragma once
#include "GltfLoader.h"
#include "RenderMode.h"
#include <memory>

class RayTracingRenderer {
public:
    RayTracingRenderer();
    ~RayTracingRenderer();
    RayTracingRenderer(const RayTracingRenderer&) = delete;
    RayTracingRenderer& operator=(const RayTracingRenderer&) = delete;
    void Initialize(VmaAllocator allocator, VkPhysicalDevice physical, VkDevice device, VkQueue queue,
                    uint32_t family, uint32_t width, uint32_t height, const GltfScene& scene,
                    VkBuffer vertices, VkBuffer indices);
    VkDescriptorSetLayout DescriptorLayout() const;
    void Prepare(const glm::mat4& root, const CameraMatrices& camera, RenderMode mode, uint32_t samples);
    void RecordUpdate(VkCommandBuffer commands);
    void BindGraphics(VkCommandBuffer commands, VkPipelineLayout layout);
    // Target must be in TRANSFER_DST_OPTIMAL; this method leaves it there.
    void Trace(VkCommandBuffer commands, VkImage target);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
