/*
* Class wrapping access to the swap chain
* 
* A swap chain is a collection of framebuffers used for rendering and presentation to the windowing system
*
* Copyright (C) 2016-2025 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "VulkanSwapChain.h"

/** @brief Creates the platform specific surface abstraction of the native platform window used for presentation */	
#if defined(VK_USE_PLATFORM_WIN32_KHR)
void VulkanSwapChain::initSurface(void* platformHandle, void* platformWindow)
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
void VulkanSwapChain::initSurface(ANativeWindow* window)
#elif defined(VK_USE_PLATFORM_DIRECTFB_EXT)
void VulkanSwapChain::initSurface(IDirectFB* dfb, IDirectFBSurface* window)
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
void VulkanSwapChain::initSurface(wl_display *display, wl_surface *window)
#elif defined(VK_USE_PLATFORM_XCB_KHR)
void VulkanSwapChain::initSurface(xcb_connection_t* connection, xcb_window_t window)
#elif (defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK))
void VulkanSwapChain::initSurface(void* view)
#elif defined(VK_USE_PLATFORM_METAL_EXT)
void VulkanSwapChain::initSurface(CAMetalLayer* metalLayer)
#elif (defined(_DIRECT2DISPLAY) || defined(VK_USE_PLATFORM_HEADLESS_EXT))
void VulkanSwapChain::initSurface(uint32_t width, uint32_t height)
#elif defined(VK_USE_PLATFORM_SCREEN_QNX)
void VulkanSwapChain::initSurface(screen_context_t screen_context, screen_window_t screen_window)
#endif
{
	VkResult err = VK_SUCCESS;

	// Create the os-specific surface
#if defined(VK_USE_PLATFORM_WIN32_KHR)
	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo{};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surfaceCreateInfo.hinstance = (HINSTANCE)platformHandle;
	surfaceCreateInfo.hwnd = (HWND)platformWindow;
	err = vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface);
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
	VkAndroidSurfaceCreateInfoKHR surfaceCreateInfo = {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
	surfaceCreateInfo.window = window;
	err = vkCreateAndroidSurfaceKHR(instance, &surfaceCreateInfo, NULL, &surface);
#elif defined(VK_USE_PLATFORM_IOS_MVK)
	VkIOSSurfaceCreateInfoMVK surfaceCreateInfo = {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_IOS_SURFACE_CREATE_INFO_MVK;
	surfaceCreateInfo.pNext = NULL;
	surfaceCreateInfo.flags = 0;
	surfaceCreateInfo.pView = view;
	err = vkCreateIOSSurfaceMVK(instance, &surfaceCreateInfo, nullptr, &surface);
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
	VkMacOSSurfaceCreateInfoMVK surfaceCreateInfo = {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
	surfaceCreateInfo.pNext = NULL;
	surfaceCreateInfo.flags = 0;
	surfaceCreateInfo.pView = view;
	err = vkCreateMacOSSurfaceMVK(instance, &surfaceCreateInfo, NULL, &surface);
#elif defined(VK_USE_PLATFORM_METAL_EXT)
	VkMetalSurfaceCreateInfoEXT surfaceCreateInfo = {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
	surfaceCreateInfo.pNext = NULL;
	surfaceCreateInfo.flags = 0;
	surfaceCreateInfo.pLayer = metalLayer;
	err = vkCreateMetalSurfaceEXT(instance, &surfaceCreateInfo, NULL, &surface);
#elif defined(_DIRECT2DISPLAY)
	createDirect2DisplaySurface(width, height);
#elif defined(VK_USE_PLATFORM_DIRECTFB_EXT)
	VkDirectFBSurfaceCreateInfoEXT surfaceCreateInfo = {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_DIRECTFB_SURFACE_CREATE_INFO_EXT;
	surfaceCreateInfo.dfb = dfb;
	surfaceCreateInfo.surface = window;
	err = vkCreateDirectFBSurfaceEXT(instance, &surfaceCreateInfo, nullptr, &surface);
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
	VkWaylandSurfaceCreateInfoKHR surfaceCreateInfo = {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
	surfaceCreateInfo.display = display;
	surfaceCreateInfo.surface = window;
	err = vkCreateWaylandSurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface);
#elif defined(VK_USE_PLATFORM_XCB_KHR)
	VkXcbSurfaceCreateInfoKHR surfaceCreateInfo = {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
	surfaceCreateInfo.connection = connection;
	surfaceCreateInfo.window = window;
	err = vkCreateXcbSurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface);
#elif defined(VK_USE_PLATFORM_HEADLESS_EXT)
	VkHeadlessSurfaceCreateInfoEXT surfaceCreateInfo = {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_HEADLESS_SURFACE_CREATE_INFO_EXT;
	PFN_vkCreateHeadlessSurfaceEXT fpCreateHeadlessSurfaceEXT = (PFN_vkCreateHeadlessSurfaceEXT)vkGetInstanceProcAddr(instance, "vkCreateHeadlessSurfaceEXT");
	if (!fpCreateHeadlessSurfaceEXT){
		vks::tools::exitFatal("Could not fetch function pointer for the headless extension!", -1);
	}
	err = fpCreateHeadlessSurfaceEXT(instance, &surfaceCreateInfo, nullptr, &surface);
#elif defined(VK_USE_PLATFORM_SCREEN_QNX)
	VkScreenSurfaceCreateInfoQNX surfaceCreateInfo = {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_SCREEN_SURFACE_CREATE_INFO_QNX;
	surfaceCreateInfo.pNext = NULL;
	surfaceCreateInfo.flags = 0;
	surfaceCreateInfo.context = screen_context;
	surfaceCreateInfo.window = screen_window;
	err = vkCreateScreenSurfaceQNX(instance, &surfaceCreateInfo, NULL, &surface);
#endif

	if (err != VK_SUCCESS) {
		vks::tools::exitFatal("Could not create surface!", err);
	}

	// Get available queue family properties
	uint32_t queueCount;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, nullptr);
	assert(queueCount >= 1);
	std::vector<VkQueueFamilyProperties> queueProps(queueCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, queueProps.data());

	// Iterate over each queue to learn whether it supports presenting:
	// Find a queue with present support
	// Will be used to present the swap chain images to the windowing system
	std::vector<VkBool32> supportsPresent(queueCount);
	for (uint32_t i = 0; i < queueCount; i++) 
	{
		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &supportsPresent[i]);
	}

	// Search for a graphics and a present queue in the array of queue
	// families, try to find one that supports both
	uint32_t graphicsQueueNodeIndex = UINT32_MAX;
	uint32_t presentQueueNodeIndex = UINT32_MAX;
	for (uint32_t i = 0; i < queueCount; i++) {
		if ((queueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)  {
			if (graphicsQueueNodeIndex == UINT32_MAX) {
				graphicsQueueNodeIndex = i;
			}
			if (supportsPresent[i] == VK_TRUE) {
				graphicsQueueNodeIndex = i;
				presentQueueNodeIndex = i;
				break;
			}
		}
	}
	if (presentQueueNodeIndex == UINT32_MAX) 
	{	
		// If there's no queue that supports both present and graphics
		// try to find a separate present queue
		for (uint32_t i = 0; i < queueCount; ++i) {
			if (supportsPresent[i] == VK_TRUE) {
				presentQueueNodeIndex = i;
				break;
			}
		}
	}

	// Exit if either a graphics or a presenting queue hasn't been found
	if (graphicsQueueNodeIndex == UINT32_MAX || presentQueueNodeIndex == UINT32_MAX)  {
		vks::tools::exitFatal("Could not find a graphics and/or presenting queue!", -1);
	}
	if (graphicsQueueNodeIndex != presentQueueNodeIndex) {
		vks::tools::exitFatal("Separate graphics and presenting queues are not supported yet!", -1);
	}
	queueNodeIndex = graphicsQueueNodeIndex;

	// Get list of supported surface formats
	uint32_t formatCount;
	VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, NULL));
	assert(formatCount > 0);
	std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
	VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, surfaceFormats.data()));

	// We want to get a format that best suits our needs, so we try to get one from a set of preferred formats
	// Initialize the format to the first one returned by the implementation in case we can't find one of the preffered formats
	VkSurfaceFormatKHR selectedFormat = surfaceFormats[0];
	std::vector<VkFormat> preferredImageFormats = { 
		VK_FORMAT_B8G8R8A8_UNORM,
		VK_FORMAT_R8G8B8A8_UNORM, 
		VK_FORMAT_A8B8G8R8_UNORM_PACK32 
	};
	for (auto& availableFormat : surfaceFormats) {
		if (std::find(preferredImageFormats.begin(), preferredImageFormats.end(), availableFormat.format) != preferredImageFormats.end()) {
			selectedFormat = availableFormat;
			break;
		}
	}

	colorFormat = selectedFormat.format;
	colorSpace = selectedFormat.colorSpace;
}

void VulkanSwapChain::setContext(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device)
{
	this->instance = instance;
	this->physicalDevice = physicalDevice;
	this->device = device;
}

void VulkanSwapChain::create(uint32_t& width, uint32_t& height, bool vsync, bool fullscreen)
{
	assert(physicalDevice);
	assert(device);
	assert(instance);

	// Store the current swap chain handle so we can use it later on to ease up recreation
	VkSwapchainKHR oldSwapchain = swapChain;

	// Get physical device surface properties and formats
	VkSurfaceCapabilitiesKHR surfaceCaps;
	VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCaps));

	VkExtent2D swapchainExtent = {};
	// If width (and height) equals the special value 0xFFFFFFFF, the size of the surface will be set by the swapchain
	if (surfaceCaps.currentExtent.width == (uint32_t)-1) {
		// If the surface size is undefined, the size is set to the size of the images requested
		swapchainExtent.width = width;
		swapchainExtent.height = height;
	} else {
		// If the surface size is defined, the swap chain size must match
		swapchainExtent = surfaceCaps.currentExtent;
		width = surfaceCaps.currentExtent.width;
		height = surfaceCaps.currentExtent.height;
	}


	// Select a present mode for the swapchain
	uint32_t presentModeCount;
	VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr));
	assert(presentModeCount > 0);

	std::vector<VkPresentModeKHR> presentModes(presentModeCount);
	VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data()));

	// The VK_PRESENT_MODE_FIFO_KHR mode must always be present as per spec
	// This mode waits for the vertical blank ("v-sync")
	VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;

	// If v-sync is not requested, try to find a mailbox mode
	// It's the lowest latency non-tearing present mode available
	if (!vsync)
	{
		for (size_t i = 0; i < presentModeCount; i++)
		{
			if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
				break;
			}
			if (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)
			{
				swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
			}
		}
	}

	// Determine the number of images
	uint32_t desiredNumberOfSwapchainImages = surfaceCaps.minImageCount + 1;
	if ((surfaceCaps.maxImageCount > 0) && (desiredNumberOfSwapchainImages > surfaceCaps.maxImageCount)) {
		desiredNumberOfSwapchainImages = surfaceCaps.maxImageCount;
	}

	// Find the transformation of the surface
	VkSurfaceTransformFlagsKHR preTransform;
	if (surfaceCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
		// We prefer a non-rotated transform
		preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	} else {
		preTransform = surfaceCaps.currentTransform;
	}

	// Find a supported composite alpha format (not all devices support alpha opaque)
	VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	// Simply select the first composite alpha format available
	std::vector<VkCompositeAlphaFlagBitsKHR> compositeAlphaFlags = {
		VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
		VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
		VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
	};
	for (auto& compositeAlphaFlag : compositeAlphaFlags) {
		if (surfaceCaps.supportedCompositeAlpha & compositeAlphaFlag) {
			compositeAlpha = compositeAlphaFlag;
			break;
		};
	}

	VkSwapchainCreateInfoKHR swapchainCI{
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = surface,
		.minImageCount = desiredNumberOfSwapchainImages,
		.imageFormat = colorFormat,
		.imageColorSpace = colorSpace,
		.imageExtent = { swapchainExtent.width, swapchainExtent.height },
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.preTransform = (VkSurfaceTransformFlagBitsKHR)preTransform,
		.compositeAlpha = compositeAlpha,
		.presentMode = swapchainPresentMode,
		// Setting clipped to VK_TRUE allows the implementation to discard rendering outside of the surface area
		.clipped = VK_TRUE,
		// Setting oldSwapChain to the saved handle of the previous swapchain aids in resource reuse and makes sure that we can still present already acquired images
		.oldSwapchain = oldSwapchain,
	};
	// Enable transfer source on swap chain images if supported
	if (surfaceCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
		swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	}
	// Enable transfer destination on swap chain images if supported
	if (surfaceCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
		swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}
	VK_CHECK_RESULT(vkCreateSwapchainKHR(device, &swapchainCI, nullptr, &swapChain));

	// If an existing swap chain is re-created, destroy the old swap chain and the ressources owned by the application (image views, images are owned by the swap chain)
	if (oldSwapchain != VK_NULL_HANDLE) { 
		for (auto i = 0; i < images.size(); i++) {
			vkDestroyImageView(device, imageViews[i], nullptr);
		}
		vkDestroySwapchainKHR(device, oldSwapchain, nullptr);
	}
	// Get the (new) swap chain images
	VK_CHECK_RESULT(vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr));
	images.resize(imageCount);
	VK_CHECK_RESULT(vkGetSwapchainImagesKHR(device, swapChain, &imageCount, images.data()));

	// Get the swap chain buffers containing the image and imageview
	imageViews.resize(imageCount);
	for (auto i = 0; i < images.size(); i++)
	{
		VkImageViewCreateInfo colorAttachmentView{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = images[i],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = colorFormat,
			.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A },
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			},
		};
		VK_CHECK_RESULT(vkCreateImageView(device, &colorAttachmentView, nullptr, &imageViews[i]));
	}
}

VkResult VulkanSwapChain::acquireNextImage(VkSemaphore presentCompleteSemaphore, uint32_t& imageIndex)
{
	// By setting timeout to UINT64_MAX we will always wait until the next image has been acquired or an actual error is thrown
	// With that we don't have to handle VK_NOT_READY
	return vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, presentCompleteSemaphore, (VkFence)nullptr, &imageIndex);
}
void VulkanSwapChain::cleanup()
{
	if (swapChain != VK_NULL_HANDLE) {
		for (auto i = 0; i < images.size(); i++) {
			vkDestroyImageView(device, imageViews[i], nullptr);
		}
		vkDestroySwapchainKHR(device, swapChain, nullptr);
	}
	if (surface != VK_NULL_HANDLE) {
		vkDestroySurfaceKHR(instance, surface, nullptr);
	}
	surface = VK_NULL_HANDLE;
	swapChain = VK_NULL_HANDLE;
}

#if defined(_DIRECT2DISPLAY)
/**
* Create direct to display surface
*/	
void VulkanSwapChain::createDirect2DisplaySurface(uint32_t width, uint32_t height)
{
	uint32_t displayPropertyCount;
		
	// Get display property
	vkGetPhysicalDeviceDisplayPropertiesKHR(physicalDevice, &displayPropertyCount, NULL);
	VkDisplayPropertiesKHR* pDisplayProperties = new VkDisplayPropertiesKHR[displayPropertyCount];
	vkGetPhysicalDeviceDisplayPropertiesKHR(physicalDevice, &displayPropertyCount, pDisplayProperties);

	// Get plane property
	uint32_t planePropertyCount;
	vkGetPhysicalDeviceDisplayPlanePropertiesKHR(physicalDevice, &planePropertyCount, NULL);
	VkDisplayPlanePropertiesKHR* pPlaneProperties = new VkDisplayPlanePropertiesKHR[planePropertyCount];
	vkGetPhysicalDeviceDisplayPlanePropertiesKHR(physicalDevice, &planePropertyCount, pPlaneProperties);

	VkDisplayKHR display = VK_NULL_HANDLE;
	VkDisplayModeKHR displayMode;
	VkDisplayModePropertiesKHR* pModeProperties;
	bool foundMode = false;

	for(uint32_t i = 0; i < displayPropertyCount;++i)
	{
		display = pDisplayProperties[i].display;
		uint32_t modeCount;
		vkGetDisplayModePropertiesKHR(physicalDevice, display, &modeCount, NULL);
		pModeProperties = new VkDisplayModePropertiesKHR[modeCount];
		vkGetDisplayModePropertiesKHR(physicalDevice, display, &modeCount, pModeProperties);

		for (uint32_t j = 0; j < modeCount; ++j)
		{
			const VkDisplayModePropertiesKHR* mode = &pModeProperties[j];

			if (mode->parameters.visibleRegion.width == width && mode->parameters.visibleRegion.height == height)
			{
				displayMode = mode->displayMode;
				foundMode = true;
				break;
			}
		}
		if (foundMode)
		{
			break;
		}
		delete [] pModeProperties;
	}

	if(!foundMode)
	{
		vks::tools::exitFatal("Can't find a display and a display mode!", -1);
		return;
	}

	// Search for a best plane we can use
	uint32_t bestPlaneIndex = UINT32_MAX;
	VkDisplayKHR* pDisplays = NULL;
	for(uint32_t i = 0; i < planePropertyCount; i++)
	{
		uint32_t planeIndex=i;
		uint32_t displayCount;
		vkGetDisplayPlaneSupportedDisplaysKHR(physicalDevice, planeIndex, &displayCount, NULL);
		if (pDisplays)
		{
			delete [] pDisplays;
		}
		pDisplays = new VkDisplayKHR[displayCount];
		vkGetDisplayPlaneSupportedDisplaysKHR(physicalDevice, planeIndex, &displayCount, pDisplays);

		// Find a display that matches the current plane
		bestPlaneIndex = UINT32_MAX;
		for(uint32_t j = 0; j < displayCount; j++)
		{
			if(display == pDisplays[j])
			{
				bestPlaneIndex = i;
				break;
			}
		}
		if(bestPlaneIndex != UINT32_MAX)
		{
			break;
		}
	}

	if(bestPlaneIndex == UINT32_MAX)
	{
		vks::tools::exitFatal("Can't find a plane for displaying!", -1);
		return;
	}

	VkDisplayPlaneCapabilitiesKHR planeCap;
	vkGetDisplayPlaneCapabilitiesKHR(physicalDevice, displayMode, bestPlaneIndex, &planeCap);
	VkDisplayPlaneAlphaFlagBitsKHR alphaMode = (VkDisplayPlaneAlphaFlagBitsKHR)0;

	if (planeCap.supportedAlpha & VK_DISPLAY_PLANE_ALPHA_PER_PIXEL_PREMULTIPLIED_BIT_KHR)
	{
		alphaMode = VK_DISPLAY_PLANE_ALPHA_PER_PIXEL_PREMULTIPLIED_BIT_KHR;
	}
	else if (planeCap.supportedAlpha & VK_DISPLAY_PLANE_ALPHA_PER_PIXEL_BIT_KHR)
	{
		alphaMode = VK_DISPLAY_PLANE_ALPHA_PER_PIXEL_BIT_KHR;
	}
	else if (planeCap.supportedAlpha & VK_DISPLAY_PLANE_ALPHA_GLOBAL_BIT_KHR)
	{
		alphaMode = VK_DISPLAY_PLANE_ALPHA_GLOBAL_BIT_KHR;
	}
	else if (planeCap.supportedAlpha & VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR)
	{
		alphaMode = VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR;
	}

	VkDisplaySurfaceCreateInfoKHR surfaceInfo{};
	surfaceInfo.sType = VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR;
	surfaceInfo.pNext = NULL;
	surfaceInfo.flags = 0;
	surfaceInfo.displayMode = displayMode;
	surfaceInfo.planeIndex = bestPlaneIndex;
	surfaceInfo.planeStackIndex = pPlaneProperties[bestPlaneIndex].currentStackIndex;
	surfaceInfo.transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	surfaceInfo.globalAlpha = 1.0;
	surfaceInfo.alphaMode = alphaMode;
	surfaceInfo.imageExtent.width = width;
	surfaceInfo.imageExtent.height = height;

	VkResult result = vkCreateDisplayPlaneSurfaceKHR(instance, &surfaceInfo, NULL, &surface);
	if (result !=VK_SUCCESS) {
		vks::tools::exitFatal("Failed to create surface!", result);
	}

	delete[] pDisplays;
	delete[] pModeProperties;
	delete[] pDisplayProperties;
	delete[] pPlaneProperties;
}
#endif 
