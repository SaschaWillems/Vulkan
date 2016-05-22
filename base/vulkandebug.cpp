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

	PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallback = VK_NULL_HANDLE;
	PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallback = VK_NULL_HANDLE;
	PFN_vkDebugReportMessageEXT dbgBreakCallback = VK_NULL_HANDLE;

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

	namespace debugReport
	{
		PFN_vkDebugMarkerSetObjectNameEXT DebugMarkerSetObjectName = VK_NULL_HANDLE;
		PFN_vkCmdDebugMarkerBeginEXT CmdDebugMarkerBegin = VK_NULL_HANDLE;
		PFN_vkCmdDebugMarkerEndEXT CmdDebugMarkerEnd = VK_NULL_HANDLE;
		PFN_vkCmdDebugMarkerInsertEXT CmdDebugMarkerInsert = VK_NULL_HANDLE;

		// Set up the debug marker function pointers
		void setupDebugMarkers(VkDevice device)
		{
			DebugMarkerSetObjectName = (PFN_vkDebugMarkerSetObjectNameEXT)vkGetDeviceProcAddr(device, "vkDebugMarkerSetObjectNameEXT");
			CmdDebugMarkerBegin = (PFN_vkCmdDebugMarkerBeginEXT)vkGetDeviceProcAddr(device, "vkCmdDebugMarkerBeginEXT");
			CmdDebugMarkerEnd = (PFN_vkCmdDebugMarkerEndEXT)vkGetDeviceProcAddr(device, "vkCmdDebugMarkerEndEXT");
			CmdDebugMarkerInsert = (PFN_vkCmdDebugMarkerInsertEXT)vkGetDeviceProcAddr(device, "vkCmdDebugMarkerInsertEXT");
		}

		void setObjectName(VkDevice device, uint64_t object, VkDebugReportObjectTypeEXT objectType, const char *name)
		{
			// need to check if the function pointer is valid - extension might not be present
			if (DebugMarkerSetObjectName)
			{
				VkDebugMarkerObjectNameInfoEXT nameInfo = {};
				nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
				nameInfo.objectType = objectType;
				nameInfo.object = object;
				nameInfo.pObjectName = name;
				DebugMarkerSetObjectName(device, &nameInfo);
			}
		}

		void setCommandBufferName(VkDevice device, VkCommandBuffer cmdBuffer, const char * name)
		{
			setObjectName(device, (uint64_t)cmdBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, name);
		}

		void setQueueName(VkDevice device, VkQueue queue, const char * name)
		{
			setObjectName(device, (uint64_t)queue, VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT, name);
		}

		void setImageName(VkDevice device, VkImage image, const char * name)
		{
			setObjectName(device, (uint64_t)image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, name);
		}

		void setSamplerName(VkDevice device, VkSampler sampler, const char * name)
		{
			setObjectName(device, (uint64_t)sampler, VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT, name);
		}

		void setBufferName(VkDevice device, VkBuffer buffer, const char * name)
		{
			setObjectName(device, (uint64_t)buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, name);
		}

		void setDeviceMemoryName(VkDevice device, VkDeviceMemory memory, const char * name)
		{
			setObjectName(device, (uint64_t)memory, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, name);
		}

		void setShaderModuleName(VkDevice device, VkShaderModule shaderModule, const char * name)
		{
			setObjectName(device, (uint64_t)shaderModule, VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT, name);
		}

		void setPipelineName(VkDevice device, VkPipeline pipeline, const char * name)
		{
			setObjectName(device, (uint64_t)pipeline, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT, name);
		}

		void setPipelineLayoutName(VkDevice device, VkPipelineLayout pipelineLayout, const char * name)
		{
			setObjectName(device, (uint64_t)pipelineLayout, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT, name);
		}

		void setRenderPassName(VkDevice device, VkRenderPass renderPass, const char * name)
		{
			setObjectName(device, (uint64_t)renderPass, VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT, name);
		}

		void setFramebufferName(VkDevice device, VkFramebuffer framebuffer, const char * name)
		{
			setObjectName(device, (uint64_t)framebuffer, VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT, name);
		}

		void setDescriptorSetLayoutName(VkDevice device, VkDescriptorSetLayout descriptorSetLayout, const char * name)
		{
			setObjectName(device, (uint64_t)descriptorSetLayout, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT, name);
		}

		void setDescriptorSetName(VkDevice device, VkDescriptorSet descriptorSet, const char * name)
		{
			setObjectName(device, (uint64_t)descriptorSet, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT, name);
		}

		void setSemaphoreName(VkDevice device, VkSemaphore semaphore, const char * name)
		{
			setObjectName(device, (uint64_t)semaphore, VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT, name);
		}

		void setFenceName(VkDevice device, VkFence fence, const char * name)
		{
			setObjectName(device, (uint64_t)fence, VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT, name);
		}

		void setEventName(VkDevice device, VkEvent _event, const char * name)
		{
			setObjectName(device, (uint64_t)_event, VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT, name);
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
				memcpy(markerInfo.color, color, sizeof(float) * 4);
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
				memcpy(markerInfo.color, color, sizeof(float) * 4);
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
			{
				CmdDebugMarkerEnd(cmd);
			}
		}
	}
}

