set(EXTERNAL_NAME glew)

include(ExternalProject)
ExternalProject_Add(
  ${EXTERNAL_NAME}
  URL http://hifi-public.s3.amazonaws.com/dependencies/glew_simple_1.13.0.zip
  URL_MD5 73f833649e904257b35bf4e84f8bdfb5
  CONFIGURE_COMMAND CMAKE_ARGS ${ANDROID_CMAKE_ARGS} -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
  LOG_DOWNLOAD 1
  LOG_CONFIGURE 1
  LOG_BUILD 1
)


# Hide this external target (for ide users)
set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "externals")

ExternalProject_Get_Property(${EXTERNAL_NAME} INSTALL_DIR)

string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)
set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${INSTALL_DIR}/include CACHE PATH "List of glew include directories")

if (UNIX)
  set(LIB_PREFIX "lib")
  set(LIB_EXT "a")
elseif (WIN32)
  set(LIB_EXT "lib")
endif ()

set(${EXTERNAL_NAME_UPPER}_LIBRARY_DEBUG ${INSTALL_DIR}/lib/${LIB_PREFIX}glew_d.${LIB_EXT} CACHE FILEPATH "Path to glew debug library")
set(${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE ${INSTALL_DIR}/lib/${LIB_PREFIX}glew.${LIB_EXT} CACHE FILEPATH "Path to glew release library")

include(SelectLibraryConfigurations)
select_library_configurations(${EXTERNAL_NAME_UPPER})

set(${EXTERNAL_NAME_UPPER}_LIBRARY ${${EXTERNAL_NAME_UPPER}_LIBRARY} CACHE FILEPATH "Location of glew libraries")
set(${EXTERNAL_NAME_UPPER}_LIBRARIES ${${EXTERNAL_NAME_UPPER}_LIBRARIES} CACHE FILEPATH "Location of glew libraries")
