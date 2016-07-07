set(EXTERNAL_NAME glslang)
string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)

include(ExternalProject)

ExternalProject_Add(
  ${EXTERNAL_NAME}
  DOWNLOAD_NAME glslang-95d85ce0caea6609372e83324e370fddfa368c28.zip
  URL https://codeload.github.com/KhronosGroup/glslang/zip/95d85ce0caea6609372e83324e370fddfa368c28
  URL_MD5 6eeb4523352a06da51c89fdb1f2db0b6
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
  BINARY_DIR ${EXTERNAL_PROJECT_PREFIX}/build
  LOG_DOWNLOAD 1
  LOG_CONFIGURE 1
  LOG_BUILD 1
)

# Hide this external target (for ide users)
set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "externals")

# includes
ExternalProject_Get_Property(${EXTERNAL_NAME} SOURCE_DIR)
list(APPEND INCLUDE_DIRS ${SOURCE_DIR}/glslang/Include)
list(APPEND INCLUDE_DIRS ${SOURCE_DIR}/SPIRV)
set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIR ${INCLUDE_DIRS} CACHE PATH "List of glslang include directories")
set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${${EXTERNAL_NAME_UPPER}_INCLUDE_DIR} CACHE PATH "List of glslang include directories")


ExternalProject_Get_Property(${EXTERNAL_NAME} INSTALL_DIR)
set(LIB_DIR ${INSTALL_DIR}/lib)
list(APPEND LIB_NAMES glslang HLSL OGLCompiler OSDependent SPIRV)

if(WIN32)
	set(LIB_PREFIX "")
	set(LIB_SUFFIX ".lib")
else()
	set(LIB_PREFIX "lib")
	set(LIB_SUFFIX ".a")
endif()

foreach(BASE_LIB ${LIB_NAMES})
    string(TOUPPER ${BASE_LIB} BASE_LIB_UPPER)
    list(APPEND ${EXTERNAL_NAME_UPPER}_LIBRARY_RELEASE "${LIB_DIR}/${LIB_PREFIX}${BASE_LIB}${LIB_SUFFIX}")
    if (WIN32)
       list(APPEND ${EXTERNAL_NAME_UPPER}_LIBRARY_DEBUG "${LIB_DIR}/${LIB_PREFIX}${BASE_LIB}d${LIB_SUFFIX}")
    endif()
endforeach()

include(SelectLibraryConfigurations)
select_library_configurations(${EXTERNAL_NAME_UPPER})

set(${EXTERNAL_NAME_UPPER}_LIBRARY ${${EXTERNAL_NAME_UPPER}_LIBRARY} CACHE FILEPATH "Location of glslang libraries")
set(${EXTERNAL_NAME_UPPER}_LIBRARIES ${${EXTERNAL_NAME_UPPER}_LIBRARIES} CACHE FILEPATH "Location of glslang libraries")
set(${EXTERNAL_NAME_UPPER}_EXEC ${INSTALL_DIR}/bin/glslangValidator CACHE FILEPATH "Location of glslang executable")
