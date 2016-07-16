/*
* Vulkan device class
*
* Encapsulates a physical Vulkan device and it's logical representation
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#include <exception>
#include "vulkan/vulkan.h"
#include "vulkantools.h"

namespace vk
{	
	struct VulkanDevice
	{
		/** @brief Physical device representation */
		VkPhysicalDevice physicalDevice;
		/** @brief Logical device representation (application's view of the device) */
		VkDevice device;
		/** @brief Properties of the physical device including limits that the application can check against */
		VkPhysicalDeviceProperties properties;
		/** @brief Features of the physical device that an application can use to check if a feature is supported */
		VkPhysicalDeviceFeatures features;
		/** @brief Memory types and heaps of the physical device */
		VkPhysicalDeviceMemoryProperties memoryProperties;

		/** @brief Set to true when the debug marker extension is detected */
		bool enableDebugMarkers = false;

		/**
		* Return the index of a memory type that has all the requested property bits set
		*
		* @param typeBits Bitmask with bits set for each memory type supported by the resource to request for (from VkMemoryRequirements)
		* @param properties Bitmask of properties for the memory type to request
		*
		* @return Index of the requested memory type
		*
		* @throw Throws an exception if no memory type could be found that supports the requested properties
		*/
		uint32_t getMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties)
		{
			for (uint32_t i = 0; i < memoryProperties.memoryTypeCount - 1; i++)
			{
				if ((typeBits & 1) == 1)
				{
					if ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
					{
						return i;
					}
				}
				typeBits >>= 1;
			}

#if defined(__ANDROID__)
			//todo : Exceptions are disabled by default on Android (need to add LOCAL_CPP_FEATURES += exceptions to Android.mk), so for now just return zero
			return 0;
#else
			throw "Could not find a memory type for the passed properties";
#endif
		}

		/**
		* Create the logical device based on the passed physical device
		*
		* @param physicalDevice The physical device for which the logical reprenstation is to be created
		* @param queueCreateInfos A vector containing queue create infos for all queues to be requested on the device 
		* @param enabledFeatures Can be used to enable certain features upon device creation
		* @param useSwapChain Set to false for headless rendering to omit the swapchain device extensions
		* 
		* @return VkResult of the device creation call
		*/
		VkResult create(VkPhysicalDevice physicalDevice, std::vector<VkDeviceQueueCreateInfo> &queueCreateInfos, VkPhysicalDeviceFeatures enabledFeatures, bool useSwapChain = true)
		{
			this->physicalDevice = physicalDevice;

			// Store Properties features, limits and properties of the physical device for later use
			// Device properties also contain limits and sparse properties
			vkGetPhysicalDeviceProperties(physicalDevice, &properties);
			// Features should be checked by the examples before using them
			vkGetPhysicalDeviceFeatures(physicalDevice, &features);
			// Memory properties are used regularly for creating all kinds of buffer
			vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

			// Create the logical device representation
			std::vector<const char*> deviceExtensions;
			if (useSwapChain)
			{
				// If the device will be used for presenting to a display via a swapchain
				// we need to request the swapchain extension
				deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
			}

			VkDeviceCreateInfo deviceCreateInfo = {};
			deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());;
			deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
			deviceCreateInfo.pEnabledFeatures = &enabledFeatures;

			// Cnable the debug marker extension if it is present (likely meaning a debugging tool is present)
			if (vkTools::checkDeviceExtensionPresent(physicalDevice, VK_EXT_DEBUG_MARKER_EXTENSION_NAME))
			{
				deviceExtensions.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
				enableDebugMarkers = true;
			}

			if (deviceExtensions.size() > 0)
			{
				deviceCreateInfo.enabledExtensionCount = (uint32_t)deviceExtensions.size();
				deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
			}

			return vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device);
		}

		/**
		* Create a buffer on the device
		*
		* @param usageFlags Usage flag bitmask for the buffer (i.e. index, vertex, uniform buffer)
		* @param memoryPropertyFlags Memory properties for this buffer (i.e. device local, host visible, coherent)
		* @param size Size of the buffer in byes
		* @param buffer Pointer to the buffer handle acquired by the function
		* @param memory Pointer to the memory handle acquired by the function
		* @param data Pointer to the data that should be copied to the buffer after creation (optional, if not set, no data is copied over)
		*
		* @return VK_SUCCESS if buffer handle and memory have been created and (optionally passed) data has been copied
		*/
		VkResult createBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceSize size, VkBuffer *buffer, VkDeviceMemory *memory, void *data = nullptr)
		{
			// Create the buffer handle
			VkBufferCreateInfo bufferCreateInfo = vkTools::initializers::bufferCreateInfo(usageFlags, size);
			VK_CHECK_RESULT(vkCreateBuffer(device, &bufferCreateInfo, nullptr, buffer));

			// Create the memory backing up the buffer handle
			VkMemoryRequirements memReqs;
			VkMemoryAllocateInfo memAlloc = vkTools::initializers::memoryAllocateInfo();
			vkGetBufferMemoryRequirements(device, *buffer, &memReqs);
			memAlloc.allocationSize = memReqs.size;
			// Find a memory type index that fits the properties of the buffer
			memAlloc.memoryTypeIndex = getMemoryType(memReqs.memoryTypeBits, memoryPropertyFlags);
			VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, memory));
			
			// If a pointer to the buffer data has been passed, map the buffer and copy over the data
			if (data != nullptr)
			{
				void *mapped;
				VK_CHECK_RESULT(vkMapMemory(device, *memory, 0, size, 0, &mapped));
				memcpy(mapped, data, size);
				vkUnmapMemory(device, *memory);
			}

			// Attach the memory to the buffer object
			VK_CHECK_RESULT(vkBindBufferMemory(device, *buffer, *memory, 0));

			return VK_SUCCESS;
		}
	};
}
