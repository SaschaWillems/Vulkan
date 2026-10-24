#pragma once
#include <glm/glm.hpp>
#include <vk_mem_alloc.h>
#include <filesystem>
#include <optional>
#include <span>
#include <vector>

struct GltfVertex {
    glm::vec3 position{0}, normal{0};
    glm::vec2 uv{0}, mrUv{0};
    glm::vec4 color{1};
};
struct GltfImage {
    uint32_t width = 1, height = 1;
    std::vector<uint8_t> rgba{255, 255, 255, 255};
};
struct GltfTexture {
    size_t image = 0;
    VkFilter mag = VK_FILTER_LINEAR, min = VK_FILTER_LINEAR;
    VkSamplerMipmapMode mipMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    bool mipmapped = true;
    VkSamplerAddressMode wrapU = VK_SAMPLER_ADDRESS_MODE_REPEAT, wrapV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
};
struct GltfMaterial {
    glm::vec4 baseColor{1};
    float metallic = 1, roughness = 1, alphaCutoff = 0.5f;
    int alphaMode = 0;
    bool doubleSided = false;
    size_t baseUv = 0, mrUv = 0;
    glm::mat3 baseUvTransform{1}, mrUvTransform{1};
    std::optional<size_t> baseTexture, metallicRoughnessTexture;
};
struct GltfDraw {
    uint32_t firstIndex = 0, indexCount = 0;
    size_t material = 0;
    glm::mat4 transform{1};
    glm::vec3 center{0};
};
struct GltfScene {
    std::vector<GltfVertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<GltfDraw> draws;
    std::vector<GltfMaterial> materials;
    std::vector<GltfImage> images;
    std::vector<GltfTexture> textures;
    glm::vec3 boundsMin{0}, boundsMax{0};
    glm::mat4 normalization{1};
};
struct GpuBuffer {
    VmaAllocator allocator = nullptr;
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = nullptr;
    void* mapped = nullptr;
    GpuBuffer() = default;
    ~GpuBuffer();
    GpuBuffer(const GpuBuffer&) = delete;
    GpuBuffer& operator=(const GpuBuffer&) = delete;
    GpuBuffer(GpuBuffer&& other) noexcept;
    GpuBuffer& operator=(GpuBuffer&& other) noexcept;
    void Reset() noexcept;
    void Create(VmaAllocator allocator, VkDeviceSize size, VkBufferUsageFlags usage, bool host = false);
    void Write(const void* bytes, VkDeviceSize size);
};
GltfScene LoadGltf(const std::filesystem::path& path);
GltfScene MakeFallbackBox();
// Staging allocations must outlive the caller's initialization submission fence.
GpuBuffer UploadGltfBuffer(VmaAllocator allocator, VkCommandBuffer commands, std::span<const std::byte> bytes,
                           VkBufferUsageFlags usage, std::vector<GpuBuffer>& staging);
