#include "CudaVulkanInterop.h"
#include "CudaColorConversion.h"
#include "VulkanMemoryExporter.h"
#include <cuda.h>
#include <cuda_runtime_api.h>
#include <cstring>
#include <dlfcn.h>
#include <limits>
#include <stdexcept>
#include <string>

namespace {
void Check(cudaError_t result, const char* expression) {
    if (result != cudaSuccess)
        throw std::runtime_error(std::string(expression) + ": " + cudaGetErrorString(result));
}
#define CUDA_CHECK(expression) Check((expression), #expression)
template<typename T> T Load(void* library, const char* name) {
    auto symbol = reinterpret_cast<T>(dlsym(library, name));
    if (!symbol) throw std::runtime_error(std::string("Missing CUDA driver entry point: ") + name);
    return symbol;
}
void DriverCheck(CUresult result, const char* operation) {
    if (result != CUDA_SUCCESS)
        throw std::runtime_error(std::string(operation) + " failed (CUresult " + std::to_string(result) + ")");
}
}

struct CudaVulkanInterop::Impl {
    int device = -1;
    uint32_t width = 0, height = 0;
    void* driver = nullptr;
    decltype(&cuMemFree) freeMemory = nullptr;
    CUcontext context = nullptr;
    CUdeviceptr nv12 = 0;
    size_t pitch = 0;
    cudaStream_t stream = nullptr;
    cudaExternalMemory_t memory = nullptr;
    cudaExternalSemaphore_t ready = nullptr, done = nullptr;
    cudaMipmappedArray_t mipmapped = nullptr;
    cudaSurfaceObject_t surface = 0;
    void* rgba = nullptr;

    ~Impl() {
        if (device >= 0) (void)cudaSetDevice(device);
        if (stream) (void)cudaStreamSynchronize(stream);
        if (nv12 && freeMemory) (void)freeMemory(nv12);
        if (surface) (void)cudaDestroySurfaceObject(surface);
        if (mipmapped) (void)cudaFreeMipmappedArray(mipmapped);
        if (rgba) (void)cudaFree(rgba);
        if (memory) (void)cudaDestroyExternalMemory(memory);
        if (done) (void)cudaDestroyExternalSemaphore(done);
        if (ready) (void)cudaDestroyExternalSemaphore(ready);
        if (stream) (void)cudaStreamDestroy(stream);
        // The primary context belongs to the CUDA runtime; do not reset other CUDA users.
        if (driver) dlclose(driver);
    }
};

CudaVulkanInterop::CudaVulkanInterop() : impl_(std::make_unique<Impl>()) {}
CudaVulkanInterop::~CudaVulkanInterop() = default;

void CudaVulkanInterop::Initialize(const VulkanMemoryExporter& source) {
    impl_ = std::make_unique<Impl>();
    auto& state = *impl_;
    state.width = source.Width(); state.height = source.Height();
    if (!state.width || !state.height || state.width % 2 || state.height % 2)
        throw std::invalid_argument("NV12 requires positive even dimensions");
    int count = 0;
    CUDA_CHECK(cudaGetDeviceCount(&count));
    for (int device = 0; device < count; ++device) {
        cudaDeviceProp properties{};
        CUDA_CHECK(cudaGetDeviceProperties(&properties, device));
        if (std::memcmp(properties.uuid.bytes, source.DeviceUuid().data(), VK_UUID_SIZE) == 0) {
            state.device = device;
            break;
        }
    }
    if (state.device < 0) throw std::runtime_error("No CUDA device matches the Vulkan device UUID");
    CUDA_CHECK(cudaSetDevice(state.device));
    CUDA_CHECK(cudaFree(nullptr)); // Initialize this device's primary runtime context.
    state.driver = dlopen("libcuda.so.1", RTLD_NOW | RTLD_LOCAL);
    if (!state.driver) throw std::runtime_error("Cannot load libcuda.so.1");
    const auto getContext = Load<decltype(&cuCtxGetCurrent)>(state.driver, "cuCtxGetCurrent");
    const auto allocate = Load<decltype(&cuMemAllocPitch)>(state.driver, "cuMemAllocPitch_v2");
    state.freeMemory = Load<decltype(&cuMemFree)>(state.driver, "cuMemFree_v2");
    DriverCheck(getContext(&state.context), "cuCtxGetCurrent");
    if (!state.context) throw std::runtime_error("CUDA runtime did not create a context");
    CUDA_CHECK(cudaStreamCreateWithFlags(&state.stream, cudaStreamNonBlocking));

    auto memoryFd = source.ExportMemoryFd();
    cudaExternalMemoryHandleDesc memory{};
    memory.type = cudaExternalMemoryHandleTypeOpaqueFd;
    memory.handle.fd = memoryFd.Get();
    memory.size = source.AllocationSize();
    memory.flags = cudaExternalMemoryDedicated;
    CUDA_CHECK(cudaImportExternalMemory(&state.memory, &memory));
    (void)memoryFd.Release(); // CUDA owns the FD only after a successful import.
    if (source.Type() == VulkanMemoryExporter::Kind::Image) {
        cudaExternalMemoryMipmappedArrayDesc mapping{};
        mapping.extent = make_cudaExtent(state.width, state.height, 0);
        mapping.formatDesc = cudaCreateChannelDesc<uchar4>();
        mapping.numLevels = 1;
        mapping.flags = cudaArrayColorAttachment | cudaArraySurfaceLoadStore;
        CUDA_CHECK(cudaExternalMemoryGetMappedMipmappedArray(&state.mipmapped, state.memory, &mapping));
        cudaArray_t array = nullptr;
        CUDA_CHECK(cudaGetMipmappedArrayLevel(&array, state.mipmapped, 0));
        cudaResourceDesc resource{};
        resource.resType = cudaResourceTypeArray;
        resource.res.array.array = array;
        CUDA_CHECK(cudaCreateSurfaceObject(&state.surface, &resource));
    } else {
        cudaExternalMemoryBufferDesc mapping{};
        mapping.size = static_cast<unsigned long long>(state.width) * state.height * 4;
        CUDA_CHECK(cudaExternalMemoryGetMappedBuffer(&state.rgba, state.memory, &mapping));
    }
    const auto importSemaphore = [](UniqueFd fd, cudaExternalSemaphore_t& semaphore) {
        cudaExternalSemaphoreHandleDesc handle{};
        handle.type = cudaExternalSemaphoreHandleTypeOpaqueFd;
        handle.handle.fd = fd.Get();
        CUDA_CHECK(cudaImportExternalSemaphore(&semaphore, &handle));
        (void)fd.Release();
    };
    importSemaphore(source.ExportReadyFd(), state.ready);
    importSemaphore(source.ExportDoneFd(), state.done);
    // NVENC requires an encoder-owned, pitched CUDA allocation; the conversion writes here
    // directly from the imported Vulkan memory. There is no host framebuffer allocation.
    DriverCheck(allocate(&state.nv12, &state.pitch, state.width,
        static_cast<size_t>(state.height) * 3 / 2, 16), "cuMemAllocPitch(NV12)");
    if (state.pitch > std::numeric_limits<uint32_t>::max()) throw std::runtime_error("NVENC pitch overflow");
}

void CudaVulkanInterop::ConvertFrame() {
    auto& state = *impl_;
    if (!state.memory || !state.nv12) throw std::logic_error("CUDA interop is not initialized");
    cudaExternalSemaphoreWaitParams wait{};
    CUDA_CHECK(cudaWaitExternalSemaphoresAsync(&state.ready, &wait, 1, state.stream));
    CUDA_CHECK(LaunchRgbaToNv12(static_cast<const uint8_t*>(state.rgba), state.surface,
        state.width, state.height, reinterpret_cast<uint8_t*>(state.nv12), state.pitch, state.stream));
    cudaExternalSemaphoreSignalParams signal{};
    CUDA_CHECK(cudaSignalExternalSemaphoresAsync(&state.done, &signal, 1, state.stream));
    // The source can now be reused by Vulkan. NVENC uses the separate NV12 allocation;
    // synchronization here makes its writes visible before nvEncMapInputResource.
    CUDA_CHECK(cudaStreamSynchronize(state.stream));
}
void* CudaVulkanInterop::Context() const { return impl_->context; }
void* CudaVulkanInterop::Nv12Pointer() const { return reinterpret_cast<void*>(impl_->nv12); }
uint32_t CudaVulkanInterop::Pitch() const { return static_cast<uint32_t>(impl_->pitch); }
