file(GLOB BASE_SRC "../../base/*.cpp" "../../external/imgui/*.cpp")

add_library(libbase SHARED ${BASE_SRC})

message(STATUS "base_dir = ${BASE_DIR}")
include_directories(../../base)
# include_directories(../../external)  # Removed to avoid old vulkan.h
include_directories(../../external/glm)
include_directories(../../external/gli)
include_directories(../../external/imgui)
include_directories(../../external/tinygltf)

set(KTX_DIR ../../external/ktx)
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
	uv
	rawfile.z
	ace_ndk.z
	hilog_ndk.z
	z
	libktx
	vulkan
)
