# 
#  Created by Bradley Austin Davis on 2016/02/16
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 
macro(TARGET_VULKAN)
    
    if (VULKAN_FOUND)
        add_definitions(-DHAVE_VULKAN)
        target_link_libraries(${TARGET_NAME} Vulkan::Vulkan)
        include_directories(${VULKAN_INCLUDE_DIR})
         
        target_include_directories(${TARGET_NAME} PRIVATE ${VULKAN_INCLUDE_DIR})
        target_link_libraries(${TARGET_NAME} ${VULKAN_LIBRARY})
    endif()
endmacro()