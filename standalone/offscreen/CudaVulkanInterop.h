#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>

class VulkanMemoryExporter;

// Render-thread-only interop. NVENC's registered input must be released before this object.
class CudaVulkanInterop {
public:
    CudaVulkanInterop();
    ~CudaVulkanInterop();
    CudaVulkanInterop(const CudaVulkanInterop&) = delete;
    CudaVulkanInterop& operator=(const CudaVulkanInterop&) = delete;
    void Initialize(const VulkanMemoryExporter& source);
    // Waits on Vulkan's ready semaphore, converts in VRAM, and signals CUDA-done.
    void ConvertFrame();
    void* Context() const;
    void* Nv12Pointer() const;
    uint32_t Pitch() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
