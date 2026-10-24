#pragma once

#include "IVideoEncoder.h"
#include <memory>

// Init/EncodeFrame/destruction belong to the render thread; RequestKeyFrame is thread safe.
class VideoToolboxEncoder final : public IVideoEncoder {
public:
    VideoToolboxEncoder();
    ~VideoToolboxEncoder() override;
    VideoToolboxEncoder(const VideoToolboxEncoder&) = delete;
    VideoToolboxEncoder& operator=(const VideoToolboxEncoder&) = delete;

    bool Init(uint32_t width, uint32_t height, uint32_t fps, uint32_t bitrate) override;
    std::vector<uint8_t> EncodeFrame(const uint8_t* rgbaPixels) override;
    void RequestKeyFrame() override;
    const char* BackendName() const noexcept override { return "VideoToolbox (hardware)"; }
    bool IsHardwareAccelerated() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
