#include "AccelerationStructure.h"
#include <algorithm>
#include <cstring>
#include <map>
#include <stdexcept>
#include <utility>

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
VkDeviceAddress Address(VkDevice device, VkBuffer buffer) {
    auto info = Info<VkBufferDeviceAddressInfo>(VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO);
    info.buffer = buffer;
    const auto address = vkGetBufferDeviceAddress(device, &info);
    if (!address)
        throw std::runtime_error("Buffer has no device address");
    return address;
}
void BuildBarrier(VkCommandBuffer commands, VkPipelineStageFlags destination,
                  VkAccessFlags access = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR) {
    auto barrier = Info<VkMemoryBarrier>(VK_STRUCTURE_TYPE_MEMORY_BARRIER);
    barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
    barrier.dstAccessMask = access;
    vkCmdPipelineBarrier(commands, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, destination, 0, 1,
                         &barrier, 0, nullptr, 0, nullptr);
}
struct Submission {
    VkDevice device{};
    VkCommandPool pool{};
    VkFence fence{};
    VkCommandBuffer commands{};
    bool submitted = false;
    ~Submission() {
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
        Check(vkCreateCommandPool(device, &create, nullptr, &pool), "Create AS command pool");
        auto allocate = Info<VkCommandBufferAllocateInfo>(VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO);
        allocate.commandPool = pool;
        allocate.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocate.commandBufferCount = 1;
        Check(vkAllocateCommandBuffers(device, &allocate, &commands), "Allocate AS commands");
        auto begin = Info<VkCommandBufferBeginInfo>(VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO);
        begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        Check(vkBeginCommandBuffer(commands, &begin), "Begin AS commands");
    }
    void Submit(VkQueue queue) {
        Check(vkEndCommandBuffer(commands), "End AS commands");
        auto create = Info<VkFenceCreateInfo>(VK_STRUCTURE_TYPE_FENCE_CREATE_INFO);
        Check(vkCreateFence(device, &create, nullptr, &fence), "Create AS fence");
        auto submit = Info<VkSubmitInfo>(VK_STRUCTURE_TYPE_SUBMIT_INFO);
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &commands;
        Check(vkQueueSubmit(queue, 1, &submit, fence), "Submit AS build");
        submitted = true;
        Check(vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX), "Wait AS build");
        submitted = false;
    }
};
struct Structure {
    VkDevice device{};
    PFN_vkDestroyAccelerationStructureKHR destroy = nullptr;
    GpuBuffer storage;
    VkAccelerationStructureKHR handle{};
    Structure() = default;
    Structure(const Structure&) = delete;
    Structure& operator=(const Structure&) = delete;
    ~Structure() {
        if (handle)
            destroy(device, handle, nullptr);
    }
};
} // namespace
struct AccelerationStructure::Impl {
    VkDevice device{};
    VmaAllocator allocator{};
    PFN_vkCreateAccelerationStructureKHR create{};
    PFN_vkDestroyAccelerationStructureKHR destroy{};
    PFN_vkGetAccelerationStructureBuildSizesKHR sizes{};
    PFN_vkCmdBuildAccelerationStructuresKHR build{};
    PFN_vkGetAccelerationStructureDeviceAddressKHR address{};
    std::vector<std::unique_ptr<Structure>> blas;
    std::unique_ptr<Structure> tlas;
    GpuBuffer scratch, instances;
    VkDeviceAddress scratchAddress{};
    std::vector<VkAccelerationStructureInstanceKHR> instanceData;
    std::vector<glm::mat4> transforms;
    glm::mat4 previousRoot{0};
    bool dirty = false;
    VkAccelerationStructureGeometryKHR instanceGeometry{};
    VkAccelerationStructureBuildGeometryInfoKHR tlasBuild{};
    uint32_t instanceCount = 0;
    std::unique_ptr<Structure> Allocate(VkAccelerationStructureTypeKHR type, VkDeviceSize bytes) {
        auto result = std::make_unique<Structure>();
        result->device = device;
        result->destroy = destroy;
        result->storage.Create(allocator, bytes,
                               VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
                                   VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
        auto info = Info<VkAccelerationStructureCreateInfoKHR>(
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR);
        info.type = type;
        info.size = bytes;
        info.buffer = result->storage.buffer;
        Check(create(device, &info, nullptr, &result->handle), "Create acceleration structure");
        return result;
    }
};
AccelerationStructure::AccelerationStructure() : impl_(std::make_unique<Impl>()) {}
AccelerationStructure::~AccelerationStructure() = default;
VkAccelerationStructureKHR AccelerationStructure::Tlas() const { return impl_->tlas->handle; }
void AccelerationStructure::Initialize(VmaAllocator allocator, VkPhysicalDevice physical, VkDevice device,
                                       VkQueue queue, uint32_t family, const GltfScene& scene,
                                       VkBuffer vertices, VkBuffer indices) {
    auto& state = *impl_;
    state.device = device;
    state.allocator = allocator;
#define LOAD(member, name)                                                                                   \
    state.member = reinterpret_cast<PFN_##name>(vkGetDeviceProcAddr(device, #name));                         \
    if (!state.member)                                                                                       \
    throw std::runtime_error("Missing " #name)
    LOAD(create, vkCreateAccelerationStructureKHR);
    LOAD(destroy, vkDestroyAccelerationStructureKHR);
    LOAD(sizes, vkGetAccelerationStructureBuildSizesKHR);
    LOAD(build, vkCmdBuildAccelerationStructuresKHR);
    LOAD(address, vkGetAccelerationStructureDeviceAddressKHR);
#undef LOAD
    auto limits = Info<VkPhysicalDeviceAccelerationStructurePropertiesKHR>(
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR);
    auto properties = Info<VkPhysicalDeviceProperties2>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2);
    properties.pNext = &limits;
    vkGetPhysicalDeviceProperties2(physical, &properties);
    if (scene.draws.size() > std::min<uint64_t>(0x1000000, limits.maxInstanceCount))
        throw std::runtime_error("Scene exceeds TLAS instance limit");
    state.instanceCount = static_cast<uint32_t>(scene.draws.size());
    const auto vertexAddress = Address(device, vertices), indexAddress = Address(device, indices);
    struct Build {
        VkAccelerationStructureGeometryKHR geometry;
        VkAccelerationStructureBuildGeometryInfoKHR info;
        VkAccelerationStructureBuildSizesInfoKHR sizes;
        uint32_t count;
    };
    std::vector<Build> builds;
    std::map<std::pair<uint32_t, uint32_t>, size_t> cache;
    std::vector<size_t> drawBlas;
    VkDeviceSize scratchBytes = 1;
    for (const auto& draw : scene.draws) {
        const auto key = std::pair{draw.firstIndex, draw.indexCount};
        if (auto found = cache.find(key); found != cache.end()) {
            drawBlas.push_back(found->second);
            continue;
        }
        Build item{};
        item.count = draw.indexCount / 3;
        if (item.count > limits.maxPrimitiveCount)
            throw std::runtime_error("Primitive exceeds BLAS triangle limit");
        item.geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        item.geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        auto& triangles = item.geometry.geometry.triangles;
        triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        triangles.vertexData.deviceAddress = vertexAddress + offsetof(GltfVertex, position);
        triangles.vertexStride = sizeof(GltfVertex);
        triangles.maxVertex = static_cast<uint32_t>(scene.vertices.size() - 1);
        triangles.indexType = VK_INDEX_TYPE_UINT32;
        triangles.indexData.deviceAddress = indexAddress + VkDeviceSize(draw.firstIndex) * sizeof(uint32_t);
        item.info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        item.info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        item.info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        item.info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        item.info.geometryCount = 1;
        item.info.pGeometries = &item.geometry;
        item.sizes.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
        state.sizes(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &item.info, &item.count,
                    &item.sizes);
        scratchBytes = std::max(scratchBytes, item.sizes.buildScratchSize);
        cache.emplace(key, builds.size());
        drawBlas.push_back(builds.size());
        builds.push_back(item);
    }
    state.instances.Create(allocator,
                           VkDeviceSize(state.instanceCount) * sizeof(VkAccelerationStructureInstanceKHR),
                           VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
                               VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                           true);
    state.instanceGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    state.instanceGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    auto& instances = state.instanceGeometry.geometry.instances;
    instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    instances.data.deviceAddress = Address(device, state.instances.buffer);
    auto& tlas = state.tlasBuild;
    tlas.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    tlas.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    tlas.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR |
                 VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
    tlas.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    tlas.geometryCount = 1;
    tlas.pGeometries = &state.instanceGeometry;
    auto tlasSizes = Info<VkAccelerationStructureBuildSizesInfoKHR>(
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR);
    state.sizes(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &tlas, &state.instanceCount,
                &tlasSizes);
    scratchBytes = std::max({scratchBytes, tlasSizes.buildScratchSize, tlasSizes.updateScratchSize});
    const VkDeviceSize alignment = limits.minAccelerationStructureScratchOffsetAlignment;
    state.scratch.Create(allocator, scratchBytes + alignment,
                         VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
    state.scratchAddress = (Address(device, state.scratch.buffer) + alignment - 1) & ~(alignment - 1);
    Submission submission;
    submission.Begin(device, family);
    auto input = Info<VkMemoryBarrier>(VK_STRUCTURE_TYPE_MEMORY_BARRIER);
    input.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    input.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
    vkCmdPipelineBarrier(submission.commands, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &input, 0, nullptr, 0,
                         nullptr);
    for (auto& item : builds) {
        state.blas.push_back(state.Allocate(VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
                                            item.sizes.accelerationStructureSize));
        item.info.pGeometries = &item.geometry;
        item.info.dstAccelerationStructure = state.blas.back()->handle;
        item.info.scratchData.deviceAddress = state.scratchAddress;
        const VkAccelerationStructureBuildRangeInfoKHR range{item.count, 0, 0, 0};
        const auto* ranges = &range;
        state.build(submission.commands, 1, &item.info, &ranges);
        BuildBarrier(submission.commands, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                     VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR |
                         VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR);
    }
    for (size_t i = 0; i < scene.draws.size(); ++i) {
        VkAccelerationStructureInstanceKHR instance{};
        instance.instanceCustomIndex = static_cast<uint32_t>(i);
        instance.mask = 0xff;
        instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        auto info = Info<VkAccelerationStructureDeviceAddressInfoKHR>(
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR);
        info.accelerationStructure = state.blas[drawBlas[i]]->handle;
        instance.accelerationStructureReference = state.address(device, &info);
        state.instanceData.push_back(instance);
        state.transforms.push_back(scene.draws[i].transform);
    }
    state.tlas =
        state.Allocate(VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR, tlasSizes.accelerationStructureSize);
    tlas.dstAccelerationStructure = state.tlas->handle;
    tlas.scratchData.deviceAddress = state.scratchAddress;
    Prepare(scene.normalization);
    const VkAccelerationStructureBuildRangeInfoKHR range{state.instanceCount, 0, 0, 0};
    const auto* ranges = &range;
    state.build(submission.commands, 1, &tlas, &ranges);
    BuildBarrier(submission.commands,
                 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    submission.Submit(queue);
    state.dirty = false;
    tlas.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
    tlas.srcAccelerationStructure = state.tlas->handle;
}
void AccelerationStructure::Prepare(const glm::mat4& root) {
    auto& state = *impl_;
    if (std::memcmp(&root, &state.previousRoot, sizeof(root)) == 0)
        return;
    state.previousRoot = root;
    state.dirty = true;
    for (size_t i = 0; i < state.transforms.size(); ++i) {
        const auto matrix = root * state.transforms[i];
        for (size_t row = 0; row < 3; ++row)
            for (size_t column = 0; column < 4; ++column)
                state.instanceData[i].transform.matrix[row][column] =
                    matrix[static_cast<int>(column)][static_cast<int>(row)];
    }
    state.instances.Write(state.instanceData.data(),
                          state.instanceData.size() * sizeof(VkAccelerationStructureInstanceKHR));
}
void AccelerationStructure::RecordUpdate(VkCommandBuffer commands) {
    auto& state = *impl_;
    if (!state.dirty)
        return;
    // Previous-frame ray reads and builds must finish before an in-place TLAS update/scratch reuse.
    auto barrier = Info<VkMemoryBarrier>(VK_STRUCTURE_TYPE_MEMORY_BARRIER);
    barrier.srcAccessMask =
        VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
    barrier.dstAccessMask =
        VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
    vkCmdPipelineBarrier(commands,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT |
                             VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                         VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &barrier, 0, nullptr,
                         0, nullptr);
    const VkAccelerationStructureBuildRangeInfoKHR range{state.instanceCount, 0, 0, 0};
    const auto* ranges = &range;
    state.build(commands, 1, &state.tlasBuild, &ranges);
    BuildBarrier(commands, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    state.dirty = false;
}
