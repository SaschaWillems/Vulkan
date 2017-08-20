cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
cmake_policy(VERSION 2.8)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

set(NAME vulkanExamples)

project(${NAME})

include_directories(external)
include_directories(external/glm)
include_directories(external/gli)
include_directories(external/assimp)
include_directories(external/imgui)
include_directories(base)

OPTION(USE_D2D_WSI "Build the project using Direct to Display swapchain" OFF)
OPTION(USE_WAYLAND_WSI "Build the project using Wayland swapchain" OFF)

# Use FindVulkan module added with CMAKE 3.7
if (NOT CMAKE_VERSION VERSION_LESS 3.7.0)
	message(STATUS "Using module to find Vulkan")
	find_package(Vulkan)
endif()

IF(WIN32)
	IF (NOT Vulkan_FOUND)
		find_library(Vulkan_LIBRARY NAMES vulkan-1 vulkan PATHS ${CMAKE_SOURCE_DIR}/libs/vulkan)
		IF (Vulkan_LIBRARY)
			set(Vulkan_FOUND ON)
			MESSAGE("Using bundled Vulkan library version")
		ENDIF()
	ENDIF()
	find_library(ASSIMP_LIBRARIES NAMES assimp libassimp.dll.a PATHS ${CMAKE_SOURCE_DIR}/libs/assimp)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_WIN32_KHR")
ELSE(WIN32)
	IF (NOT Vulkan_FOUND)
		find_library(Vulkan_LIBRARY NAMES vulkan HINTS "$ENV{VULKAN_SDK}/lib" "${CMAKE_SOURCE_DIR}/libs/vulkan" REQUIRED)
		IF (Vulkan_LIBRARY)
			set(Vulkan_FOUND ON)
			MESSAGE("Using bundled Vulkan library version")
		ENDIF()
	ENDIF()
	find_package(ASSIMP REQUIRED)
	find_package(Threads REQUIRED)
IF(USE_D2D_WSI)
	MESSAGE("Using direct to display extension...")
	add_definitions(-D_DIRECT2DISPLAY)
ELSEIF(USE_WAYLAND_WSI)
	find_package(Wayland REQUIRED)
	if (NOT WAYLAND_FOUND)
		message(FATAL_ERROR "Wayland development package not found")
	endif ()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_WAYLAND_KHR")
	include_directories(${WAYLAND_INCLUDE_DIR})
ELSE(USE_D2D_WSI)
	find_package(XCB REQUIRED)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_XCB_KHR")
ENDIF(USE_D2D_WSI)
	# Todo : android?
ENDIF(WIN32)

IF (NOT Vulkan_FOUND)
	message(FATAL_ERROR "Could not find Vulkan library!")
ELSE()
	message(STATUS ${Vulkan_LIBRARY})
ENDIF()

# Set preprocessor defines
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNOMINMAX -D_USE_MATH_DEFINES")

# Clang specific stuff
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-switch-enum")
endif()

# Debug flags
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
if(CMAKE_COMPILER_IS_GNUCXX)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wextra -Wundef")
endif(CMAKE_COMPILER_IS_GNUCXX)

add_definitions(-D_CRT_SECURE_NO_WARNINGS)
add_definitions(-std=c++11)

# Find glslc shader compiler.
# On Android, the NDK includes the binary, so no external dependency.
if(ANDROID)
    file(GLOB glslc-folders ${ANDROID_NDK}/shader-tools/*)
else()
    file(GLOB glslc-folders ${VULKAN_SDK}/bin "$ENV{VULKAN_SDK}/bin" "$ENV{PATH}")
endif()
find_program(GLSLC glslc HINTS ${glslc-folders})
IF (GLSLC_NOTFOUND)
	message(FATAL_ERROR "Could not find glslc compiler!")
ELSE()
	message(STATUS "Found spir-v compiler: " ${GLSLC})
ENDIF()

# Add data target if build outside source tree
if (${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_BINARY_DIR})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/")
    add_definitions(-DBUILT_INSIDE_SRC_TREE)
else()
    file(GLOB_RECURSE DATAS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "data/models/*" "data/textures/*" "data/*.fnt")
    FOREACH(DATA_FILE ${DATAS})
        set(copy-output ${CMAKE_CURRENT_BINARY_DIR}/${DATA_FILE})
        GET_FILENAME_COMPONENT(copy-dest-dir ${copy-output} DIRECTORY)
        ADD_CUSTOM_COMMAND(
           OUTPUT  "${copy-output}"
           COMMAND ${CMAKE_COMMAND} -E make_directory ${copy-dest-dir}
           COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/${DATA_FILE}" "${copy-output}"
           COMMENT "Copying ${DATA_FILE} to ${copy-output}"
           DEPENDS ${DATA_FILE}
           VERBATIM
        )
        set(DATA_OUTPUTS ${DATA_OUTPUTS} ${copy-output})
    ENDFOREACH()
    add_custom_target(DataCopy ALL DEPENDS ${DATA_OUTPUTS} SOURCES ${DATAS})
endif()

file(GLOB SOURCE *.cpp)

# Function for building shader
function(buildShader EXAMPLE_NAME)
	# Add shaders target
	set(SHADER_DIR data/shaders/${EXAMPLE_NAME})
	file(GLOB SHADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "${SHADER_DIR}/*.vert" "${SHADER_DIR}/*.frag" "${SHADER_DIR}/*.geom" "${SHADER_DIR}/*.tesc" "${SHADER_DIR}/*.tese")
	source_group("Shaders" FILES ${SHADERS})
	foreach(SHADER ${SHADERS})
		set(shader-input ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER})
		set(shader-output ${CMAKE_CURRENT_BINARY_DIR}/${SHADER}.spv)
		add_custom_command (
		  OUTPUT ${shader-output}
		  COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/${SHADER_DIR}"
		  COMMAND "${GLSLC}" ${shader-input} -o ${shader-output}
		  COMMENT "Compiling ${shader-input}"
		  DEPENDS ${shader-input}
		  VERBATIM
		)
		set(SHADER_OUTPUTS ${SHADER_OUTPUTS} ${shader-output})
	endforeach(SHADER)
	add_custom_target("${EXAMPLE_NAME}.Shaders" ALL DEPENDS ${SHADER_OUTPUTS} SOURCES ${SHADERS})
endfunction(buildShader)

# Function for building single example
function(buildExample EXAMPLE_NAME)
	# Main
	file(GLOB SOURCE *.cpp ${BASE_HEADERS} ${EXAMPLE_NAME}/*.cpp)
	SET(MAIN_CPP ${EXAMPLE_NAME}/${EXAMPLE_NAME}.cpp)
	if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${EXAMPLE_NAME}/main.cpp)
		SET(MAIN_CPP ${EXAMPLE_NAME}/main.cpp)
	ENDIF()
	# imgui example requires additional source files
	IF(${EXAMPLE_NAME} STREQUAL "imgui")
		file(GLOB ADD_SOURCE external/imgui/*.cpp)
		SET(SOURCE ${SOURCE} ${ADD_SOURCE})
	ENDIF()
	buildShader(${EXAMPLE_NAME})
	if(WIN32)
		add_executable(${EXAMPLE_NAME} WIN32 ${MAIN_CPP} ${SOURCE})
		target_link_libraries(${EXAMPLE_NAME} base ${Vulkan_LIBRARY} ${ASSIMP_LIBRARIES} ${WINLIBS})
	else(WIN32)
		add_executable(${EXAMPLE_NAME} ${MAIN_CPP} ${SOURCE})
		target_link_libraries(${EXAMPLE_NAME} base )
	endif(WIN32)
endfunction(buildExample)

# Build all examples
function(buildExamples)
	foreach(EXAMPLE ${EXAMPLES})
		buildExample(${EXAMPLE})
	endforeach(EXAMPLE)
endfunction(buildExamples)

# Compiler specific stuff
IF(MSVC)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
ENDIF(MSVC)

IF(WIN32)
	# Nothing here (yet)
ELSE(WIN32)
	link_libraries(${XCB_LIBRARIES} ${Vulkan_LIBRARY} ${Vulkan_LIBRARY} ${ASSIMP_LIBRARIES} ${WAYLAND_CLIENT_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
ENDIF(WIN32)

add_subdirectory(base)

set(EXAMPLES
	bloom
	computecloth
	computecullandlod
	computenbody
	computeparticles
	computeshader
	debugmarker
	deferred
	deferredmultisampling
	deferredshadows
	displacement
	distancefieldfonts
	dynamicuniformbuffer
	gears
	geometryshader
	hdr
	imgui
	indirectdraw
	instancing
	mesh
	multisampling
	multithreading
	occlusionquery
	offscreen
	parallaxmapping
	particlefire
	pbrbasic
	pbribl
	pbrtexture
	pipelines
	pushconstants
	radialblur
	raytracing
	scenerendering
	screenshot
	shadowmapping
	shadowmappingomni
	skeletalanimation
	specializationconstants
	sphericalenvmapping
	ssao
	stencilbuffer
	subpasses
	terraintessellation
	tessellation
	textoverlay
	texture
	texture3d
	texturearray
	texturecubemap
	texturemipmapgen
	texturesparseresidency
	triangle
	viewportarray
	vulkanscene
)

buildShader(base)
buildExamples()
