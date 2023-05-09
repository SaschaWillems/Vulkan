/*
 * Vulkan examples debug wrapper
 *
 * Copyright (C) 2016-2023 by Sascha Willems - www.saschawillems.de
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */

#pragma once
#include "vulkan/vulkan.h"

#include <math.h>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <fstream>
#include <assert.h>
#include <stdio.h>
#include <vector>
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#endif
#ifdef __ANDROID__
#include "VulkanAndroid.h"
#endif
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace vks
{
	namespace debug
	{
		// Default debug callback
		VKAPI_ATTR VkBool32 VKAPI_CALL messageCallback(
			VkDebugReportFlagsEXT flags,
			VkDebugReportObjectTypeEXT objType,
			uint64_t srcObject,
			size_t location,
			int32_t msgCode,
			const char* pLayerPrefix,
			const char* pMsg,
			void* pUserData);

		// Load debug function pointers and set debug callback
		void setupDebugging(VkInstance instance);
		// Clear debug callback
		void freeDebugCallback(VkInstance instance);
	}

	// Wrapper for the VK_EXT_debug_utils extension
	// These can be used to name Vulkan objects for debugging tools like RenderDoc
	namespace debugutils
	{
		void setup(VkInstance instance);
		void cmdBeginLabel(VkCommandBuffer cmdbuffer, std::string caption, glm::vec4 color);
		void cmdEndLabel(VkCommandBuffer cmdbuffer);
	}
}
