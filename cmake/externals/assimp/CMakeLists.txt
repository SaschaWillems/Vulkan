set(EXTERNAL_NAME assimp)

include(ExternalProject)

ExternalProject_Add(
  ${EXTERNAL_NAME}
  URL https://github.com/assimp/assimp/archive/v3.1.1.zip
  URL_MD5 fabc9b11f57dc0ed26671e9a80ec4e29
  CMAKE_ARGS ${PLATFORM_CMAKE_ARGS} --config Release -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>  -DBUILD_SHARED_LIBS=1 -DASSIMP_BUILD_ASSIMP_TOOLS=0 -DASSIMP_BUILD_TESTS=0 -DASSIMP_BUILD_STATIC_LIB=OFF -DASSIMP_BUILD_STATIC_LIB=OFF
  LOG_DOWNLOAD 1
  LOG_CONFIGURE 1
  LOG_BUILD 1
  BINARY_DIR ${EXTERNAL_PROJECT_PREFIX}/build
)

# Hide this external target (for ide users)
set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "externals")

ExternalProject_Get_Property(${EXTERNAL_NAME} INSTALL_DIR)

string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)

set(LIB_DIR "${INSTALL_DIR}/lib")

if (UNIX)
  set(LIB_PREFIX "lib")
  set(LIB_EXT "a")
elseif (WIN32)
  set(LIB_EXT "lib")
endif ()

set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${INSTALL_DIR}/include CACHE PATH "List of assimp include directories")
set(${EXTERNAL_NAME_UPPER}_LIBRARY_DEBUG ${INSTALL_DIR}/lib/${LIB_PREFIX}assimpd.${LIB_EXT} CACHE FILEPATH "Path to assimp debug library")
set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE ${INSTALL_DIR}/lib/${LIB_PREFIX}assimp.${LIB_EXT} CACHE FILEPATH "Path to assimp release library")

add_custom_command(TARGET ${EXTERNAL_NAME} POST_BUILD COMMAND if 1==$<CONFIG:Release> ${CMAKE_COMMAND} -E copy ${INSTALL_DIR}/bin/assimp.dll ${CMAKE_SOURCE_DIR}/bin )
add_custom_command(TARGET ${EXTERNAL_NAME} POST_BUILD COMMAND if 1==$<CONFIG:Debug> ${CMAKE_COMMAND} -E copy ${INSTALL_DIR}/bin/assimpd.dll ${CMAKE_SOURCE_DIR}/bin_debug )

include(SelectLibraryConfigurations)
select_library_configurations(${EXTERNAL_NAME_UPPER})
set(${EXTERNAL_NAME_UPPER}_LIBRARIES ${${EXTERNAL_NAME_UPPER}_LIBRARY} CACHE TYPE INTERNAL)

