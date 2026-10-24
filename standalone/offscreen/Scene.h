#pragma once
#include "Camera.h"
#include <glm/gtc/type_ptr.hpp>
#include <array>
#include <algorithm>

namespace scene {
using Matrix = std::array<float, 16>;
inline Matrix MVP(float seconds, CameraPose pose, float aspect) {
    const auto camera = CameraView(pose, aspect);
    const auto model = glm::rotate(glm::mat4(1), seconds * 0.65f, glm::vec3(0, 1, 0));
    const auto matrix = camera.projection * camera.view * model;
    Matrix result{};
    std::copy_n(glm::value_ptr(matrix), 16, result.begin());
    return result;
}
} // namespace scene
