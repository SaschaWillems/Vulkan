#pragma once
#include "RenderMode.h"
#include <vulkan/vulkan.h>
#include <filesystem>
#include <memory>

class PbrRenderer {
public:
    PbrRenderer();
    ~PbrRenderer();
    PbrRenderer(const PbrRenderer&) = delete;
    PbrRenderer& operator=(const PbrRenderer&) = delete;
    void Initialize(VkInstance instance, VkPhysicalDevice physical, VkDevice device, VkQueue queue,
                    uint32_t queueFamily, uint32_t width, uint32_t height, const std::filesystem::path& model,
                    bool rayQueries = false);
    VkImageView DepthView() const;
    VkFormat DepthFormat() const;
    void CreatePipeline(VkRenderPass renderPass);
    void PrepareFrame(CameraPose pose, float seconds, bool spin,
                      RenderModeSnapshot mode = {RenderMode::Raster, 0});
    void RecordBeforeRender(VkCommandBuffer commands);
    void Trace(VkCommandBuffer commands, VkImage target);
    void CompleteFrame();
    uint32_t Samples() const;
    void Draw(VkCommandBuffer commands);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
