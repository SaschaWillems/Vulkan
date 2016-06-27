cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
cmake_policy(VERSION 2.8)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
include("cmake/defaults.cmake")
set(NAME vulkanExamples)

project(${NAME})

add_custom_target(SetupRelease ALL ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/bin)
set_target_properties(SetupRelease PROPERTIES FOLDER "CMakeTargets")
add_custom_target(SetupDebug ALL ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/bin_debug)
set_target_properties(SetupDebug PROPERTIES FOLDER "CMakeTargets")

find_package(Vulkan REQUIRED)
link_libraries(${VULKAN_LIBRARY})
include_directories(${VULKAN_INCLUDE_DIR})

if(WIN32)
    # Disable warnings on macro redifinition
    add_definitions( "/wd4005" )
    # Disable warnings on narrowing conversions
    add_definitions( "/wd4267" )
    # Disable warnings on loss of precision conversions (int to float)
    add_definitions( "/wd4244" )
    # Disable warnings on double to float
    add_definitions( "/wd4305" )
    # Disable warnings on signed/unsigned mismatch
    add_definitions( "/wd4018" )
    # Disable warnings on unreferenced local variable
    add_definitions( "/wd4101" )
    
    add_definitions(-DVK_USE_PLATFORM_WIN32_KHR)
else()
    add_definitions(-DVK_USE_PLATFORM_XCB_KHR)
    find_package(XCB REQUIRED)
    link_libraries(${XCB_LIBRARIES})
endif()

if (WIN32)
    add_dependency_external_projects(glfw3)
    list(APPEND EXTERNALS glfw3)
    include_directories(${GLFW3_INCLUDE_DIR})
    link_libraries(${GLFW3_LIBRARY})

    add_dependency_external_projects(zlib)
    list(APPEND EXTERNALS zlib)
    include_directories(${ZLIB_INCLUDE_DIRS})
    link_libraries(${ZLIB_LIBRARIES})

    add_dependency_external_projects(assimp)
    list(APPEND EXTERNALS assimp)
    include_directories(${ASSIMP_INCLUDE_DIRS})
    link_libraries(${ASSIMP_LIBRARIES})
else()
    pkg_check_modules(GLFW3 REQUIRED glfw3>=3.2)
    link_libraries(${GLFW3_LIBRARIES})
    include_directories(${GLFW3_INCLUDEDIR})
    link_directories(${GLFW3_LIBRARY_DIRS})
    
    find_package(assimp)
    link_libraries(${ASSIMP_LIBRARIES})
    include_directories(${ASSIMP_INCLUDEDIR})
    link_directories(${ASSIMP_LIBRARY_DIRS})
endif()

add_dependency_external_projects(glm)
list(APPEND EXTERNALS glm)
include_directories(${GLM_INCLUDE_DIRS})

add_dependency_external_projects(vkcpp)
list(APPEND EXTERNALS vkcpp)
include_directories(${VKCPP_INCLUDE_DIRS})

add_dependency_external_projects(gli)
list(APPEND EXTERNALS gli)
include_directories(${GLI_INCLUDE_DIRS})

add_dependency_external_projects(glslang)
list(APPEND EXTERNALS glslang)
include_directories(${GLSLANG_INCLUDE_DIRS})
link_libraries(${GLSLANG_LIBRARIES})

add_subdirectory(base)
link_libraries(base)
include_directories(base)

foreach(EXTERNAL ${EXTERNALS})
    add_dependencies(base ${EXTERNAL})
endforeach()

macro(BUILD_EXAMPLE)
    foreach(folder_name ${ARGN})
        message("Creating example ${folder_name}")
        set(TARGET ${folder_name})
        
        # Find source files 
        file(GLOB source_files ${folder_name}/*.cpp ${folder_name}/*.h ${folder_name}/*.cpp)

        # Find shader files
        set(SHADER_DIR "${PROJECT_SOURCE_DIR}/data/shaders/${folder_name}")
        file(GLOB shader_files 
            ${SHADER_DIR}/*.vert 
            ${SHADER_DIR}/*.frag 
            ${SHADER_DIR}/*.comp 
            ${SHADER_DIR}/*.tesc 
            ${SHADER_DIR}/*.tese
            ${SHADER_DIR}/*.geom
        )
        
        # FIXME Add logic to rebuild shaders when they change 
        foreach(shader ${shader_files})
            compile_spirv_shader(${shader})
        endforeach()

        # Create target
        add_executable(${TARGET} ${source_files} ${shader_files})
        set_target_properties(${TARGET} PROPERTIES FOLDER "examples")
        source_group("Shaders" FILES ${shader_files})

        # Add dependencies
        add_dependencies(${TARGET} base)
        target_link_libraries(${TARGET} ${EXAMPLE_LIBS})
        if (NOT WIN32)
            target_link_libraries(${TARGET} Threads::Threads)
        endif()
    endforeach()
endmacro()


MACRO(SUBDIRLIST result curdir)
  FILE(GLOB children RELATIVE ${curdir} ${curdir}/*)
  SET(dirlist "")
  FOREACH(child ${children})
    IF(IS_DIRECTORY ${curdir}/${child})
        IF("'base'" STREQUAL "'${child}'")
            CONTINUE()
        ENDIF()
        FILE(GLOB example "${curdir}/${child}/*.cpp")
        LIST(LENGTH example source_count)
        IF(${source_count} EQUAL 0)
            CONTINUE()
        ENDIF()
        LIST(APPEND dirlist ${child})
    ENDIF()
  ENDFOREACH()
  SET(${result} ${dirlist})
ENDMACRO()

SUBDIRLIST(SUBDIRS ${CMAKE_CURRENT_SOURCE_DIR})
BUILD_EXAMPLE(${SUBDIRS})
