set(EXTERNAL_NAME zlib)
string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)

include(ExternalProject)

ExternalProject_Add(
  ${EXTERNAL_NAME}
  URL http://zlib.net/zlib128.zip
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
  BINARY_DIR ${EXTERNAL_PROJECT_PREFIX}/build
  LOG_DOWNLOAD 1
  LOG_CONFIGURE 1
  LOG_BUILD 1
)
  
# Hide this external target (for ide users)
set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "externals")

ExternalProject_Get_Property(${EXTERNAL_NAME} INSTALL_DIR)
set(${EXTERNAL_NAME_UPPER}_ROOT ${INSTALL_DIR} CACHE PATH "Path for Zlib install root")
set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIR ${INSTALL_DIR}/include CACHE PATH "List of zlib include directories")
set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${${EXTERNAL_NAME_UPPER}_INCLUDE_DIR} CACHE PATH "List of zlib include directories")
set(${EXTERNAL_NAME_UPPER}_DLL_PATH ${INSTALL_DIR}/bin CACHE FILEPATH "Location of ZLib DLL")
set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE ${INSTALL_DIR}/lib/zlibstatic.lib CACHE FILEPATH "Location of zlib release library")
set(${EXTERNAL_NAME_UPPER}_LIBRARY_DEBUG ${INSTALL_DIR}/lib/zlibstaticd.lib CACHE FILEPATH "Location of zlib debug library")

include(SelectLibraryConfigurations)
select_library_configurations(${EXTERNAL_NAME_UPPER})

# Force selected libraries into the cache
set(${EXTERNAL_NAME_UPPER}_LIBRARY ${${EXTERNAL_NAME_UPPER}_LIBRARY} CACHE FILEPATH "Location of zlib libraries")
set(${EXTERNAL_NAME_UPPER}_LIBRARIES ${${EXTERNAL_NAME_UPPER}_LIBRARIES} CACHE FILEPATH "Location of zlib libraries") 
