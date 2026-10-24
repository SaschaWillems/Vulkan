#pragma once

#include <algorithm>
#include <atomic>
#include <bit>
#include <cmath>
#include <cstdint>
#include <numbers>

struct CameraPose { float yaw = 0; float pitch = 0; };

class CameraState {
public:
    // One atomic word gives the render thread a coherent pair of angles.
    static_assert(std::atomic<uint64_t>::is_always_lock_free);
    CameraPose Load() const { return Unpack(angles_.load(std::memory_order_relaxed)); }

    bool Rotate(double dx, double dy) {
        if (!std::isfinite(dx) || !std::isfinite(dy)) return false;
        const float yawDelta = static_cast<float>(std::clamp(dx, -0.5, 0.5)) * std::numbers::pi_v<float>;
        const float pitchDelta = static_cast<float>(std::clamp(dy, -0.5, 0.5)) * std::numbers::pi_v<float>;
        uint64_t previous = angles_.load(std::memory_order_relaxed);
        for (;;) {
            auto pose = Unpack(previous);
            pose.yaw = std::remainder(pose.yaw + yawDelta, 2 * std::numbers::pi_v<float>);
            pose.pitch = std::clamp(pose.pitch + pitchDelta, -1.4f, 1.4f);
            if (angles_.compare_exchange_weak(previous, Pack(pose), std::memory_order_relaxed)) return true;
        }
    }

private:
    static uint64_t Pack(CameraPose pose) {
        return uint64_t{std::bit_cast<uint32_t>(pose.yaw)} |
               (uint64_t{std::bit_cast<uint32_t>(pose.pitch)} << 32);
    }
    static CameraPose Unpack(uint64_t value) {
        return {std::bit_cast<float>(static_cast<uint32_t>(value)),
                std::bit_cast<float>(static_cast<uint32_t>(value >> 32))};
    }
    std::atomic<uint64_t> angles_{0};
};
