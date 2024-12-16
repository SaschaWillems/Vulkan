cmake_minimum_required(VERSION 3.4.1 FATAL_ERROR)

set(NAME deferredmultisampling)
project(${NAME})
find_package(games-frame-pacing REQUIRED CONFIG)
find_package(game-activity REQUIRED CONFIG)

get_target_property(game-activity-include game-activity::game-activity INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(games-frame-pacing-include games-frame-pacing::swappy INTERFACE_INCLUDE_DIRECTORIES)

# Export GameActivity_onCreate(),
# Refer to: https://github.com/android-ndk/ndk/issues/381.
set(CMAKE_SHARED_LINKER_FLAGS
        "${CMAKE_SHARED_LINKER_FLAGS} -u GameActivity_onCreate")
set(CMAKE_SHARED_LINKER_FLAGS
        "${CMAKE_SHARED_LINKER_FLAGS} -u Java_com_google_androidgamesdk_GameActivity_initializeNativeCode")

set(SRC_DIR ../../../examples/${NAME})
set(BASE_DIR ../../../base)
set(EXTERNAL_DIR ../../../external)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -DVK_USE_PLATFORM_ANDROID_KHR -DVK_NO_PROTOTYPES")

file(GLOB EXAMPLE_SRC "${SRC_DIR}/*.cpp")

add_library(native-lib SHARED ${EXAMPLE_SRC})

# add_library(native-app-glue STATIC ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)

add_subdirectory(../base ${CMAKE_SOURCE_DIR}/../base)

# set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

include_directories(${BASE_DIR})
include_directories(${EXTERNAL_DIR})
include_directories(${EXTERNAL_DIR}/glm)
include_directories(${EXTERNAL_DIR}/imgui)
include_directories(${EXTERNAL_DIR}/tinygltf)
include_directories(${ANDROID_NDK}/sources/android/native_app_glue)

target_link_libraries(
    native-lib
    games-frame-pacing::swappy_static
    game-activity::game-activity_static
    libbase
    android
    log
    z
)
