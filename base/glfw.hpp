#pragma once

#include <string>
#include <vector>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

namespace glfw {
    std::vector<const char*> getRequiredInstanceExtensions();
}