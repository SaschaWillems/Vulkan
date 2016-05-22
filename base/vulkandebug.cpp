/*
* Vulkan examples debug wrapper
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "vulkandebug.h"
#include <iostream>

namespace vkDebug
{
	int validationLayerCount = 1;
	const char *validationLayerNames[] = 
	{
		// This is a meta layer that enables all of the standard
		// validation layers in the correct order :
		// threading, parameter_validation, device_limits, object_tracker, image, core_validation, swapchain, and unique_objects
		"VK_LAYER_LUNARG_standard_validation"
	};

	PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallback;
	PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallback;
	PFN_vkDebugReportMessageEXT dbgBreakCallback;

	VkDebugReportCallbackEXT msgCallback;

	VkBool32 messageCallback(
		VkDebugReportFlagsEXT flags,
		VkDebugReportObjectTypeEXT objType,
		uint64_t srcObject,
		size_t location,
		int32_t msgCode,
		const char* pLayerPrefix,
		const char* pMsg,
		void* pUserData)
	{
		char *message = (char *)malloc(strlen(pMsg) + 100);

		assert(message);

		if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
		{
			std::cout << "ERROR: " << "[" << pLayerPrefix << "] Code " << msgCode << " : " << pMsg << "\n";
		}
		else
			if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
			{
				// Uncomment to see warnings
				std::cout << "WARNING: " << "[" << pLayerPrefix << "] Code " << msgCode << " : " << pMsg << "\n";
			}
			else
			{
				return false;
			}

		fflush(stdout);

		free(message);
		return false;
	}

	void setupDebugging(VkInstance instance, VkDebugReportFlagsEXT flags, VkDebugReportCallbackEXT callBack)
	{
		CreateDebugReportCallback = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
		DestroyDebugReportCallback = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
		dbgBreakCallback = (PFN_vkDebugReportMessageEXT)vkGetInstanceProcAddr(instance, "vkDebugReportMessageEXT");

		VkDebugReportCallbackCreateInfoEXT dbgCreateInfo = {};
		dbgCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
		dbgCreateInfo.pfnCallback = (PFN_vkDebugReportCallbackEXT)messageCallback;
		dbgCreateInfo.flags = flags;

		VkResult err = CreateDebugReportCallback(
			instance,
			&dbgCreateInfo,
			nullptr,
			(callBack != VK_NULL_HANDLE) ? &callBack : &msgCallback);
		assert(!err);
	}
	
	void freeDebugCallback(VkInstance instance)
	{
		if (msgCallback != VK_NULL_HANDLE)
		{
			DestroyDebugReportCallback(instance, msgCallback, nullptr);
		}
	}

	PFN_vkDebugMarkerSetObjectNameEXT DebugMarkerSetObjectName = NULL;
	PFN_vkCmdDebugMarkerBeginEXT CmdDebugMarkerBegin = NULL;
	PFN_vkCmdDebugMarkerEndEXT CmdDebugMarkerEnd = NULL;
	PFN_vkCmdDebugMarkerInsertEXT CmdDebugMarkerInsert = NULL;

	// Set up the debug marker function pointers
	void setupDebugMarkers(VkDevice device)
	{
		DebugMarkerSetObjectName = (PFN_vkDebugMarkerSetObjectNameEXT)vkGetDeviceProcAddr(device, "vkDebugMarkerSetObjectNameEXT");
		CmdDebugMarkerBegin = (PFN_vkCmdDebugMarkerBeginEXT)vkGetDeviceProcAddr(device, "vkCmdDebugMarkerBeginEXT");
		CmdDebugMarkerEnd = (PFN_vkCmdDebugMarkerEndEXT)vkGetDeviceProcAddr(device, "vkCmdDebugMarkerEndEXT");
		CmdDebugMarkerInsert = (PFN_vkCmdDebugMarkerInsertEXT)vkGetDeviceProcAddr(device, "vkCmdDebugMarkerInsertEXT");
	}

	void insertDebugMarker(
		VkCommandBuffer cmdbuffer,
		const char* pMarkerName,
		float color[4])
	{
		// need to check if the function pointer is valid - extension might not be present
		if (CmdDebugMarkerInsert)
		{
			VkDebugMarkerMarkerInfoEXT markerInfo = {};
			markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
			memcpy(markerInfo.color, color, sizeof(float)*4);
			markerInfo.pMarkerName = pMarkerName;
			CmdDebugMarkerInsert(cmdbuffer, &markerInfo);
		}
	}

	void insertDebugMarker(
		VkCommandBuffer cmdbuffer,
		const char* pMarkerName)
	{
		// need to check if the function pointer is valid - extension might not be present
		if (CmdDebugMarkerInsert)
		{
			VkDebugMarkerMarkerInfoEXT markerInfo = {};
			markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
			markerInfo.pMarkerName = pMarkerName;
			CmdDebugMarkerInsert(cmdbuffer, &markerInfo);
		}
	}

	DebugMarkerRegion::DebugMarkerRegion(VkCommandBuffer cmdbuffer,
		const char* pMarkerName,
		float color[4])
	{
		cmd = cmdbuffer;
		// need to check if the function pointer is valid - extension might not be present
		if (CmdDebugMarkerBegin)
		{
			VkDebugMarkerMarkerInfoEXT markerInfo = {};
			markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
			memcpy(markerInfo.color, color, sizeof(float)*4);
			markerInfo.pMarkerName = pMarkerName;
			CmdDebugMarkerBegin(cmd, &markerInfo);
		}
	}

	DebugMarkerRegion::DebugMarkerRegion(VkCommandBuffer cmdbuffer,
		const char* pMarkerName)
	{
		cmd = cmdbuffer;
		// need to check if the function pointer is valid - extension might not be present
		if (CmdDebugMarkerBegin)
		{
			VkDebugMarkerMarkerInfoEXT markerInfo = {};
			markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
			markerInfo.pMarkerName = pMarkerName;
			CmdDebugMarkerBegin(cmd, &markerInfo);
		}
	}

	DebugMarkerRegion::~DebugMarkerRegion()
	{
		// need to check if the function pointer is valid - extension might not be present
		if (CmdDebugMarkerEnd)
			CmdDebugMarkerEnd(cmd);
	}

	// we specialise this template for each object type
	template<typename VulkanType>
	VkDebugReportObjectTypeEXT GetObjectTypeEnum(VulkanType object);

#define OBJECT_TYPE(enumName, objectType) template<> VkDebugReportObjectTypeEXT GetObjectTypeEnum(objectType o) { (void)o; return VK_DEBUG_REPORT_OBJECT_TYPE_ ##enumName ##_EXT; }
	OBJECT_TYPE(INSTANCE, VkInstance);
	OBJECT_TYPE(PHYSICAL_DEVICE, VkPhysicalDevice);
	OBJECT_TYPE(DEVICE, VkDevice);
	OBJECT_TYPE(QUEUE, VkQueue);
	OBJECT_TYPE(SEMAPHORE, VkSemaphore);
	OBJECT_TYPE(COMMAND_BUFFER, VkCommandBuffer);
	OBJECT_TYPE(FENCE, VkFence);
	OBJECT_TYPE(DEVICE_MEMORY, VkDeviceMemory);
	OBJECT_TYPE(BUFFER, VkBuffer);
	OBJECT_TYPE(IMAGE, VkImage);
	OBJECT_TYPE(EVENT, VkEvent);
	OBJECT_TYPE(QUERY_POOL, VkQueryPool);
	OBJECT_TYPE(BUFFER_VIEW, VkBufferView);
	OBJECT_TYPE(IMAGE_VIEW, VkImageView);
	OBJECT_TYPE(SHADER_MODULE, VkShaderModule);
	OBJECT_TYPE(PIPELINE_CACHE, VkPipelineCache);
	OBJECT_TYPE(PIPELINE_LAYOUT, VkPipelineLayout);
	OBJECT_TYPE(RENDER_PASS, VkRenderPass);
	OBJECT_TYPE(PIPELINE, VkPipeline);
	OBJECT_TYPE(DESCRIPTOR_SET_LAYOUT, VkDescriptorSetLayout);
	OBJECT_TYPE(SAMPLER, VkSampler);
	OBJECT_TYPE(DESCRIPTOR_POOL, VkDescriptorPool);
	OBJECT_TYPE(DESCRIPTOR_SET, VkDescriptorSet);
	OBJECT_TYPE(FRAMEBUFFER, VkFramebuffer);
	OBJECT_TYPE(COMMAND_POOL, VkCommandPool);
	OBJECT_TYPE(SURFACE_KHR, VkSurfaceKHR);
	OBJECT_TYPE(SWAPCHAIN_KHR, VkSwapchainKHR);
	OBJECT_TYPE(DEBUG_REPORT, VkDebugReportCallbackEXT);                       
#undef OBJECT_TYPE

	void SetObjectName(
		VkDevice device,
		VkDebugReportObjectTypeEXT objectType,
		uint64_t object,
		const char* pObjectName)
	{
		// need to check if the function pointer is valid - extension might not be present
		if (DebugMarkerSetObjectName)
		{
			VkDebugMarkerObjectNameInfoEXT nameInfo = {};
			nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
			nameInfo.objectType = objectType;
			nameInfo.object = object;
			nameInfo.pObjectName = pObjectName;
			DebugMarkerSetObjectName(device, &nameInfo);
		}
	}
}