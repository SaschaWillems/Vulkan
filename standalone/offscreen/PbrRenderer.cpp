#include "PbrRenderer.h"
#include "GltfLoader.h"
#include "PbrShaders.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstring>
#include <map>
#include <numeric>
#include <stdexcept>
#include <utility>

namespace {
void Check(VkResult result, const char* operation) {
    if (result != VK_SUCCESS)
        throw std::runtime_error(std::string(operation) + " (VkResult " + std::to_string(result) + ")");
}
#define VK_CHECK(call) Check((call), #call)
template <typename T> T Info(VkStructureType type) {
    T result{};
    result.sType = type;
    return result;
}
struct Image {
    VmaAllocator allocator = nullptr;
    VkDevice device = VK_NULL_HANDLE;
    VkImage image = VK_NULL_HANDLE;
    VmaAllocation allocation = nullptr;
    VkImageView view = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
    Image() = default;
    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;
    Image(Image&& other) noexcept { *this = std::move(other); }
    Image& operator=(Image&& other) noexcept {
        if (this != &other) {
            Reset();
            allocator = other.allocator;
            device = other.device;
            image = std::exchange(other.image, VK_NULL_HANDLE);
            allocation = std::exchange(other.allocation, nullptr);
            view = std::exchange(other.view, VK_NULL_HANDLE);
            sampler = std::exchange(other.sampler, VK_NULL_HANDLE);
        }
        return *this;
    }
    ~Image() { Reset(); }
    void Reset() noexcept {
        if (sampler)
            vkDestroySampler(device, sampler, nullptr);
        if (view)
            vkDestroyImageView(device, view, nullptr);
        if (image)
            vmaDestroyImage(allocator, image, allocation);
        image = VK_NULL_HANDLE;
        view = VK_NULL_HANDLE;
        sampler = VK_NULL_HANDLE;
        allocation = nullptr;
    }
    void Create(VmaAllocator owner, VkDevice logical, uint32_t width, uint32_t height, VkFormat format,
                VkImageUsageFlags usage, VkImageAspectFlags aspect, uint32_t levels = 1) {
        allocator = owner;
        device = logical;
        auto info = Info<VkImageCreateInfo>(VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO);
        info.imageType = VK_IMAGE_TYPE_2D;
        info.extent = {width, height, 1};
        info.format = format;
        info.mipLevels = levels;
        info.arrayLayers = 1;
        info.samples = VK_SAMPLE_COUNT_1_BIT;
        info.tiling = VK_IMAGE_TILING_OPTIMAL;
        info.usage = usage;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VmaAllocationCreateInfo memory{};
        memory.usage = VMA_MEMORY_USAGE_AUTO;
        memory.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        VK_CHECK(vmaCreateImage(allocator, &info, &memory, &image, &allocation, nullptr));
        auto imageView = Info<VkImageViewCreateInfo>(VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO);
        imageView.image = image;
        imageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageView.format = format;
        imageView.subresourceRange = {aspect, 0, levels, 0, 1};
        VK_CHECK(vkCreateImageView(device, &imageView, nullptr, &view));
    }
};
struct Upload {
    VkDevice device = VK_NULL_HANDLE;
    VkCommandPool pool = VK_NULL_HANDLE;
    VkCommandBuffer commands = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
    bool submitted = false;
    std::vector<GpuBuffer> staging;
    ~Upload() {
        if (submitted)
            (void)vkDeviceWaitIdle(device);
        if (fence)
            vkDestroyFence(device, fence, nullptr);
        if (pool)
            vkDestroyCommandPool(device, pool, nullptr);
    }
    void Initialize(VkDevice logical, uint32_t family) {
        device = logical;
        auto info = Info<VkCommandPoolCreateInfo>(VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO);
        info.queueFamilyIndex = family;
        info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        VK_CHECK(vkCreateCommandPool(device, &info, nullptr, &pool));
        auto allocation = Info<VkCommandBufferAllocateInfo>(VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO);
        allocation.commandPool = pool;
        allocation.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocation.commandBufferCount = 1;
        VK_CHECK(vkAllocateCommandBuffers(device, &allocation, &commands));
        auto begin = Info<VkCommandBufferBeginInfo>(VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO);
        begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        VK_CHECK(vkBeginCommandBuffer(commands, &begin));
    }
    void Submit(VkQueue queue) {
        VK_CHECK(vkEndCommandBuffer(commands));
        auto create = Info<VkFenceCreateInfo>(VK_STRUCTURE_TYPE_FENCE_CREATE_INFO);
        VK_CHECK(vkCreateFence(device, &create, nullptr, &fence));
        auto submit = Info<VkSubmitInfo>(VK_STRUCTURE_TYPE_SUBMIT_INFO);
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &commands;
        VK_CHECK(vkQueueSubmit(queue, 1, &submit, fence));
        submitted = true;
        VK_CHECK(vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX));
        submitted = false;
    }
};
struct alignas(16) Lighting {
    glm::vec4 eye, direction, radiance;
};
struct alignas(16) Material {
    glm::vec4 base, factors, flags;
};
struct Push {
    glm::mat4 model, viewProjection;
};
static_assert(sizeof(Push) == 128 && sizeof(Material) == 48 && sizeof(Lighting) == 48);
} // namespace

struct PbrRenderer::Impl {
    VkDevice device = VK_NULL_HANDLE;
    VkPhysicalDevice physical = VK_NULL_HANDLE;
    VmaAllocator allocator = nullptr;
    uint32_t width = 0, height = 0;
    VkFormat depthFormat = VK_FORMAT_UNDEFINED;
    Image depth;
    GltfScene scene;
    GpuBuffer vertices, indices, lighting, materials;
    std::vector<Image> textures;
    VkDescriptorSetLayout descriptorLayout = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> descriptors;
    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkShaderModule vert = VK_NULL_HANDLE, frag = VK_NULL_HANDLE;
    VkPipeline opaque = VK_NULL_HANDLE, blend = VK_NULL_HANDLE;
    glm::mat4 viewProjection{1}, root{1};
    glm::vec3 eye{0};
    ~Impl() {
        if (device)
            (void)vkDeviceWaitIdle(device);
        if (blend)
            vkDestroyPipeline(device, blend, nullptr);
        if (opaque)
            vkDestroyPipeline(device, opaque, nullptr);
        if (vert)
            vkDestroyShaderModule(device, vert, nullptr);
        if (frag)
            vkDestroyShaderModule(device, frag, nullptr);
        if (layout)
            vkDestroyPipelineLayout(device, layout, nullptr);
        if (descriptorPool)
            vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        if (descriptorLayout)
            vkDestroyDescriptorSetLayout(device, descriptorLayout, nullptr);
        textures.clear();
        depth.Reset();
        vertices.Reset();
        indices.Reset();
        lighting.Reset();
        materials.Reset();
        if (allocator)
            vmaDestroyAllocator(allocator);
    }
    size_t UploadTexture(const GltfImage& source, const GltfTexture& sampler, bool srgb, Upload& upload) {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(physical, &properties);
        if (source.width > properties.limits.maxImageDimension2D ||
            source.height > properties.limits.maxImageDimension2D)
            throw std::runtime_error("Texture exceeds GPU image dimension limits");
        const auto format = srgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
        VkFormatProperties support{};
        vkGetPhysicalDeviceFormatProperties(physical, format, &support);
        if (!(support.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT))
            throw std::runtime_error("GPU cannot sample RGBA8 textures");
        const VkFormatFeatureFlags blit = VK_FORMAT_FEATURE_BLIT_SRC_BIT | VK_FORMAT_FEATURE_BLIT_DST_BIT |
                                          VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
        const uint32_t levels =
            (support.optimalTilingFeatures & blit) == blit
                ? static_cast<uint32_t>(std::bit_width(std::max(source.width, source.height)))
                : 1u;
        GpuBuffer staging;
        staging.Create(allocator, source.rgba.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true);
        staging.Write(source.rgba.data(), source.rgba.size());
        Image texture;
        texture.Create(allocator, device, source.width, source.height, format,
                       VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                           VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                       VK_IMAGE_ASPECT_COLOR_BIT, levels);
        auto barrier = Info<VkImageMemoryBarrier>(VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER);
        barrier.image = texture.image;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, levels, 0, 1};
        vkCmdPipelineBarrier(upload.commands, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        VkBufferImageCopy copy{};
        copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        copy.imageExtent = {source.width, source.height, 1};
        vkCmdCopyBufferToImage(upload.commands, staging.buffer, texture.image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
        int32_t widthNow = static_cast<int32_t>(source.width),
                heightNow = static_cast<int32_t>(source.height);
        for (uint32_t level = 1; level < levels; ++level) {
            barrier.subresourceRange.baseMipLevel = level - 1;
            barrier.subresourceRange.levelCount = 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            vkCmdPipelineBarrier(upload.commands, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
            VkImageBlit region{};
            region.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, level - 1, 0, 1};
            region.srcOffsets[1] = {widthNow, heightNow, 1};
            region.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, level, 0, 1};
            const int32_t nextWidth = std::max(1, widthNow / 2), nextHeight = std::max(1, heightNow / 2);
            region.dstOffsets[1] = {nextWidth, nextHeight, 1};
            vkCmdBlitImage(upload.commands, texture.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region, VK_FILTER_LINEAR);
            widthNow = nextWidth;
            heightNow = nextHeight;
        }
        for (uint32_t level = 0; level < levels; ++level) {
            barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, level, 1, 0, 1};
            barrier.oldLayout = level + 1 == levels ? VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
                                                    : VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask =
                level + 1 == levels ? VK_ACCESS_TRANSFER_WRITE_BIT : VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            vkCmdPipelineBarrier(upload.commands, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                                 &barrier);
        }
        auto sampling = Info<VkSamplerCreateInfo>(VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO);
        sampling.magFilter = sampler.mag;
        sampling.minFilter = sampler.min;
        sampling.mipmapMode = sampler.mipMode;
        sampling.addressModeU = sampler.wrapU;
        sampling.addressModeV = sampler.wrapV;
        sampling.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampling.maxLod = sampler.mipmapped ? static_cast<float>(levels - 1) : 0.f;
        VK_CHECK(vkCreateSampler(device, &sampling, nullptr, &texture.sampler));
        upload.staging.push_back(std::move(staging));
        textures.push_back(std::move(texture));
        return textures.size() - 1;
    }
};

PbrRenderer::PbrRenderer() : impl_(std::make_unique<Impl>()) {}
PbrRenderer::~PbrRenderer() = default;
VkImageView PbrRenderer::DepthView() const { return impl_->depth.view; }
VkFormat PbrRenderer::DepthFormat() const { return impl_->depthFormat; }

void PbrRenderer::Initialize(VkInstance instance, VkPhysicalDevice physical, VkDevice device, VkQueue queue,
                             uint32_t queueFamily, uint32_t width, uint32_t height,
                             const std::filesystem::path& model) {
    auto& state = *impl_;
    state.device = device;
    state.physical = physical;
    state.width = width;
    state.height = height;
    state.scene = model.empty() ? MakeFallbackBox() : LoadGltf(model);
    VmaAllocatorCreateInfo allocator{};
    allocator.instance = instance;
    allocator.physicalDevice = physical;
    allocator.device = device;
    allocator.vulkanApiVersion = VK_API_VERSION_1_1;
    VK_CHECK(vmaCreateAllocator(&allocator, &state.allocator));
    for (auto format : {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D16_UNORM}) {
        VkFormatProperties support{};
        vkGetPhysicalDeviceFormatProperties(physical, format, &support);
        if (support.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            state.depthFormat = format;
            break;
        }
    }
    if (state.depthFormat == VK_FORMAT_UNDEFINED)
        throw std::runtime_error("GPU has no supported depth format");
    state.depth.Create(state.allocator, device, width, height, state.depthFormat,
                       VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
    Upload upload;
    upload.Initialize(device, queueFamily);
    state.vertices =
        UploadGltfBuffer(state.allocator, upload.commands, std::as_bytes(std::span(state.scene.vertices)),
                         VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, upload.staging);
    state.indices =
        UploadGltfBuffer(state.allocator, upload.commands, std::as_bytes(std::span(state.scene.indices)),
                         VK_BUFFER_USAGE_INDEX_BUFFER_BIT, upload.staging);
    state.lighting.Create(state.allocator, sizeof(Lighting), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, true);
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physical, &properties);
    const VkDeviceSize alignment = properties.limits.minUniformBufferOffsetAlignment;
    const VkDeviceSize stride = (sizeof(Material) + alignment - 1) / alignment * alignment;
    if (state.scene.materials.size() > UINT32_MAX / 2)
        throw std::runtime_error("Too many materials");
    const auto count = static_cast<uint32_t>(state.scene.materials.size());
    state.materials.Create(state.allocator, stride * count, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, true);
    const std::array<VkDescriptorSetLayoutBinding, 4> bindings{
        {{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
         {1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
         {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
         {3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}}};
    auto layout = Info<VkDescriptorSetLayoutCreateInfo>(VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO);
    layout.bindingCount = static_cast<uint32_t>(bindings.size());
    layout.pBindings = bindings.data();
    VK_CHECK(vkCreateDescriptorSetLayout(device, &layout, nullptr, &state.descriptorLayout));
    const VkDescriptorPoolSize sizes[] = {{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, count * 2},
                                          {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, count * 2}};
    auto pool = Info<VkDescriptorPoolCreateInfo>(VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO);
    pool.poolSizeCount = 2;
    pool.pPoolSizes = sizes;
    pool.maxSets = count;
    VK_CHECK(vkCreateDescriptorPool(device, &pool, nullptr, &state.descriptorPool));
    std::vector<VkDescriptorSetLayout> layouts(count, state.descriptorLayout);
    state.descriptors.resize(count);
    auto allocation = Info<VkDescriptorSetAllocateInfo>(VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO);
    allocation.descriptorPool = state.descriptorPool;
    allocation.descriptorSetCount = count;
    allocation.pSetLayouts = layouts.data();
    VK_CHECK(vkAllocateDescriptorSets(device, &allocation, state.descriptors.data()));
    const auto whiteColor = state.UploadTexture({}, {}, true, upload),
               whiteLinear = state.UploadTexture({}, {}, false, upload);
    std::map<std::pair<size_t, bool>, size_t> cache;
    const auto texture = [&](std::optional<size_t> index, bool srgb) {
        if (!index)
            return srgb ? whiteColor : whiteLinear;
        const auto key = std::make_pair(*index, srgb);
        if (auto it = cache.find(key); it != cache.end())
            return it->second;
        const auto& source = state.scene.textures.at(*index);
        const auto uploaded = state.UploadTexture(state.scene.images.at(source.image), source, srgb, upload);
        cache.emplace(key, uploaded);
        return uploaded;
    };
    for (uint32_t i = 0; i < count; ++i) {
        const auto& source = state.scene.materials[i];
        const Material material{
            source.baseColor,
            glm::vec4(source.metallic, source.roughness, source.alphaCutoff, source.alphaMode),
            glm::vec4(source.doubleSided ? 1.f : 0.f, 0, 0, 0)};
        std::memcpy(static_cast<std::byte*>(state.materials.mapped) + stride * i, &material,
                    sizeof(material));
        const auto base = texture(source.baseTexture, true),
                   mr = texture(source.metallicRoughnessTexture, false);
        const VkDescriptorBufferInfo buffers[] = {{state.lighting.buffer, 0, sizeof(Lighting)},
                                                  {state.materials.buffer, stride * i, sizeof(Material)}};
        const VkDescriptorImageInfo images[] = {
            {state.textures[base].sampler, state.textures[base].view,
             VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
            {state.textures[mr].sampler, state.textures[mr].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}};
        std::array<VkWriteDescriptorSet, 4> writes{};
        for (uint32_t binding = 0; binding < 4; ++binding) {
            writes[binding] = Info<VkWriteDescriptorSet>(VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET);
            writes[binding].dstSet = state.descriptors[i];
            writes[binding].dstBinding = binding;
            writes[binding].descriptorCount = 1;
            writes[binding].descriptorType =
                binding < 2 ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            if (binding < 2)
                writes[binding].pBufferInfo = &buffers[binding];
            else
                writes[binding].pImageInfo = &images[binding - 2];
        }
        vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }
    VK_CHECK(vmaFlushAllocation(state.allocator, state.materials.allocation, 0, VK_WHOLE_SIZE));
    upload.Submit(queue);
    state.scene.vertices.clear();
    state.scene.vertices.shrink_to_fit();
    state.scene.indices.clear();
    state.scene.indices.shrink_to_fit();
    state.scene.images.clear();
}

void PbrRenderer::CreatePipeline(VkRenderPass renderPass) {
    auto& state = *impl_;
    const auto module = [&](auto& code, VkShaderModule& output) {
        auto info = Info<VkShaderModuleCreateInfo>(VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO);
        info.codeSize = sizeof(code);
        info.pCode = code;
        VK_CHECK(vkCreateShaderModule(state.device, &info, nullptr, &output));
    };
    module(shaders::VERT, state.vert);
    module(shaders::FRAG, state.frag);
    const VkPushConstantRange push{VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Push)};
    auto layout = Info<VkPipelineLayoutCreateInfo>(VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO);
    layout.setLayoutCount = 1;
    layout.pSetLayouts = &state.descriptorLayout;
    layout.pushConstantRangeCount = 1;
    layout.pPushConstantRanges = &push;
    VK_CHECK(vkCreatePipelineLayout(state.device, &layout, nullptr, &state.layout));
    std::array<VkPipelineShaderStageCreateInfo, 2> stages{};
    for (auto& stage : stages) {
        stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stage.pName = "main";
    }
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = state.vert;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = state.frag;
    const VkVertexInputBindingDescription binding{0, sizeof(GltfVertex), VK_VERTEX_INPUT_RATE_VERTEX};
    const VkVertexInputAttributeDescription attributes[] = {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(GltfVertex, position)},
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(GltfVertex, normal)},
        {2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(GltfVertex, uv)},
        {3, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(GltfVertex, color)},
        {4, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(GltfVertex, mrUv)}};
    auto vertices =
        Info<VkPipelineVertexInputStateCreateInfo>(VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO);
    vertices.vertexBindingDescriptionCount = 1;
    vertices.pVertexBindingDescriptions = &binding;
    vertices.vertexAttributeDescriptionCount = 5;
    vertices.pVertexAttributeDescriptions = attributes;
    auto assembly = Info<VkPipelineInputAssemblyStateCreateInfo>(
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO);
    assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    const VkViewport viewport{0, 0, static_cast<float>(state.width), static_cast<float>(state.height), 0, 1};
    const VkRect2D scissor{{0, 0}, {state.width, state.height}};
    auto viewportState =
        Info<VkPipelineViewportStateCreateInfo>(VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO);
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;
    auto raster = Info<VkPipelineRasterizationStateCreateInfo>(
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO);
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.cullMode = VK_CULL_MODE_NONE;
    raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    raster.lineWidth = 1;
    auto sampling =
        Info<VkPipelineMultisampleStateCreateInfo>(VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO);
    sampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    auto depth = Info<VkPipelineDepthStencilStateCreateInfo>(
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO);
    depth.depthTestEnable = VK_TRUE;
    depth.depthWriteEnable = VK_TRUE;
    depth.depthCompareOp = VK_COMPARE_OP_LESS;
    VkPipelineColorBlendAttachmentState attachment{};
    attachment.colorWriteMask = 0xf;
    attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    attachment.colorBlendOp = VK_BLEND_OP_ADD;
    attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    attachment.alphaBlendOp = VK_BLEND_OP_ADD;
    auto blending =
        Info<VkPipelineColorBlendStateCreateInfo>(VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO);
    blending.attachmentCount = 1;
    blending.pAttachments = &attachment;
    auto pipeline = Info<VkGraphicsPipelineCreateInfo>(VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO);
    pipeline.stageCount = 2;
    pipeline.pStages = stages.data();
    pipeline.pVertexInputState = &vertices;
    pipeline.pInputAssemblyState = &assembly;
    pipeline.pViewportState = &viewportState;
    pipeline.pRasterizationState = &raster;
    pipeline.pMultisampleState = &sampling;
    pipeline.pDepthStencilState = &depth;
    pipeline.pColorBlendState = &blending;
    pipeline.layout = state.layout;
    pipeline.renderPass = renderPass;
    VK_CHECK(vkCreateGraphicsPipelines(state.device, VK_NULL_HANDLE, 1, &pipeline, nullptr, &state.opaque));
    attachment.blendEnable = VK_TRUE;
    depth.depthWriteEnable = VK_FALSE;
    VK_CHECK(vkCreateGraphicsPipelines(state.device, VK_NULL_HANDLE, 1, &pipeline, nullptr, &state.blend));
}

void PbrRenderer::PrepareFrame(CameraPose pose, float seconds, bool spin) {
    auto& state = *impl_;
    const auto camera = CameraView(pose, static_cast<float>(state.width) / static_cast<float>(state.height));
    state.viewProjection = camera.projection * camera.view;
    state.eye = camera.eye;
    state.root = glm::rotate(glm::mat4(1), spin ? seconds * 0.65f : 0.f, glm::vec3(0, 1, 0)) *
                 state.scene.normalization;
    const Lighting lighting{glm::vec4(camera.eye, 1), glm::vec4(-0.4f, 0.8f, 0.6f, 0),
                            glm::vec4(3.5f, 3.5f, 3.5f, 0)};
    state.lighting.Write(&lighting, sizeof(lighting));
}
void PbrRenderer::Draw(VkCommandBuffer commands) {
    const auto& state = *impl_;
    const VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commands, 0, 1, &state.vertices.buffer, &offset);
    vkCmdBindIndexBuffer(commands, state.indices.buffer, 0, VK_INDEX_TYPE_UINT32);
    std::vector<size_t> order(state.scene.draws.size());
    std::iota(order.begin(), order.end(), 0);
    std::stable_sort(order.begin(), order.end(), [&](size_t a, size_t b) {
        const auto& left = state.scene.draws[a];
        const auto& right = state.scene.draws[b];
        const bool transparentA = state.scene.materials[left.material].alphaMode == 2,
                   transparentB = state.scene.materials[right.material].alphaMode == 2;
        if (transparentA != transparentB)
            return !transparentA;
        if (!transparentA)
            return false;
        const auto pa = glm::vec3(state.root * left.transform * glm::vec4(left.center, 1)) - state.eye;
        const auto pb = glm::vec3(state.root * right.transform * glm::vec4(right.center, 1)) - state.eye;
        return glm::dot(pa, pa) > glm::dot(pb, pb);
    });
    for (auto index : order) {
        const auto& draw = state.scene.draws[index];
        vkCmdBindPipeline(commands, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          state.scene.materials[draw.material].alphaMode == 2 ? state.blend : state.opaque);
        vkCmdBindDescriptorSets(commands, VK_PIPELINE_BIND_POINT_GRAPHICS, state.layout, 0, 1,
                                &state.descriptors[draw.material], 0, nullptr);
        const Push push{state.root * draw.transform, state.viewProjection};
        vkCmdPushConstants(commands, state.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(push), &push);
        vkCmdDrawIndexed(commands, draw.indexCount, 1, draw.firstIndex, 0, 0);
    }
}
