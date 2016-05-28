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
				//std::cout << "WARNING: " << "[" << pLayerPrefix << "] Code " << msgCode << " : " << pMsg << "\n";
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

	namespace DebugMarker
	{
		bool active = false;

		PFN_vkDebugMarkerSetObjectTagEXT pfnDebugMarkerSetObjectTag = VK_NULL_HANDLE;
		PFN_vkDebugMarkerSetObjectNameEXT pfnDebugMarkerSetObjectName = VK_NULL_HANDLE;
		PFN_vkCmdDebugMarkerBeginEXT pfnCmdDebugMarkerBegin = VK_NULL_HANDLE;
		PFN_vkCmdDebugMarkerEndEXT pfnCmdDebugMarkerEnd = VK_NULL_HANDLE;
		PFN_vkCmdDebugMarkerInsertEXT pfnCmdDebugMarkerInsert = VK_NULL_HANDLE;

		void setup(VkDevice device)
		{
			pfnDebugMarkerSetObjectTag = (PFN_vkDebugMarkerSetObjectTagEXT)vkGetDeviceProcAddr(device, "vkDebugMarkerSetObjectTagEXT");
			pfnDebugMarkerSetObjectName = (PFN_vkDebugMarkerSetObjectNameEXT)vkGetDeviceProcAddr(device, "vkDebugMarkerSetObjectNameEXT");
			pfnCmdDebugMarkerBegin = (PFN_vkCmdDebugMarkerBeginEXT)vkGetDeviceProcAddr(device, "vkCmdDebugMarkerBeginEXT");
			pfnCmdDebugMarkerEnd = (PFN_vkCmdDebugMarkerEndEXT)vkGetDeviceProcAddr(device, "vkCmdDebugMarkerEndEXT");
			pfnCmdDebugMarkerInsert = (PFN_vkCmdDebugMarkerInsertEXT)vkGetDeviceProcAddr(device, "vkCmdDebugMarkerInsertEXT");

			// Set flag if at least one function pointer is present
			active = (pfnDebugMarkerSetObjectName != VK_NULL_HANDLE);
		}

		void setObjectName(VkDevice device, uint64_t object, VkDebugReportObjectTypeEXT objectType, const char *name)
		{
			// Check for valid function pointer (may not be present if not running in a debugging application)
			if (pfnDebugMarkerSetObjectName)
			{
				VkDebugMarkerObjectNameInfoEXT nameInfo = {};
				nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
				nameInfo.objectType = objectType;
				nameInfo.object = object;
				nameInfo.pObjectName = name;
				pfnDebugMarkerSetObjectName(device, &nameInfo);
			}
		}

		void setObjectTag(VkDevice device, uint64_t object, VkDebugReportObjectTypeEXT objectType, uint64_t name, size_t tagSize, const void* tag)
		{
			// Check for valid function pointer (may not be present if not running in a debugging application)
			if (pfnDebugMarkerSetObjectTag)
			{
				VkDebugMarkerObjectTagInfoEXT tagInfo = {};
				tagInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_TAG_INFO_EXT;
				tagInfo.objectType = objectType;
				tagInfo.object = object;
				tagInfo.tagName = name;
				tagInfo.tagSize = tagSize;
				tagInfo.pTag = tag;
				pfnDebugMarkerSetObjectTag(device, &tagInfo);
			}
		}

		void beginRegion(VkCommandBuffer cmdbuffer, const char* pMarkerName, glm::vec4 color)
		{
			// Check for valid function pointer (may not be present if not running in a debugging application)
			if (pfnCmdDebugMarkerBegin)
			{
				VkDebugMarkerMarkerInfoEXT markerInfo = {};
				markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
				memcpy(markerInfo.color, &color[0], sizeof(float) * 4);
				markerInfo.pMarkerName = pMarkerName;
				pfnCmdDebugMarkerBegin(cmdbuffer, &markerInfo);
			}
		}

		void insert(VkCommandBuffer cmdbuffer, std::string markerName, glm::vec4 color)
		{
			// Check for valid function pointer (may not be present if not running in a debugging application)
			if (pfnCmdDebugMarkerInsert)
			{
				VkDebugMarkerMarkerInfoEXT markerInfo = {};
				markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
				memcpy(markerInfo.color, &color[0], sizeof(float) * 4);
				markerInfo.pMarkerName = markerName.c_str();
				pfnCmdDebugMarkerInsert(cmdbuffer, &markerInfo);
			}
		}

		void endRegion(VkCommandBuffer cmdBuffer)
		{
			// Check for valid function (may not be present if not runnin in a debugging application)
			if (pfnCmdDebugMarkerEnd)
			{
				pfnCmdDebugMarkerEnd(cmdBuffer);
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
	};

}

