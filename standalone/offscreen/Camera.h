#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <memory>
#include <numbers>

struct CameraPose {
    float yaw = 0, pitch = 0, distance = 3;
    glm::vec3 target{0};
};
struct CameraMatrices {
    glm::mat4 view, projection;
    glm::vec3 eye;
};
inline CameraMatrices CameraView(CameraPose pose, float aspect) {
    const float cp = std::cos(pose.pitch);
    const glm::vec3 eye =
        pose.target +
        pose.distance * glm::vec3(cp * std::sin(pose.yaw), std::sin(pose.pitch), cp * std::cos(pose.yaw));
    auto projection = glm::perspective(glm::radians(60.0f), aspect, 0.05f, 100.0f);
    projection[1][1] *= -1;
    return {glm::lookAt(eye, pose.target, glm::vec3(0, 1, 0)), projection, eye};
}
class CameraState {
public:
    static constexpr float minDistance = 1.5f, maxDistance = 12.0f;
    CameraPose Load() const { return *std::atomic_load_explicit(&pose_, std::memory_order_acquire); }
    bool Rotate(double dx, double dy) {
        if (!std::isfinite(dx) || !std::isfinite(dy))
            return false;
        Update([&](CameraPose& pose) {
            pose.yaw = std::remainder(pose.yaw + static_cast<float>(std::clamp(dx, -0.5, 0.5)) *
                                                     std::numbers::pi_v<float>,
                                      2 * std::numbers::pi_v<float>);
            pose.pitch = std::clamp(pose.pitch + static_cast<float>(std::clamp(dy, -0.5, 0.5)) *
                                                     std::numbers::pi_v<float>,
                                    -1.4f, 1.4f);
        });
        return true;
    }
    bool Zoom(double delta) {
        if (!std::isfinite(delta))
            return false;
        Update([&](CameraPose& pose) {
            pose.distance =
                std::clamp(pose.distance * static_cast<float>(std::exp(std::clamp(delta, -1.0, 1.0))),
                           minDistance, maxDistance);
        });
        return true;
    }
    bool Pan(double dx, double dy) {
        if (!std::isfinite(dx) || !std::isfinite(dy))
            return false;
        Update([&](CameraPose& pose) {
            const float cy = std::cos(pose.yaw), sy = std::sin(pose.yaw);
            const float cp = std::cos(pose.pitch), sp = std::sin(pose.pitch);
            const float extent = 2 * pose.distance * std::tan(std::numbers::pi_v<float> / 6);
            pose.target +=
                extent * (-glm::vec3(cy, 0, -sy) * static_cast<float>(std::clamp(dx, -0.5, 0.5)) +
                          glm::vec3(-sp * sy, cp, -sp * cy) * static_cast<float>(std::clamp(dy, -0.5, 0.5)));
            pose.target = glm::clamp(pose.target, glm::vec3(-10), glm::vec3(10));
        });
        return true;
    }
    void Reset() {
        std::atomic_store_explicit(&pose_, std::make_shared<const CameraPose>(), std::memory_order_release);
    }

private:
    template <typename Transform> void Update(Transform transform) {
        auto previous = std::atomic_load_explicit(&pose_, std::memory_order_acquire);
        for (;;) {
            CameraPose changed = *previous;
            transform(changed);
            auto next = std::make_shared<const CameraPose>(changed);
            if (std::atomic_compare_exchange_weak_explicit(&pose_, &previous, next, std::memory_order_acq_rel,
                                                           std::memory_order_acquire))
                return;
        }
    }
    // Free shared_ptr atomic operations also work on Apple libc++ versions without atomic<shared_ptr>.
    // Immutable snapshots keep target, orbit, and distance coherent together.
    std::shared_ptr<const CameraPose> pose_{std::make_shared<const CameraPose>()};
};
