# 
#  Copyright 2015 High Fidelity, Inc.
#  Created by Bradley Austin Davis on 2015/10/10
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 
macro(TARGET_OCULUS)
    add_dependency_external_projects(LibOVR)
    add_dependencies(${TARGET_NAME} LibOVR) 
    target_include_directories(${TARGET_NAME} PRIVATE ${LIBOVR_INCLUDE_DIRS})
    target_link_libraries(${TARGET_NAME} ${LIBOVR_LIBRARIES})
endmacro()
