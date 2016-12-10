# 
#  CompileSpirvShader.cmake
# 
#  Created by Bradley Austin Davis on 2016/06/23
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

function(COMPILE_SPIRV_SHADER SHADER_FILE)
    # Define the final name of the generated shader file
    find_program(GLSLANG_EXECUTABLE glslangValidator
        HINTS "$ENV{VULKAN_SDK}/bin")
    get_filename_component(SHADER_TARGET ${SHADER_FILE} NAME_WE)
    get_filename_component(SHADER_EXT ${SHADER_FILE} EXT)
    set(COMPILE_OUTPUT "${SHADER_FILE}.spv")
    set(COMPILE_COMMAND "${PROJECT_BINARY_DIR}/${GLSLANG_EXEC} ${COMPILE_ARGS}")
    add_custom_command(
        OUTPUT ${COMPILE_OUTPUT} 
        COMMAND ${GLSLANG_EXECUTABLE} -V ${SHADER_FILE} -o ${COMPILE_OUTPUT} 
        DEPENDS ${SHADER_FILE})
    set(COMPILE_SPIRV_SHADER_RETURN ${COMPILE_OUTPUT} PARENT_SCOPE)
endfunction()

