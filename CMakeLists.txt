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

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMakeTargets")
set(CMAKE_DEBUG_POSTFIX "d")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/bin/")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin/")

if(WIN32)
    if(NOT (CMAKE_SIZEOF_VOID_P EQUAL 8) )
        message(FATAL_ERROR "Cannot build 32 bit examples on windows.\nUse `cmake -G \"Visual Studio 14 2015 Win64\" <args>`")
    endif()
	find_library(VULKAN_LIB NAMES vulkan-1 vulkan PATHS ${CMAKE_SOURCE_DIR}/libs/vulkan)
	find_library(ASSIMP_LIBRARIES NAMES assimp libassimp.dll.a PATHS ${CMAKE_SOURCE_DIR}/libs/assimp)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_WIN32_KHR")
else()
	find_library(VULKAN_LIB NAMES libvulkan.so PATHS ${CMAKE_SOURCE_DIR}/libs/vulkan)
	find_package(ASSIMP REQUIRED)
	find_package(XCB REQUIRED)
	find_package(Threads REQUIRED)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_XCB_KHR")
	# Todo : android?
endif()

# Compiler specific stuff
add_definitions(-D_CRT_SECURE_NO_WARNINGS)
add_definitions(-D_USE_MATH_DEFINES)
add_definitions(-DNOMINMAX)

if (MSVC)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
else()
    add_definitions(-std=c++14)
    add_definitions(-std=c++0x)
    add_definitions(-std=gnu++14)
endif()

file(GLOB COMMON_SOURCE base/*.cpp base/*.h base/*.hpp)
add_library(base STATIC ${COMMON_SOURCE})
set_target_properties(base PROPERTIES FOLDER "shared")


# Function for building single example
function(buildExample EXAMPLE_NAME)
	file(GLOB SOURCE ${EXAMPLE_NAME}/*.cpp)
	if(WIN32)
		add_executable(${EXAMPLE_NAME} WIN32 ${SOURCE})
	else()
        add_executable(${EXAMPLE_NAME} ${SOURCE})
    endif()

    set_target_properties(${EXAMPLE_NAME} PROPERTIES FOLDER "examples")
	set_target_properties(${EXAMPLE_NAME} PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
	
    add_dependencies(${EXAMPLE_NAME} base)
    target_link_libraries(${EXAMPLE_NAME} base ${VULKAN_LIB} ${ASSIMP_LIBRARIES} )
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
    
    if (WIN32)
        target_link_libraries(${EXAMPLE_NAME} ${WINLIBS})
    else()
        target_link_libraries(${EXAMPLE_NAME} ${XCB_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
    endif()
endfunction(buildExample)

# Build all examples
function(buildExamples)
	foreach(EXAMPLE ${EXAMPLES})
		buildExample(${EXAMPLE})
    endforeach(EXAMPLE)
endfunction(buildExamples)


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
    multithreading
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
	textoverlay
	texture
	texturearray
	texturecubemap
	triangle
	vulkanscene
)

buildExamples()
