cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
cmake_policy(VERSION 2.8)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

set(NAME vulkanExamples)

project(${NAME})

include_directories(external)
include_directories(external/glm)
include_directories(external/gli)
include_directories(external/assimp)
include_directories(base)

IF(WIN32)
	find_library(VULKAN_LIB NAMES vulkan-1 vulkan PATHS ${CMAKE_SOURCE_DIR}/libs/vulkan)
	find_library(ASSIMP_LIBRARIES NAMES assimp libassimp.dll.a PATHS ${CMAKE_SOURCE_DIR}/libs/assimp)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_WIN32_KHR")
ELSE(WIN32)
	find_library(VULKAN_LIB NAMES libvulkan.so PATHS ${CMAKE_SOURCE_DIR}/libs/vulkan)
	find_package(ASSIMP REQUIRED)
	find_package(XCB REQUIRED)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_XCB_KHR")
	# Todo : android?
ENDIF(WIN32)

message(STATUS ${VULKAN_LIB})

# Set preprocessor defines
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNOMINMAX -D_USE_MATH_DEFINES")

add_definitions(-D_CRT_SECURE_NO_WARNINGS)
add_definitions(-std=c++11)
add_definitions(-std=c++0x)

file(GLOB SOURCE *.cpp base/*.cpp)

# Function for building single example
function(buildExample EXAMPLE_NAME)
	file(GLOB SOURCE *.cpp base/*.cpp ${EXAMPLE_NAME}/*.cpp)
	if(WIN32)
		add_executable(${EXAMPLE_NAME} WIN32 ${EXAMPLE_NAME}/${EXAMPLE_NAME}.cpp ${SOURCE})
		target_link_libraries(${EXAMPLE_NAME} ${VULKAN_LIB} ${ASSIMP_LIBRARIES} ${WINLIBS})
	else(WIN32)
		add_executable(${EXAMPLE_NAME} ${EXAMPLE_NAME}/${EXAMPLE_NAME}.cpp ${SOURCE})
		target_link_libraries(${EXAMPLE_NAME} ${VULKAN_LIB} ${ASSIMP_LIBRARIES} ${PTHREAD})
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
    SET(CMAKE_CXX_FLAGS "/EHsc")
ENDIF(MSVC)

IF(WIN32)
	# Nothing here (yet)
ELSE(WIN32)
	link_libraries(${XCB_LIBRARIES} ${VULKAN_LIB})
ENDIF(WIN32)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin/")

set(EXAMPLES 
	bloom
	computeshader
	computeparticles
	deferred
	displacement
	distancefieldfonts
	gears
	geometryshader
	instancing
	mesh
	multisampling
	occlusionquery
	offscreen
	parallaxmapping
	particlefire
	pipelines
	pushconstants
	raytracing
	radialblur
	shadowmapping
	shadowmappingomni
	skeletalanimation
	sphericalenvmapping
	tessellation
	texture
	texturearray
	texturecubemap
	triangle
	vulkanscene
)

buildExamples()
