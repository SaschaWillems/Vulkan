#include "glfw.hpp"

namespace glfw {
    std::vector<const char*> getRequiredInstanceExtensions() {
        std::vector<const char*> result;
        uint32_t count = 0;
        const char** names = glfwGetRequiredInstanceExtensions(&count);
        if (names && count) {
            for (uint32_t i = 0; i < count; ++i) {
                result.push_back(names[i]);
            }
        }
        return result;
    }
}