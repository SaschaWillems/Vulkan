set(EXTERNAL_NAME gli)
string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)

include(ExternalProject)
ExternalProject_Add(
    ${EXTERNAL_NAME}
    URL "https://github.com/g-truc/gli/releases/download/0.7.0.0/gli-0.7.0.0.zip"
    URL_MD5 662a00f7e85dcb7bf33f8214e80b850a
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    LOG_DOWNLOAD 1
)

# Hide this external target (for ide users)
set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "externals")

ExternalProject_Get_Property(${EXTERNAL_NAME} SOURCE_DIR)
set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${SOURCE_DIR} CACHE TYPE INTERNAL)

