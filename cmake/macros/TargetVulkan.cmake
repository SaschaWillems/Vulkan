# 
#  Created by Bradley Austin Davis on 2016/02/16
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 
macro(TARGET_VULKAN)
    find_package(vulkan)

    if (VULKAN_FOUND)
        add_definitions(-DHAVE_VULKAN) 
        target_include_directories(${TARGET_NAME} PRIVATE ${VULKAN_INCLUDE_DIR})
        target_link_libraries(${TARGET_NAME} ${VULKAN_LIBRARY})

        add_dependency_external_projects(glslang)
        target_include_directories(${TARGET_NAME} PRIVATE ${GLSLANG_INCLUDE_DIRS})
        target_link_libraries(${TARGET_NAME} ${GLSLANG_LIBRARIES})

        add_dependency_external_projects(vkcpp)
        target_include_directories(${TARGET_NAME} PRIVATE ${VKCPP_INCLUDE_DIRS})
    endif()
endmacro()