cmake_minimum_required(VERSION 3.4.1 FATAL_ERROR)

set(NAME %EXAMPLE_FOLDER%)

set(SRC_DIR ../../../examples/${NAME})
set(BASE_DIR ../../../base)
set(EXTERNAL_DIR ../../../external)
set(LIB_DIR ../../../libs)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -DVK_USE_PLATFORM_ANDROID_KHR -DVK_NO_PROTOTYPES")

file(GLOB EXAMPLE_SRC "${SRC_DIR}/*.cpp")
file(GLOB BASE_SRC "${BASE_DIR}/*.cpp")

add_library(
    native-lib SHARED
    ${EXAMPLE_SRC}
    ${BASE_SRC}
    ${EXTERNAL_DIR}/imgui/imgui.cpp
    ${EXTERNAL_DIR}/imgui/imgui_draw.cpp
)

add_library(native-app-glue STATIC ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)

add_library(libassimp STATIC IMPORTED)
set_target_properties(libassimp PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/../../../libs/assimp/${ANDROID_ABI}/libassimp.a)

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

include_directories(${BASE_DIR})
include_directories(${EXTERNAL_DIR}/glm)
include_directories(${EXTERNAL_DIR}/gli)
include_directories(${EXTERNAL_DIR}/imgui)
include_directories(${EXTERNAL_DIR}/assimp)
include_directories(${ANDROID_NDK}/sources/android/native_app_glue)

target_link_libraries(
    native-lib
    native-app-glue
    android
    log
    z
    libassimp
)
