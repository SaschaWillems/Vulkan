file(GLOB BASE_SRC "../../../base/*.cpp" "../../../external/imgui/*.cpp")

add_library(libbase SHARED ${BASE_SRC})

include_directories(${BASE_DIR})
include_directories(../../../external)
include_directories(../../../external/glm)
include_directories(../../../external/gli)
include_directories(../../../external/imgui)
include_directories(${EXTERNAL_DIR}/tinygltf)
include_directories(${ANDROID_NDK}/sources/android/native_app_glue)
include_directories(${game-activity-include})
include_directories(${games-frame-pacing-include})

set(KTX_DIR ../../../external/ktx)
set(KTX_SOURCES
	${KTX_DIR}/lib/texture.c
	${KTX_DIR}/lib/hashlist.c
	${KTX_DIR}/lib/checkheader.c
	${KTX_DIR}/lib/swap.c
	${KTX_DIR}/lib/memstream.c
	${KTX_DIR}/lib/filestream.c
	${KTX_DIR}/lib/vkloader.c
)
set(KTX_INCLUDE
	${KTX_DIR}/include
	${KTX_DIR}/lib
	${KTX_DIR}/other_include
)

add_library(libktx ${KTX_SOURCES})
target_include_directories(libktx PUBLIC ${KTX_INCLUDE})
set_property(TARGET libktx PROPERTY FOLDER "external")


target_link_libraries(
	libbase
	games-frame-pacing::swappy_static
	android
	log
	z
	libktx
)
