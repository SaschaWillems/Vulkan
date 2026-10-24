#pragma once

#include "CameraState.h"
#include <array>
#include <cmath>

namespace scene {
using Matrix = std::array<float, 16>;

inline Matrix Multiply(const Matrix& a, const Matrix& b) {
    Matrix result{};
    for (size_t column = 0; column < 4; ++column)
        for (size_t row = 0; row < 4; ++row)
            for (size_t k = 0; k < 4; ++k)
                result[column * 4 + row] += a[k * 4 + row] * b[column * 4 + k];
    return result;
}

inline Matrix MVP(float seconds, CameraPose camera, float aspect) {
    // Column-major matrices match GLSL. The camera looks at the origin from radius 3.
    const float cy = std::cos(camera.yaw), sy = std::sin(camera.yaw);
    const float cp = std::cos(camera.pitch), sp = std::sin(camera.pitch);
    const Matrix view{cy, -sp * sy, cp * sy, 0,
                      0, cp, sp, 0,
                      -sy, -sp * cy, cp * cy, 0,
                      0, 0, -3, 1};
    const float angle = std::remainder(seconds * 0.65f, 2 * std::numbers::pi_v<float>);
    const float c = std::cos(angle), s = std::sin(angle);
    const Matrix model{c, 0, -s, 0, 0, 1, 0, 0, s, 0, c, 0, 0, 0, 0, 1};
    constexpr float nearPlane = 0.1f, farPlane = 100.0f;
    const float f = 1.0f / std::tan(std::numbers::pi_v<float> / 6.0f);
    // Flip Y for Vulkan's positive-height viewport; depth maps into [0,1].
    const Matrix projection{f / aspect, 0, 0, 0, 0, -f, 0, 0,
                            0, 0, farPlane / (nearPlane - farPlane), -1,
                            0, 0, (farPlane * nearPlane) / (nearPlane - farPlane), 0};
    return Multiply(projection, Multiply(view, model));
}
} // namespace scene
