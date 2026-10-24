#include "RayTracingRenderer.h"
#include "AccelerationStructure.h"
#include "PbrShaders.h"
#include <array>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace {
void Check(VkResult result, const char* operation) {
    if (result != VK_SUCCESS)
        throw std::runtime_error(std::string(operation) + ": " + std::to_string(result));
}
template <typename T> T Info(VkStructureType type) {
    T value{};
    value.sType = type;
    return value;
}
struct alignas(16) Vertex {
    glm::vec4 position, normal, uv, color;
};
struct alignas(16) Draw {
    glm::mat4 model;
    glm::uvec4 info;
};
struct alignas(16) Material {
    glm::vec4 base, factors, flags;
    glm::uvec4 baseImage, mrImage;
};
struct alignas(16) Frame {
    glm::mat4 inverseViewProjection, root;
    glm::vec4 eye, light, radiance;
    glm::uvec4 state;
};
static_assert(sizeof(Vertex) == 64 && sizeof(Draw) == 80 && sizeof(Material) == 80 && sizeof(Frame) == 192);
struct SceneUpload {
    VkDevice device{};
    VkCommandPool pool{};
    VkCommandBuffer commands{};
    VkFence fence{};
    std::vector<GpuBuffer> staging;
    bool submitted = false;
    ~SceneUpload() {
        if (submitted)
            (void)vkDeviceWaitIdle(device);
        if (fence)
            vkDestroyFence(device, fence, nullptr);
        if (pool)
            vkDestroyCommandPool(device, pool, nullptr);
    }
    void Begin(VkDevice logical, uint32_t family) {
        device = logical;
        auto create = Info<VkCommandPoolCreateInfo>(VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO);
        create.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        create.queueFamilyIndex = family;
        Check(vkCreateCommandPool(device, &create, nullptr, &pool), "Create RT upload pool");
        auto allocate = Info<VkCommandBufferAllocateInfo>(VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO);
        allocate.commandPool = pool;
        allocate.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocate.commandBufferCount = 1;
        Check(vkAllocateCommandBuffers(device, &allocate, &commands), "Allocate RT upload commands");
        auto begin = Info<VkCommandBufferBeginInfo>(VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO);
        begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        Check(vkBeginCommandBuffer(commands, &begin), "Begin RT upload");
    }
    void Submit(VkQueue queue) {
        Check(vkEndCommandBuffer(commands), "End RT upload");
        auto create = Info<VkFenceCreateInfo>(VK_STRUCTURE_TYPE_FENCE_CREATE_INFO);
        Check(vkCreateFence(device, &create, nullptr, &fence), "Create RT upload fence");
        auto submit = Info<VkSubmitInfo>(VK_STRUCTURE_TYPE_SUBMIT_INFO);
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &commands;
        Check(vkQueueSubmit(queue, 1, &submit, fence), "Submit RT upload");
        submitted = true;
        Check(vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX), "Wait RT upload");
        submitted = false;
    }
};
struct StorageImage {
    StorageImage() = default;
    StorageImage(const StorageImage&) = delete;
    StorageImage& operator=(const StorageImage&) = delete;
    VmaAllocator allocator{};
    VkDevice device{};
    VkImage image{};
    VmaAllocation allocation{};
    VkImageView view{};
    ~StorageImage() {
        if (view)
            vkDestroyImageView(device, view, nullptr);
        if (image)
            vmaDestroyImage(allocator, image, allocation);
    }
    void Create(VmaAllocator owner, VkDevice logical, uint32_t width, uint32_t height, VkFormat format,
                bool copy) {
        allocator = owner;
        device = logical;
        auto info = Info<VkImageCreateInfo>(VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO);
        info.imageType = VK_IMAGE_TYPE_2D;
        info.extent = {width, height, 1};
        info.mipLevels = 1;
        info.arrayLayers = 1;
        info.format = format;
        info.samples = VK_SAMPLE_COUNT_1_BIT;
        info.tiling = VK_IMAGE_TILING_OPTIMAL;
        info.usage = VK_IMAGE_USAGE_STORAGE_BIT |
                     (copy ? VkImageUsageFlags{VK_IMAGE_USAGE_TRANSFER_SRC_BIT} : VkImageUsageFlags{});
        VmaAllocationCreateInfo memory{};
        memory.usage = VMA_MEMORY_USAGE_AUTO;
        memory.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        Check(vmaCreateImage(allocator, &info, &memory, &image, &allocation, nullptr), "Create RT image");
        auto create = Info<VkImageViewCreateInfo>(VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO);
        create.image = image;
        create.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create.format = format;
        create.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        Check(vkCreateImageView(device, &create, nullptr, &view), "Create RT image view");
    }
};
VkImageMemoryBarrier ImageBarrier(VkImage image, VkImageLayout before, VkImageLayout after,
                                  VkAccessFlags source, VkAccessFlags destination) {
    auto result = Info<VkImageMemoryBarrier>(VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER);
    result.image = image;
    result.oldLayout = before;
    result.newLayout = after;
    result.srcAccessMask = source;
    result.dstAccessMask = destination;
    result.srcQueueFamilyIndex = result.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    result.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    return result;
}
} // namespace
struct RayTracingRenderer::Impl {
    VkDevice device{};
    VmaAllocator allocator{};
    uint32_t width{}, height{};
    AccelerationStructure acceleration;
    GpuBuffer vertices, indices, draws, materials, frame, texels;
    StorageImage accumulation, output;
    VkDescriptorSetLayout descriptorLayout{};
    VkDescriptorPool pool{};
    VkDescriptorSet descriptors{};
    VkPipelineLayout layout{};
    VkPipeline pipeline{};
    VkShaderModule shader{};
    bool imagesInitialized = false;
    ~Impl() {
        if (pipeline)
            vkDestroyPipeline(device, pipeline, nullptr);
        if (shader)
            vkDestroyShaderModule(device, shader, nullptr);
        if (layout)
            vkDestroyPipelineLayout(device, layout, nullptr);
        if (pool)
            vkDestroyDescriptorPool(device, pool, nullptr);
        if (descriptorLayout)
            vkDestroyDescriptorSetLayout(device, descriptorLayout, nullptr);
    }
    template <typename T>
    void Buffer(GpuBuffer& buffer, const std::vector<T>& data, VkBufferUsageFlags usage,
                SceneUpload& upload) {
        const VkDeviceSize bytes = data.size() * sizeof(T);
        VkPhysicalDeviceProperties properties{};
        VmaAllocatorInfo owner{};
        vmaGetAllocatorInfo(allocator, &owner);
        vkGetPhysicalDeviceProperties(owner.physicalDevice, &properties);
        if (!bytes || bytes > properties.limits.maxStorageBufferRange)
            throw std::runtime_error("RT scene buffer exceeds storage-buffer range");
        buffer = UploadGltfBuffer(allocator, upload.commands, std::as_bytes(std::span(data)), usage,
                                  upload.staging);
    }
};
RayTracingRenderer::RayTracingRenderer() : impl_(std::make_unique<Impl>()) {}
RayTracingRenderer::~RayTracingRenderer() = default;
VkDescriptorSetLayout RayTracingRenderer::DescriptorLayout() const { return impl_->descriptorLayout; }
void RayTracingRenderer::Initialize(VmaAllocator allocator, VkPhysicalDevice physical, VkDevice device,
                                    VkQueue queue, uint32_t family, uint32_t width, uint32_t height,
                                    const GltfScene& scene, VkBuffer vertices, VkBuffer indices) {
    auto& s = *impl_;
    s.device = device;
    s.allocator = allocator;
    s.width = width;
    s.height = height;
    s.acceleration.Initialize(allocator, physical, device, queue, family, scene, vertices, indices);
    std::vector<Vertex> packedVertices;
    packedVertices.reserve(scene.vertices.size());
    for (const auto& vertex : scene.vertices)
        packedVertices.push_back({glm::vec4(vertex.position, 1), glm::vec4(vertex.normal, 0),
                                  glm::vec4(vertex.uv, vertex.mrUv), vertex.color});
    std::vector<Draw> packedDraws;
    for (const auto& draw : scene.draws)
        packedDraws.push_back({draw.transform, glm::uvec4(draw.firstIndex, draw.material, 0, 0)});
    std::vector<uint32_t> pixels{0xffffffffu};
    std::vector<uint32_t> offsets(scene.images.size(), UINT32_MAX);
    const auto imageInfo = [&](std::optional<size_t> index) -> glm::uvec4 {
        if (!index)
            return {0, 1, 1, 0};
        const auto& texture = scene.textures.at(*index);
        const auto& image = scene.images.at(texture.image);
        if (offsets.at(texture.image) == UINT32_MAX) {
            if (pixels.size() + image.rgba.size() / 4 > UINT32_MAX)
                throw std::runtime_error("RT texture table exceeds 32-bit indexing");
            offsets[texture.image] = static_cast<uint32_t>(pixels.size());
            for (size_t i = 0; i < image.rgba.size(); i += 4)
                pixels.push_back(uint32_t(image.rgba[i]) | uint32_t(image.rgba[i + 1]) << 8 |
                                 uint32_t(image.rgba[i + 2]) << 16 | uint32_t(image.rgba[i + 3]) << 24);
        }

        const auto wrap = [](VkSamplerAddressMode mode) -> uint32_t {
            return mode == VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE     ? 1u
                   : mode == VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT ? 2u
                                                                     : 0u;
        };
        return {offsets.at(texture.image), image.width, image.height,
                wrap(texture.wrapU) | (wrap(texture.wrapV) << 2) |
                    (texture.mag == VK_FILTER_NEAREST ? 16u : 0u)};
    };
    std::vector<Material> packedMaterials;
    for (const auto& m : scene.materials)
        packedMaterials.push_back({m.baseColor,
                                   glm::vec4(m.metallic, m.roughness, m.alphaCutoff, m.alphaMode),
                                   glm::vec4(m.doubleSided ? 1.f : 0.f, 0, 0, 0), imageInfo(m.baseTexture),
                                   imageInfo(m.metallicRoughnessTexture)});
    SceneUpload upload;
    upload.Begin(device, family);
    s.Buffer(s.vertices, packedVertices, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, upload);
    s.Buffer(s.indices, scene.indices, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, upload);
    s.Buffer(s.draws, packedDraws, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, upload);
    s.Buffer(s.materials, packedMaterials, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, upload);
    s.Buffer(s.texels, pixels, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, upload);
    upload.Submit(queue);
    s.frame.Create(allocator, sizeof(Frame), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, true);
    s.accumulation.Create(allocator, device, width, height, VK_FORMAT_R32G32B32A32_SFLOAT, false);
    s.output.Create(allocator, device, width, height, VK_FORMAT_R8G8B8A8_UNORM, true);
    std::array<VkDescriptorSetLayoutBinding, 9> bindings{};
    for (uint32_t i = 0; i < bindings.size(); ++i) {
        bindings[i].binding = i;
        bindings[i].descriptorCount = 1;
        bindings[i].stageFlags =
            i >= 7 ? VK_SHADER_STAGE_COMPUTE_BIT : VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
        bindings[i].descriptorType = i == 0   ? VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR
                                     : i == 5 ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
                                     : i >= 7 ? VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
                                              : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    }
    auto create = Info<VkDescriptorSetLayoutCreateInfo>(VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO);
    create.bindingCount = static_cast<uint32_t>(bindings.size());
    create.pBindings = bindings.data();
    Check(vkCreateDescriptorSetLayout(device, &create, nullptr, &s.descriptorLayout),
          "Create RT descriptor layout");
    const VkDescriptorPoolSize poolSizes[] = {{VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1},
                                              {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 5},
                                              {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
                                              {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 2}};
    auto pool = Info<VkDescriptorPoolCreateInfo>(VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO);
    pool.maxSets = 1;
    pool.poolSizeCount = 4;
    pool.pPoolSizes = poolSizes;
    Check(vkCreateDescriptorPool(device, &pool, nullptr, &s.pool), "Create RT descriptor pool");
    auto allocate = Info<VkDescriptorSetAllocateInfo>(VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO);
    allocate.descriptorPool = s.pool;
    allocate.descriptorSetCount = 1;
    allocate.pSetLayouts = &s.descriptorLayout;
    Check(vkAllocateDescriptorSets(device, &allocate, &s.descriptors), "Allocate RT descriptors");
    const auto tlas = s.acceleration.Tlas();
    auto asWrite = Info<VkWriteDescriptorSetAccelerationStructureKHR>(
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR);
    asWrite.accelerationStructureCount = 1;
    asWrite.pAccelerationStructures = &tlas;
    const std::array buffers{s.vertices.buffer,  s.indices.buffer, s.draws.buffer,
                             s.materials.buffer, s.frame.buffer,   s.texels.buffer};
    std::array<VkDescriptorBufferInfo, 6> bufferInfo{};
    const VkDescriptorImageInfo images[] = {{VK_NULL_HANDLE, s.accumulation.view, VK_IMAGE_LAYOUT_GENERAL},
                                            {VK_NULL_HANDLE, s.output.view, VK_IMAGE_LAYOUT_GENERAL}};
    std::array<VkWriteDescriptorSet, 9> writes{};
    for (uint32_t i = 0; i < writes.size(); ++i) {
        auto& write = writes[i];
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = s.descriptors;
        write.dstBinding = i;
        write.descriptorCount = 1;
        write.descriptorType = bindings[i].descriptorType;
        if (i == 0)
            write.pNext = &asWrite;
        else if (i < 7) {
            bufferInfo[i - 1] = {buffers[i - 1], 0, VK_WHOLE_SIZE};
            write.pBufferInfo = &bufferInfo[i - 1];
        } else
            write.pImageInfo = &images[i - 7];
    }
    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    // Set zero is intentionally empty: graphics uses the same RT table at set one.
    auto empty = Info<VkDescriptorSetLayoutCreateInfo>(VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO);
    VkDescriptorSetLayout emptyLayout{};
    Check(vkCreateDescriptorSetLayout(device, &empty, nullptr, &emptyLayout), "Create empty compute layout");
    const VkDescriptorSetLayout layouts[] = {emptyLayout, s.descriptorLayout};
    auto pipelineLayout = Info<VkPipelineLayoutCreateInfo>(VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO);
    pipelineLayout.setLayoutCount = 2;
    pipelineLayout.pSetLayouts = layouts;
    const auto layoutResult = vkCreatePipelineLayout(device, &pipelineLayout, nullptr, &s.layout);
    vkDestroyDescriptorSetLayout(device, emptyLayout, nullptr);
    Check(layoutResult, "Create path pipeline layout");
    auto module = Info<VkShaderModuleCreateInfo>(VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO);
    module.codeSize = sizeof(shaders::PATH);
    module.pCode = shaders::PATH;
    Check(vkCreateShaderModule(device, &module, nullptr, &s.shader), "Create path shader");
    auto pipeline = Info<VkComputePipelineCreateInfo>(VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO);
    pipeline.layout = s.layout;
    pipeline.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipeline.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipeline.stage.module = s.shader;
    pipeline.stage.pName = "main";
    Check(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipeline, nullptr, &s.pipeline),
          "Create path pipeline");
}
void RayTracingRenderer::Prepare(const glm::mat4& root, const CameraMatrices& camera, RenderMode mode,
                                 uint32_t samples) {
    auto& s = *impl_;
    s.acceleration.Prepare(root);
    const Frame frame{glm::inverse(camera.projection * camera.view),
                      root,
                      glm::vec4(camera.eye, 1),
                      glm::vec4(-0.4f, 0.8f, 0.6f, 0),
                      glm::vec4(3.5f, 3.5f, 3.5f, 0),
                      glm::uvec4(static_cast<uint32_t>(mode), samples, s.width, s.height)};
    s.frame.Write(&frame, sizeof(frame));
}
void RayTracingRenderer::RecordUpdate(VkCommandBuffer commands) {
    impl_->acceleration.RecordUpdate(commands);
}
void RayTracingRenderer::BindGraphics(VkCommandBuffer commands, VkPipelineLayout layout) {
    vkCmdBindDescriptorSets(commands, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 1, 1, &impl_->descriptors, 0,
                            nullptr);
}
void RayTracingRenderer::Trace(VkCommandBuffer commands, VkImage target) {
    auto& s = *impl_;
    const VkImageMemoryBarrier before[] = {
        ImageBarrier(s.accumulation.image,
                     s.imagesInitialized ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_UNDEFINED,
                     VK_IMAGE_LAYOUT_GENERAL, s.imagesInitialized ? VK_ACCESS_SHADER_WRITE_BIT : 0,
                     VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT),
        ImageBarrier(s.output.image,
                     s.imagesInitialized ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_UNDEFINED,
                     VK_IMAGE_LAYOUT_GENERAL, 0, VK_ACCESS_SHADER_WRITE_BIT)};
    vkCmdPipelineBarrier(commands,
                         s.imagesInitialized ? VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
                                             : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 2, before);
    vkCmdBindPipeline(commands, VK_PIPELINE_BIND_POINT_COMPUTE, s.pipeline);
    vkCmdBindDescriptorSets(commands, VK_PIPELINE_BIND_POINT_COMPUTE, s.layout, 1, 1, &s.descriptors, 0,
                            nullptr);
    vkCmdDispatch(commands, (s.width + 7) / 8, (s.height + 7) / 8, 1);
    auto copy = ImageBarrier(s.output.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                             VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT);
    vkCmdPipelineBarrier(commands, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
                         nullptr, 0, nullptr, 1, &copy);
    VkImageCopy region{};
    region.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    region.dstSubresource = region.srcSubresource;
    region.extent = {s.width, s.height, 1};
    vkCmdCopyImage(commands, s.output.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, target,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    auto restore = ImageBarrier(s.output.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
                                VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT);
    vkCmdPipelineBarrier(commands, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0,
                         nullptr, 0, nullptr, 1, &restore);
    s.imagesInitialized = true;
}
