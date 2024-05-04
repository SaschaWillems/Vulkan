/*
* Class wrapping access to the swap chain
* 
* A swap chain is a collection of framebuffers used for rendering and presentation to the windowing system
*
* Copyright (C) 2016-2024 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#include <stdlib.h>
#include <string>
#include <assert.h>
#include <stdio.h>
#include <vector>

#include <vulkan/vulkan.h>
#include "VulkanTools.h"

#ifdef __ANDROID__
#include "VulkanAndroid.h"
#endif

#ifdef __APPLE__
#include <sys/utsname.h>
#endif

typedef struct _SwapChainBuffers {
	VkImage image;
	VkImageView view;
} SwapChainBuffer;

class VulkanSwapChain
{
private: 
	VkInstance instance;
	VkDevice device;
	VkPhysicalDevice physicalDevice;
	VkSurfaceKHR surface;
public:
	VkFormat colorFormat;
	VkColorSpaceKHR colorSpace;
	VkSwapchainKHR swapChain = VK_NULL_HANDLE;	
	uint32_t imageCount;
	std::vector<VkImage> images;
	std::vector<SwapChainBuffer> buffers;
	uint32_t queueNodeIndex = UINT32_MAX;

#if defined(VK_USE_PLATFORM_WIN32_KHR)
	void initSurface(void* platformHandle, void* platformWindow);
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
	void initSurface(ANativeWindow* window);
#elif defined(VK_USE_PLATFORM_DIRECTFB_EXT)
	void initSurface(IDirectFB* dfb, IDirectFBSurface* window);
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
	void initSurface(wl_display* display, wl_surface* window);
#elif defined(VK_USE_PLATFORM_XCB_KHR)
	void initSurface(xcb_connection_t* connection, xcb_window_t window);
#elif (defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK))
	void initSurface(void* view);
#elif defined(VK_USE_PLATFORM_METAL_EXT)
	void initSurface(CAMetalLayer* metalLayer);
#elif (defined(_DIRECT2DISPLAY) || defined(VK_USE_PLATFORM_HEADLESS_EXT))
	void initSurface(uint32_t width, uint32_t height);
#if defined(_DIRECT2DISPLAY)
	void createDirect2DisplaySurface(uint32_t width, uint32_t height);
#endif
#elif defined(VK_USE_PLATFORM_SCREEN_QNX)
	void initSurface(screen_context_t screen_context, screen_window_t screen_window);
#endif
	/* Set the Vulkan objects required for swapchain creation and management, must be called before swapchain creation */
	void setContext(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device);
	/**
	* Create the swapchain and get its images with given width and height
	* 
	* @param width Pointer to the width of the swapchain (may be adjusted to fit the requirements of the swapchain)
	* @param height Pointer to the height of the swapchain (may be adjusted to fit the requirements of the swapchain)
	* @param vsync (Optional, default = false) Can be used to force vsync-ed rendering (by using VK_PRESENT_MODE_FIFO_KHR as presentation mode)
	*/
	void create(uint32_t* width, uint32_t* height, bool vsync = false, bool fullscreen = false);
	/**
	* Acquires the next image in the swap chain
	* 
	* @param presentCompleteSemaphore (Optional) Semaphore that is signaled when the image is ready for use
	* @param imageIndex Pointer to the image index that will be increased if the next image could be acquired
	* 
	* @note The function will always wait until the next image has been acquired by setting timeout to UINT64_MAX
	* 
	* @return VkResult of the image acquisition
	*/
	VkResult acquireNextImage(VkSemaphore presentCompleteSemaphore, uint32_t* imageIndex);
	/**
	* Queue an image for presentation
	*
	* @param queue Presentation queue for presenting the image
	* @param imageIndex Index of the swapchain image to queue for presentation
	* @param waitSemaphore (Optional) Semaphore that is waited on before the image is presented (only used if != VK_NULL_HANDLE)
	*
	* @return VkResult of the queue presentation
	*/
	VkResult queuePresent(VkQueue queue, uint32_t imageIndex, VkSemaphore waitSemaphore = VK_NULL_HANDLE);
	/* Free all Vulkan resources acquired by the swapchain */
	void cleanup();
};
