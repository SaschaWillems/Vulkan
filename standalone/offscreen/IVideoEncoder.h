#pragma once

#include <cstdint>
#include <vector>
#include <stdexcept>

class IVideoEncoder {
public:
    virtual ~IVideoEncoder() = default;
    virtual bool Init(uint32_t width, uint32_t height, uint32_t fps, uint32_t bitrate) = 0;
    // One tightly packed RGBA frame in; one complete Annex-B access unit out.
    // Empty output means the encoder dropped the frame. Encoding failures throw.
    virtual std::vector<uint8_t> EncodeFrame(const uint8_t* rgbaPixels) = 0;
    virtual void RequestKeyFrame() = 0;
    virtual const char* BackendName() const noexcept { return "unspecified"; }
    virtual bool UsesGpuFrames() const noexcept { return false; }
    // GPU encoders bind an exported frame source at initialization. CPU pixels are never
    // passed through this entry point, and only the compressed bitstream returns to the CPU.
    virtual std::vector<uint8_t> EncodeGpuFrame() {
        throw std::logic_error("This encoder requires CPU RGBA input");
    }
};
