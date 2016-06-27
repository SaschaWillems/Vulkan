if (WIN32)
    find_path(VULKAN_INCLUDE_DIR NAMES vulkan/vulkan.h HINTS "$ENV{VULKAN_SDK}/Include" "$ENV{VK_SDK_PATH}/Include")
    if (CMAKE_CL_64)
        find_library(VULKAN_LIBRARY NAMES vulkan-1 HINTS "$ENV{VULKAN_SDK}/Bin" "$ENV{VK_SDK_PATH}/Bin")
    else()
        find_library(VULKAN_LIBRARY NAMES vulkan-1 HINTS "$ENV{VULKAN_SDK}/Bin32" "$ENV{VK_SDK_PATH}/Bin32")
    endif()
else()

    find_path(
	VULKAN_INCLUDE_DIR NAMES vulkan/vulkan.h 
	HINTS 
		"$ENV{VULKAN_SDK}/include"
		"$ENV{VULKAN_SDK}/x86_64/include")
    find_library(
	VULKAN_LIBRARY NAMES vulkan 
	HINTS
		"$ENV{VULKAN_SDK}/lib"
		"$ENV{VULKAN_SDK}/x86_64/lib")

endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Vulkan DEFAULT_MSG VULKAN_LIBRARY VULKAN_INCLUDE_DIR)
mark_as_advanced(VULKAN_INCLUDE_DIR VULKAN_LIBRARY)
