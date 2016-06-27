#
#  Created by Bradley Austin Davis on 2016/06/03
#  Copyright 2013-2016 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#
macro(TARGET_ASSIMP)
    add_dependency_external_projects(assimp)
    find_package(assimp REQUIRED)
    target_include_directories(${TARGET_NAME} PRIVATE ${ASSIMP_INCLUDE_DIRS})
    target_link_libraries(${TARGET_NAME} ${ASSIMP_LIBRARIES})
endmacro()
