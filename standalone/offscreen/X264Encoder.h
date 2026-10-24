#pragma once
#include "IVideoEncoder.h"
#include <memory>

class X264Encoder final : public IVideoEncoder {
public:
    X264Encoder();
    ~X264Encoder() override;
    bool Init(uint32_t width, uint32_t height, uint32_t fps, uint32_t bitrate) override;
    std::vector<uint8_t> EncodeFrame(const uint8_t* rgbaPixels) override;
    void RequestKeyFrame() override;
    const char* BackendName() const noexcept override { return "x264 (CPU)"; }
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
