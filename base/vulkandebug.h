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
#ifdef _WIN32
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#endif
#ifdef __ANDROID__
#include "vulkanandroid.h"
#endif

namespace vkDebug
{
	// Default validation layers
	extern int validationLayerCount;
	extern const char *validationLayerNames[];

	// Default debug callback
	VkBool32 messageCallback(
		VkDebugReportFlagsEXT flags,
		VkDebugReportObjectTypeEXT objType,
		uint64_t srcObject,
		size_t location,
		int32_t msgCode,
		const char* pLayerPrefix,
		const char* pMsg,
		void* pUserData);

	// Load debug function pointers and set debug callback
	// if callBack is NULL, default message callback will be used
	void setupDebugging(
		VkInstance instance, 
		VkDebugReportFlagsEXT flags, 
		VkDebugReportCallbackEXT callBack);
	// Clear debug callback
	void freeDebugCallback(VkInstance instance);

	// Functions for the VK_EXT_debug_report extensions
	namespace debugReport
	{
		// Set up the debug marker function pointers
		void setupDebugMarkers(VkDevice device);

		// Functions for naming different Vulkan object types
		// General
		void setObjectName(VkDevice device, uint64_t object, VkDebugReportObjectTypeEXT objectType, const char *name);
		// Dedicated object type
		void setCommandBufferName(VkDevice device, VkCommandBuffer cmdBuffer, const char *name);
		void setQueueName(VkDevice device, VkQueue queue, const char *name);
		void setImageName(VkDevice device, VkImage image, const char *name);
		void setSamplerName(VkDevice device, VkSampler sampler, const char *name);
		void setBufferName(VkDevice device, VkBuffer buffer, const char *name);
		void setDeviceMemoryName(VkDevice device, VkDeviceMemory memory, const char *name);
		void setShaderModuleName(VkDevice device, VkShaderModule shaderModule, const char *name);
		void setPipelineName(VkDevice device, VkPipeline pipeline, const char *name);
		void setPipelineLayoutName(VkDevice device, VkPipelineLayout pipelineLayout, const char *name);
		void setRenderPassName(VkDevice device, VkRenderPass renderPass, const char *name);
		void setFramebufferName(VkDevice device, VkFramebuffer framebuffer, const char *name);
		void setDescriptorSetLayoutName(VkDevice device, VkDescriptorSetLayout descriptorSetLayout, const char *name);
		void setDescriptorSetName(VkDevice device, VkDescriptorSet descriptorSet, const char *name);
		void setSemaphoreName(VkDevice device, VkSemaphore semaphore, const char *name);
		void setFenceName(VkDevice device, VkFence fence, const char *name);
		void setEventName(VkDevice device, VkEvent _event, const char *name);

		/*

		OBJECT_TYPE(DESCRIPTOR_POOL, VkDescriptorPool);

		OBJECT_TYPE(COMMAND_POOL, VkCommandPool);

		OBJECT_TYPE(QUERY_POOL, VkQueryPool);

		OBJECT_TYPE(BUFFER_VIEW, VkBufferView);
		OBJECT_TYPE(IMAGE_VIEW, VkImageView);
		OBJECT_TYPE(PIPELINE_CACHE, VkPipelineCache);
		*/

		// insert a debug label into the command buffer, with or
		// without a color
		void insertDebugMarker(
			VkCommandBuffer cmdbuffer,
			const char* pMarkerName,
			float color[4]);
		void insertDebugMarker(
			VkCommandBuffer cmdbuffer,
			const char* pMarkerName);

		// Helper class for pushing and popping a debug region around some section of code.
		struct DebugMarkerRegion
		{
			DebugMarkerRegion(VkCommandBuffer cmdbuffer,
				const char* pMarkerName,
				float color[4]);
			DebugMarkerRegion(VkCommandBuffer cmdbuffer,
				const char* pMarkerName);
			~DebugMarkerRegion();

			VkCommandBuffer cmd;
		};
	}
}
