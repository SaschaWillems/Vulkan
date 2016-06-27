set(EXTERNAL_NAME glm)

include(ExternalProject)
ExternalProject_Add(
  ${EXTERNAL_NAME}
  URL https://github.com/g-truc/glm/releases/download/0.9.7.5/glm-0.9.7.5.zip
  URL_MD5 8d8da9c561e43c975946be9c6d37935d
  BINARY_DIR ${EXTERNAL_PROJECT_PREFIX}/build
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
  LOG_DOWNLOAD 1
  LOG_CONFIGURE 1
  LOG_BUILD 1
)

# Hide this external target (for ide users)
set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "externals")

ExternalProject_Get_Property(${EXTERNAL_NAME} INSTALL_DIR)

string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)
set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${INSTALL_DIR}/include CACHE PATH "List of glm include directories")