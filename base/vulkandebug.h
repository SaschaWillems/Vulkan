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

	// Set up the debug marker function pointers
	void setupDebugMarkers(VkDevice device);

	// insert a debug label into the command buffer, with or
	// without a color
	void insertDebugMarker(
		VkCommandBuffer cmdbuffer,
		const char* pMarkerName,
		float color[4]);
	void insertDebugMarker(
		VkCommandBuffer cmdbuffer,
		const char* pMarkerName);

	// helper class for pushing and popping a debug region
	// around some section of code.
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

	// associate a friendly name with an object
	void SetObjectName(
		VkDevice device,
		VkDebugReportObjectTypeEXT objectType,
		uint64_t object,
		const char* pObjectName);

	// specialised in vulkandebug.cpp for each object type
	template<typename VulkanType>
	VkDebugReportObjectTypeEXT GetObjectTypeEnum(VulkanType object);

	// templated helper function for SetObjectName
	template<typename VulkanType>
	void SetObjectName(
		VkDevice device,
		VulkanType object,
		const char* pObjectName)
	{
		SetObjectName(device, GetObjectTypeEnum(object), (uint64_t)object, pObjectName);
	}
}
