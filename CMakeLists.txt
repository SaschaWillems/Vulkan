cmake_minimum_required(VERSION 3.7 FATAL_ERROR)
cmake_policy(VERSION 3.7)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
include("cmake/defaults.cmake")
set(NAME vulkanExamples)

project(${NAME})

find_package(Vulkan REQUIRED)
include_directories(${Vulkan_INCLUDE_DIR})
link_libraries(${Vulkan_LIBRARY})

include_directories(external)
include_directories(external/glm)
include_directories(external/gli)
include_directories(external/assimp)
include_directories(base)

OPTION(USE_D2D_WSI "Build the project using Direct to Display swapchain" OFF)

# Use FindVulkan module added with CMAKE 3.7
if (NOT CMAKE_VERSION VERSION_LESS 3.7.0)
	message(STATUS "Using module to find Vulkan")
	find_package(Vulkan REQUIRED)
endif()

IF(WIN32)
	IF (NOT Vulkan_FOUND)
		find_library(Vulkan_LIBRARY NAMES vulkan-1 vulkan PATHS ${CMAKE_SOURCE_DIR}/libs/vulkan)
		IF (Vulkan_LIBRARY)
			set(Vulkan_FOUND ON)
		ENDIF()
	ENDIF()
	find_library(ASSIMP_LIBRARIES NAMES assimp libassimp.dll.a PATHS ${CMAKE_SOURCE_DIR}/libs/assimp)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_WIN32_KHR")

    add_dependency_external_projects(glfw3)
    list(APPEND EXTERNALS glfw3)
    include_directories(${GLFW3_INCLUDE_DIR})
    link_libraries(${GLFW3_LIBRARY})
ELSE(WIN32)
	IF (NOT Vulkan_FOUND)
		find_library(Vulkan_LIBRARY NAMES vulkan HINTS "$ENV{VULKAN_SDK}/lib" "${CMAKE_SOURCE_DIR}/libs/vulkan" REQUIRED)
		IF (Vulkan_LIBRARY)
			set(Vulkan_FOUND ON)
		ENDIF()
	ENDIF()
    find_package(PkgConfig)
    find_package(ASSIMP REQUIRED)
    find_package(Threads REQUIRED)
    pkg_check_modules(GLFW3 REQUIRED glfw3>=3.2)
    link_libraries(${GLFW3_LIBRARIES})
    include_directories(${GLFW3_INCLUDEDIR})
    link_directories(${GLFW3_LIBRARY_DIRS})

    IF(USE_D2D_WSI)
        MESSAGE("Using direct to display extension...")
        add_definitions(-D_DIRECT2DISPLAY)
    ELSE(USE_D2D_WSI)
        find_package(XCB REQUIRED)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_XCB_KHR")
    ENDIF(USE_D2D_WSI)
    # Todo : android?
ENDIF()

IF (NOT Vulkan_FOUND)
	message(FATAL_ERROR "Could not find Vulkan library!")
ELSE()
	message(STATUS ${Vulkan_LIBRARY})
ENDIF()

# Set preprocessor defines
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNOMINMAX -D_USE_MATH_DEFINES")

add_definitions(-D_CRT_SECURE_NO_WARNINGS)
add_definitions(-std=c++11)

file(GLOB SOURCE base/*.hpp base/*.h *.cpp base/*.cpp)
add_custom_target( Base SOURCES ${SOURCE})

# Function for building single example
function(buildExample EXAMPLE_NAME)
	file(GLOB SOURCE *.cpp base/*.cpp ${EXAMPLE_NAME}/*.cpp)
    add_executable(${EXAMPLE_NAME} ${EXAMPLE_NAME}/${EXAMPLE_NAME}.cpp ${SOURCE})
    set_target_properties(${EXAMPLE_NAME} PROPERTIES FOLDER ${FOLDER})
    target_link_libraries(${EXAMPLE_NAME} ${VULKAN_LIB} ${ASSIMP_LIBRARIES} )
    add_dependencies(${EXAMPLE_NAME} glfw3)
	if(WIN32)
        target_link_libraries(${EXAMPLE_NAME} ${WINLIBS})
	else(WIN32)
		target_link_libraries(${EXAMPLE_NAME} ${CMAKE_THREAD_LIBS_INIT})
	endif(WIN32)
endfunction(buildExample)

# Build all examples
function(buildExamples )
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
	link_libraries(${XCB_LIBRARIES} ${Vulkan_LIBRARY})
ENDIF(WIN32)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/")

set(FOLDER "Compute")
set(EXAMPLES 
    computecullandlod
    computenbody
	computeparticles
    computeshader
	raytracing
)
buildExamples()

set(FOLDER "Deferred")
set(EXAMPLES 
    deferred
    deferredshadows
    deferredmultisampling
)
buildExamples()

set(FOLDER "Tesselation")
set(EXAMPLES 
    displacement
    terraintessellation
    tessellation
)
buildExamples()

set(FOLDER "Textures")
set(EXAMPLES 
    texture
    texture3d
    texturearray
    texturecubemap
    texturemipmapgen
    texturesparseresidency
)
buildExamples()

set(FOLDER "Other")
set(EXAMPLES 
    bloom
    debugmarker
    distancefieldfonts
    gears
    geometryshader
    indirectdraw    
    instancing
    mesh
    multisampling
    multithreading
    occlusionquery
    offscreen
    parallaxmapping
    particlefire
    pipelines
    pushconstants
    radialblur
    scenerendering
    shadowmapping
    shadowmappingomni
    skeletalanimation
    sphericalenvmapping
    subpasses
	ssao
    textoverlay
    triangle
    vulkanscene
)
buildExamples()
