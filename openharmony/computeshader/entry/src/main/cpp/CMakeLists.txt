cmake_minimum_required(VERSION 3.4.1 FATAL_ERROR)

project(computeshader)

set(NAME computeshader)

# ../../..：项目根目录位置
set(SRC_DIR ../../../../../../examples/${NAME})
set(BASE_DIR ../../../../../../base)
set(EXTERNAL_DIR ../../../../../../external)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -DVK_USE_PLATFORM_OHOS")

message(STATUS "SRC_DIR = ${SRC_DIR}")
file(GLOB EXAMPLE_SRC "${SRC_DIR}/*.cpp")

add_library(${NAME} SHARED ${EXAMPLE_SRC})

message(STATUS "CMAKE_SOURCE_DIR = ${CMAKE_SOURCE_DIR}")
add_subdirectory(../../../../../base ${CMAKE_SOURCE_DIR}/../../../../../base)

message(STATUS "base_dir = ${BASE_DIR}")
include_directories(${BASE_DIR})
# Use system Vulkan SDK from NDK, not external/vulkan
# include_directories(${EXTERNAL_DIR})  # Removed to avoid old vulkan.h
include_directories(${EXTERNAL_DIR}/glm)
include_directories(${EXTERNAL_DIR}/imgui)

target_link_libraries(${NAME} PUBLIC
    libbase
    uv
    ace_napi.z
    rawfile.z
    ace_ndk.z
    hilog_ndk.z
    vulkan
    libnative_window.so)
