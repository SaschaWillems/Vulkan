# 
#  Copyright 2015 High Fidelity, Inc.
#  Created by Bradley Austin Davis on 2015/10/10
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 
macro(TARGET_GLEW)
    add_dependency_external_projects(glew)
    add_dependencies(${TARGET_NAME} glew) 
    target_include_directories(${TARGET_NAME} PUBLIC ${GLEW_INCLUDE_DIRS})
    target_compile_definitions(${TARGET_NAME} PUBLIC -DGLEW_STATIC)
    target_link_libraries(${TARGET_NAME} ${GLEW_LIBRARY})
endmacro()
