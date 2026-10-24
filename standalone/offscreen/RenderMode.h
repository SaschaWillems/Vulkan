#pragma once
#include "Camera.h"
#include <atomic>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string_view>

enum class RenderMode : uint32_t { Raster = 0, Hybrid = 1, PathTraced = 2 };
inline constexpr std::string_view RenderModeName(RenderMode mode) {
    switch (mode) {
    case RenderMode::Raster:
        return "raster";
    case RenderMode::Hybrid:
        return "hybrid";
    case RenderMode::PathTraced:
        return "pathtrace";
    }
    return "raster";
}
inline constexpr std::optional<RenderMode> ParseRenderMode(std::string_view name) {
    if (name == "raster")
        return RenderMode::Raster;
    if (name == "hybrid")
        return RenderMode::Hybrid;
    if (name == "pathtrace")
        return RenderMode::PathTraced;
    return std::nullopt;
}
struct RenderModeSnapshot {
    RenderMode mode;
    uint64_t revision;
};
class RenderControls {
public:
    void Configure(bool supported, RenderMode initial) {
        if (static_cast<uint32_t>(initial) > 2 || (!supported && initial != RenderMode::Raster))
            throw std::invalid_argument("Initial render mode is unsupported");
        rayQueries_.store(supported);
        state_.store(static_cast<uint64_t>(initial));
    }
    bool SupportsRayQueries() const { return rayQueries_.load(); }
    bool Set(RenderMode mode) {
        if (static_cast<uint32_t>(mode) > 2 || (mode != RenderMode::Raster && !SupportsRayQueries()))
            return false;
        auto previous = state_.load();
        for (;;) {
            if ((previous & 3) == static_cast<uint64_t>(mode))
                return true;
            const auto next = ((previous & ~uint64_t{3}) + 4) | static_cast<uint64_t>(mode);
            if (state_.compare_exchange_weak(previous, next))
                return true;
        }
    }
    RenderModeSnapshot Load() const {
        const auto state = state_.load();
        return {static_cast<RenderMode>(state & 3), state >> 2};
    }
    std::atomic<uint32_t> samples{0};

private:
    std::atomic<bool> rayQueries_{false};
    std::atomic<uint64_t> state_{0};
};
// History is owned by the render thread; revisions catch even move-and-return gestures.
class ProgressiveHistory {
public:
    uint32_t Begin(const CameraPose& camera, RenderModeSnapshot mode, bool animated) {
        if (!valid_ || revision_ != camera.revision || modeRevision_ != mode.revision || mode_ != mode.mode ||
            camera.yaw != camera_.yaw || camera.pitch != camera_.pitch ||
            camera.distance != camera_.distance || camera.target != camera_.target || animated)
            samples_ = 0;
        camera_ = camera;
        revision_ = camera.revision;
        modeRevision_ = mode.revision;
        mode_ = mode.mode;
        valid_ = true;
        return samples_;
    }
    void Complete() {
        if (mode_ == RenderMode::PathTraced && samples_ < 0x00ffffffu)
            ++samples_;
    }
    uint32_t Samples() const { return samples_; }

private:
    CameraPose camera_;
    bool valid_ = false;
    RenderMode mode_ = RenderMode::Raster;
    uint64_t revision_ = 0, modeRevision_ = 0;
    uint32_t samples_ = 0;
};
