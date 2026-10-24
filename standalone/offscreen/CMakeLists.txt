cmake_minimum_required(VERSION 3.21)
project(OffscreenVulkan LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(Vulkan REQUIRED)
find_package(Threads REQUIRED)
option(OFFSCREEN_STREAMING "Build the WebRTC streaming server" ON)
set(ENABLE_NVENC AUTO CACHE STRING "Linux NVENC support: AUTO, ON (require SDK), or OFF")
set_property(CACHE ENABLE_NVENC PROPERTY STRINGS AUTO ON OFF)
if(NOT ENABLE_NVENC MATCHES "^(AUTO|ON|OFF)$")
    message(FATAL_ERROR "ENABLE_NVENC must be AUTO, ON, or OFF")
endif()

function(offscreen_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target} PRIVATE
            "$<$<COMPILE_LANGUAGE:CXX,OBJCXX>:-Wall;-Wextra;-Wpedantic;-Wconversion;-Wshadow>")
    endif()
endfunction()

add_executable(offscreen_renderer main.cpp)
target_link_libraries(offscreen_renderer PRIVATE Vulkan::Vulkan Threads::Threads)
offscreen_warnings(offscreen_renderer)

if(OFFSCREEN_STREAMING)
    if(NOT APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
        message(FATAL_ERROR "Streaming supports macOS and Linux; use OFFSCREEN_STREAMING=OFF elsewhere")
    endif()
    include(FetchContent)
    # The maintained upstream is paullouisageneau/libdatachannel (v0.24.5).
    set(NO_EXAMPLES ON CACHE BOOL "" FORCE)
    set(NO_TESTS ON CACHE BOOL "" FORCE)
    set(NO_MEDIA OFF CACHE BOOL "" FORCE)
    set(NO_WEBSOCKET OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(LIBSRTP_TEST_APPS OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(libdatachannel
        GIT_REPOSITORY https://github.com/paullouisageneau/libdatachannel.git
        GIT_TAG 443f6934d9007eb7076ab7825ba330f355fcbead
        GIT_SUBMODULES_RECURSE TRUE)
    FetchContent_MakeAvailable(libdatachannel)
    set_property(TARGET datachannel APPEND PROPERTY INTERFACE_SYSTEM_INCLUDE_DIRECTORIES
        "$<BUILD_INTERFACE:${libdatachannel_SOURCE_DIR}/include>")
    # Reuse upstream's pinned JSON submodule instead of fetching another copy.
    set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
    add_subdirectory("${libdatachannel_SOURCE_DIR}/deps/json"
                     "${libdatachannel_BINARY_DIR}/json" EXCLUDE_FROM_ALL)

    add_library(offscreen_streaming STATIC EncoderFactory.cpp WebRTCStreamer.cpp)
    target_include_directories(offscreen_streaming PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(offscreen_streaming PUBLIC LibDataChannel::LibDataChannel nlohmann_json::nlohmann_json)
    offscreen_warnings(offscreen_streaming)
    if(APPLE)
        enable_language(OBJCXX)
        set_target_properties(offscreen_streaming PROPERTIES OBJCXX_STANDARD 20 OBJCXX_STANDARD_REQUIRED ON
                              OBJCXX_EXTENSIONS OFF)
        find_library(COREFOUNDATION_FRAMEWORK CoreFoundation REQUIRED)
        find_library(COREMEDIA_FRAMEWORK CoreMedia REQUIRED)
        find_library(COREVIDEO_FRAMEWORK CoreVideo REQUIRED)
        find_library(VIDEOTOOLBOX_FRAMEWORK VideoToolbox REQUIRED)
        target_sources(offscreen_streaming PRIVATE VideoToolboxEncoder.mm)
        target_link_libraries(offscreen_streaming PRIVATE ${COREFOUNDATION_FRAMEWORK} ${COREMEDIA_FRAMEWORK}
            ${COREVIDEO_FRAMEWORK} ${VIDEOTOOLBOX_FRAMEWORK})
    else()
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(X264 REQUIRED IMPORTED_TARGET x264)
        target_sources(offscreen_streaming PRIVATE X264Encoder.cpp)
        target_link_libraries(offscreen_streaming PRIVATE PkgConfig::X264)
        if(NOT ENABLE_NVENC STREQUAL "OFF")
            if(ENABLE_NVENC STREQUAL "ON")
                find_package(CUDAToolkit 12.4 REQUIRED)
            else()
                find_package(CUDAToolkit 12.4 QUIET)
            endif()
            if(CUDAToolkit_FOUND)
                include(CheckLanguage)
                check_language(CUDA)
                if(NOT CMAKE_CUDA_COMPILER)
                    if(ENABLE_NVENC STREQUAL "ON")
                        message(FATAL_ERROR "ENABLE_NVENC=ON requires nvcc; install the CUDA compiler")
                    endif()
                    message(STATUS "nvcc unavailable: building Linux x264 streaming fallback")
                endif()
            endif()
            if(CUDAToolkit_FOUND AND CMAKE_CUDA_COMPILER)
                # Explicit architectures work in Docker builds without a visible GPU.
                if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
                    if(CUDAToolkit_VERSION VERSION_LESS 13)
                        set(CMAKE_CUDA_ARCHITECTURES "52;75;86;89" CACHE STRING "CUDA architectures")
                    else()
                        set(CMAKE_CUDA_ARCHITECTURES "75;86;89" CACHE STRING "CUDA architectures")
                    endif()
                endif()
                enable_language(CUDA)
                # Ubuntu 22.04's CMake knows nvcc's C++17 flags but not its C++20 flags.
                # Only the conversion kernel uses this dialect; C++ sources remain C++20.
                set_target_properties(offscreen_streaming PROPERTIES CUDA_STANDARD 17 CUDA_STANDARD_REQUIRED ON
                    CUDA_EXTENSIONS OFF
                    CUDA_ARCHITECTURES "${CMAKE_CUDA_ARCHITECTURES}")
                find_path(NVENC_INCLUDE_DIR nvEncodeAPI.h
                    HINTS "${NVENC_SDK_ROOT}/Interface" ENV NVENC_INCLUDE_DIR PATH_SUFFIXES ffnvcodec)
                if(NOT NVENC_INCLUDE_DIR)
                    FetchContent_Declare(nvcodec_headers
                        GIT_REPOSITORY https://github.com/FFmpeg/nv-codec-headers.git
                        GIT_TAG c69278340ab1d5559c7d7bf0edf615dc33ddbba7)
                    FetchContent_MakeAvailable(nvcodec_headers)
                    set(NVENC_INCLUDE_DIR "${nvcodec_headers_SOURCE_DIR}/include/ffnvcodec")
                endif()
                find_library(NVENC_LIBRARY NAMES nvidia-encode libnvidia-encode.so.1
                    HINTS "${NVENC_SDK_ROOT}/Lib/linux/stubs/x86_64")
                # Driver libraries are loaded at runtime: image builds need no GPU, and
                # a missing libcuda/libnvidia-encode must not prevent the x264 fallback.
                target_sources(offscreen_streaming PRIVATE VulkanMemoryExporter.cpp CudaVulkanInterop.cpp
                    CudaColorConversion.cu NvencZeroCopyEncoder.cpp)
                target_include_directories(offscreen_streaming SYSTEM PRIVATE "${NVENC_INCLUDE_DIR}")
                target_link_libraries(offscreen_streaming PRIVATE Vulkan::Vulkan CUDA::cudart_static ${CMAKE_DL_LIBS})
                target_compile_definitions(offscreen_streaming PUBLIC OFFSCREEN_HAS_NVENC=1)
                set(OFFSCREEN_NVENC_BUILT TRUE)
                message(STATUS "NVENC enabled; driver libraries will be resolved at runtime")
            elseif(NOT CUDAToolkit_FOUND)
                message(STATUS "CUDA Toolkit unavailable: building Linux x264 streaming fallback")
            endif()
        endif()
    endif()
    target_compile_definitions(offscreen_renderer PRIVATE OFFSCREEN_STREAMING=1)
    target_link_libraries(offscreen_renderer PRIVATE offscreen_streaming)
    configure_file(index.html index.html COPYONLY)
endif()

include(CTest)
if(BUILD_TESTING)
    add_test(NAME offscreen_render_and_readback COMMAND "${CMAKE_COMMAND}"
        "-DRENDERER=$<TARGET_FILE:offscreen_renderer>"
        "-DTEST_DIR=${CMAKE_CURRENT_BINARY_DIR}/test-output"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/verify_ppm.cmake")
    set_tests_properties(offscreen_render_and_readback PROPERTIES TIMEOUT 60)
    add_test(NAME offscreen_animation_and_orbit COMMAND offscreen_renderer --render-test)
    set_tests_properties(offscreen_animation_and_orbit PROPERTIES TIMEOUT 30
        FAIL_REGULAR_EXPRESSION "\\[validation error\\]")
    if(OFFSCREEN_STREAMING)
        add_executable(streaming_tests tests/Tests.cpp)
        target_link_libraries(streaming_tests PRIVATE offscreen_streaming)
        offscreen_warnings(streaming_tests)
        add_test(NAME streaming_unit_and_encoder COMMAND streaming_tests)
        add_test(NAME streaming_render_encode COMMAND offscreen_renderer --frames 120 --port 0)
        if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
            add_executable(software_encoder_tests tests/SoftwareEncoderTests.cpp X264Encoder.cpp)
            target_include_directories(software_encoder_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
            target_link_libraries(software_encoder_tests PRIVATE PkgConfig::X264)
            offscreen_warnings(software_encoder_tests)
            add_test(NAME software_encoder_and_fd_ownership COMMAND software_encoder_tests)
            option(OFFSCREEN_GPU_TESTS "Run tests requiring an NVIDIA Vulkan/CUDA/NVENC GPU" OFF)
            if(OFFSCREEN_GPU_TESTS)
                if(NOT OFFSCREEN_NVENC_BUILT)
                    message(FATAL_ERROR "OFFSCREEN_GPU_TESTS requires ENABLE_NVENC=ON")
                endif()
                foreach(input image buffer)
                    add_test(NAME nvenc_${input}_interop COMMAND offscreen_renderer
                        --encoder hardware --nvenc-input ${input} --frames 120 --port 0)
                    set_tests_properties(nvenc_${input}_interop PROPERTIES TIMEOUT 45
                        FAIL_REGULAR_EXPRESSION "\\[validation error\\]")
                endforeach()
            endif()
        endif()
        set_tests_properties(streaming_unit_and_encoder streaming_render_encode PROPERTIES
            TIMEOUT 30 FAIL_REGULAR_EXPRESSION "\\[validation error\\]")
    endif()
endif()
