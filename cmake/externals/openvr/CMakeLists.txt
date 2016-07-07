include(ExternalProject)
include(SelectLibraryConfigurations)

set(EXTERNAL_NAME OpenVR)

string(TOUPPER ${EXTERNAL_NAME} EXTERNAL_NAME_UPPER)

ExternalProject_Add(
  ${EXTERNAL_NAME}
  URL https://github.com/ValveSoftware/openvr/archive/v0.9.19.zip
  URL_MD5 843f9dde488584d8af1f3ecf2252b4e0
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
  LOG_DOWNLOAD 1
)

# Hide this external target (for ide users)
set_target_properties(${EXTERNAL_NAME} PROPERTIES FOLDER "externals")

ExternalProject_Get_Property(${EXTERNAL_NAME} SOURCE_DIR)

set(${EXTERNAL_NAME_UPPER}_INCLUDE_DIRS ${SOURCE_DIR}/headers CACHE TYPE INTERNAL)

if (WIN32)

  # FIXME need to account for different architectures
  if ("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
    set(${EXTERNAL_NAME_UPPER}_LIBRARIES ${SOURCE_DIR}/lib/win64/openvr_api.lib CACHE TYPE INTERNAL)
    add_custom_command(TARGET ${EXTERNAL_NAME} POST_BUILD COMMAND if 1==$<CONFIG:Release> ${CMAKE_COMMAND} -E copy ${SOURCE_DIR}/bin/win64/openvr_api.dll ${CMAKE_SOURCE_DIR}/bin )
    add_custom_command(TARGET ${EXTERNAL_NAME} POST_BUILD COMMAND if 1==$<CONFIG:Debug> ${CMAKE_COMMAND} -E copy ${SOURCE_DIR}/bin/win64/openvr_api.dll ${CMAKE_SOURCE_DIR}/bin_debug )
  else()
    set(${EXTERNAL_NAME_UPPER}_LIBRARIES ${SOURCE_DIR}/lib/win32/openvr_api.lib CACHE TYPE INTERNAL)
    add_paths_to_fixup_libs(${SOURCE_DIR}/bin/win32)
    add_custom_command(TARGET ${EXTERNAL_NAME} POST_BUILD COMMAND if 1==$<CONFIG:Release> ${CMAKE_COMMAND} -E copy ${SOURCE_DIR}/bin/win32/openvr_api.dll ${CMAKE_SOURCE_DIR}/bin )
    add_custom_command(TARGET ${EXTERNAL_NAME} POST_BUILD COMMAND if 1==$<CONFIG:Debug> ${CMAKE_COMMAND} -E copy ${SOURCE_DIR}/bin/win32/openvr_api.dll ${CMAKE_SOURCE_DIR}/bin_debug )
  endif()


elseif(APPLE)

  # FIXME need to account for different architectures 
  set(${EXTERNAL_NAME_UPPER}_LIBRARIES ${SOURCE_DIR}/lib/osx32/libopenvr_api.dylib CACHE TYPE INTERNAL)
  add_paths_to_fixup_libs(${SOURCE_DIR}/bin/osx32)

elseif(NOT ANDROID)

  # FIXME need to account for different architectures 
  set(${EXTERNAL_NAME_UPPER}_LIBRARIES ${SOURCE_DIR}/lib/linux64/libopenvr_api.so CACHE TYPE INTERNAL)
  add_paths_to_fixup_libs(${SOURCE_DIR}/bin/linux64)
  
endif()

