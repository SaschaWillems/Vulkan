#pragma once

#include "CameraState.h"
#include "RenderMode.h"
#include "VideoConfig.h"
#include <chrono>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

// Invalid, oversized, or non-finite messages leave the camera untouched.
bool ApplyControlMessage(CameraState& camera, std::string_view message, RenderControls* render = nullptr);

class WebRTCStreamer {
public:
    struct Options {
        uint16_t port = 8080;
        std::string bindAddress = "127.0.0.1";
        std::vector<std::string> iceServers;
        VideoCodec codec = VideoCodec::H264;
        uint32_t h264Level = 51;
        std::shared_ptr<RenderControls> renderControls = std::make_shared<RenderControls>();
    };
    WebRTCStreamer(std::shared_ptr<CameraState> camera, const Options& options);
    ~WebRTCStreamer();
    WebRTCStreamer(const WebRTCStreamer&) = delete;
    WebRTCStreamer& operator=(const WebRTCStreamer&) = delete;

    // Call on the render thread. Network callbacks only queue signaling or update atomics.
    void Poll();
    bool HasViewers() const;
    bool ConsumeKeyFrameRequest();
    void SendFrame(std::span<const uint8_t> encoded, std::chrono::duration<double> captureTime);
    uint16_t Port() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
