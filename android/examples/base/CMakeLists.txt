file(GLOB BASE_SRC "../../../base/*.cpp" "../../../external/imgui/imgui.cpp" "../../../external/imgui/imgui_draw.cpp")

add_library(libbase SHARED ${BASE_SRC})

add_library(libassimp STATIC IMPORTED)
set_target_properties(libassimp PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/../../../libs/assimp/${ANDROID_ABI}/libassimp.a)

include_directories(${BASE_DIR})
include_directories(../../../external/glm)
include_directories(../../../external/gli)
include_directories(../../../external/imgui)
include_directories(../../../external/assimp)
include_directories(${ANDROID_NDK}/sources/android/native_app_glue)

target_link_libraries(
    libbase
    android
    log
    z
    libassimp    
)
