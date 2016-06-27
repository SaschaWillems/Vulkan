set(EXTERNAL_NAME vkcpp)
string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)

include(ExternalProject)
ExternalProject_Add(
    ${EXTERNAL_NAME}
    DOWNLOAD_NAME vkcpp-cmake.zip
    URL https://codeload.github.com/jherico/vkcpp/zip/cmake
    URL_MD5 bf5d960fbaa9ff520bbfd97d9a9191ea
    CMAKE_ARGS ${PLATFORM_CMAKE_ARGS} -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> -DVK_SPEC_URL:STRING=https://raw.githubusercontent.com/KhronosGroup/Vulkan-Docs/2656f459333b3a1dc63619a9ebd83490eea22e93/src/spec/vk.xml
    LOG_DOWNLOAD 1
)

# Hide this external target (for ide users)
set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "externals")
ExternalProject_Get_Property(${EXTERNAL_NAME} INSTALL_DIR)
set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${INSTALL_DIR}/include CACHE TYPE INTERNAL)

