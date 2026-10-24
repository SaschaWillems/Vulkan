#pragma once
#include "IVideoEncoder.h"
#include <memory>

class VulkanMemoryExporter;
class NvencZeroCopyEncoder final : public IVideoEncoder {
public:
    explicit NvencZeroCopyEncoder(VulkanMemoryExporter& source);
    ~NvencZeroCopyEncoder() override;
    bool Init(uint32_t width, uint32_t height, uint32_t fps, uint32_t bitrate) override;
    std::vector<uint8_t> EncodeFrame(const uint8_t* rgbaPixels) override;
    std::vector<uint8_t> EncodeGpuFrame() override;
    void RequestKeyFrame() override;
    bool UsesGpuFrames() const noexcept override { return true; }
    const char* BackendName() const noexcept override { return "NVENC (CUDA/Vulkan zero host copy)"; }
private:
    VulkanMemoryExporter& source_;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
