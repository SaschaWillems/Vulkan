# 
#  Copyright 2015 High Fidelity, Inc.
#  Created by Bradley Austin Davis on 2015/10/10
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 
macro(TARGET_GLM)
  add_dependency_external_projects(glm)
  find_package(GLM REQUIRED)
  target_include_directories(${TARGET_NAME} PUBLIC ${GLM_INCLUDE_DIRS})
endmacro()