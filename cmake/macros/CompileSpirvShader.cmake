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
    find_program(GLSLANG_EXECUTABLE glslangValidator)
    #message("GLSL exec ${GLSLANG_EXECUTABLE}")
    get_filename_component(SHADER_TARGET ${SHADER_FILE} NAME_WE)
    get_filename_component(SHADER_EXT ${SHADER_FILE} EXT)
    set(COMPILE_OUTPUT "${SHADER_FILE}.spv")
    set(COMPILE_COMMAND "${PROJECT_BINARY_DIR}/${GLSLANG_EXEC} ${COMPILE_ARGS}")
    #message("Adding compile command for ${COMPILE_OUTPUT}") 
    #add_custom_command(
    #    OUTPUT ${COMPILE_OUTPUT} 
    #    COMMAND ${GLSLANG_EXECUTABLE} -V ${SHADER_FILE} -o ${COMPILE_OUTPUT} 
    #    MAIN_DEPENDENCY glslang DEPENDS ${SHADER_FILE})
    
    #add_dependencies(${TARGET} ${COMPILE_OUTPUT})
    #add_custom_command(
    #    TARGET ${TARGET} PRE_BUILD 
    #    COMMAND ${GLSLANG_EXECUTABLE} -V ${SHADER_FILE} -o ${COMPILE_OUTPUT} 
    #    DEPENDS ${SHADER_FILE})
endfunction()

