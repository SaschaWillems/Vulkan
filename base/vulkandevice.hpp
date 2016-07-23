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
#include "vulkanbuffer.hpp"

namespace vk
{	
	struct VulkanDevice
	{
		/** @brief Physical device representation */
		VkPhysicalDevice physicalDevice;
		/** @brief Logical device representation (application's view of the device) */
		VkDevice logicalDevice;
		/** @brief Properties of the physical device including limits that the application can check against */
		VkPhysicalDeviceProperties properties;
		/** @brief Features of the physical device that an application can use to check if a feature is supported */
		VkPhysicalDeviceFeatures features;
		/** @brief Memory types and heaps of the physical device */
		VkPhysicalDeviceMemoryProperties memoryProperties;

		/** @brief Default command pool for the graphics queue family index */
		VkCommandPool commandPool = VK_NULL_HANDLE;

		/** @brief Set to true when the debug marker extension is detected */
		bool enableDebugMarkers = false;

		/**  @brief Contains queue family indices */
		struct
		{
			uint32_t graphics = 0;
			uint32_t compute = 0;
		} queueFamilyIndices;

		/**
		* Default constructor
		*
		* @param physicalDevice Phyiscal device that is to be used
		*/
		VulkanDevice(VkPhysicalDevice physicalDevice)
		{
			assert(physicalDevice);
			this->physicalDevice = physicalDevice;

			// Store Properties features, limits and properties of the physical device for later use
			// Device properties also contain limits and sparse properties
			vkGetPhysicalDeviceProperties(physicalDevice, &properties);
			// Features should be checked by the examples before using them
			vkGetPhysicalDeviceFeatures(physicalDevice, &features);
			// Memory properties are used regularly for creating all kinds of buffer
			vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
		}

		/** 
		* Default destructor
		*
		* @note Frees the logical device
		*/
		~VulkanDevice()
		{
			if (commandPool)
			{
				vkDestroyCommandPool(logicalDevice, commandPool, nullptr);
			}
			if (logicalDevice)
			{
				vkDestroyDevice(logicalDevice, nullptr);
			}
		}

		/**
		* Get the index of a memory type that has all the requested property bits set
		*
		* @param typeBits Bitmask with bits set for each memory type supported by the resource to request for (from VkMemoryRequirements)
		* @param properties Bitmask of properties for the memory type to request
		* @param (Optional) memTypeFound Pointer to a bool that is set to true if a matching memory type has been found
		* 
		* @return Index of the requested memory type
		*
		* @throw Throws an exception if memTypeFound is null and no memory type could be found that supports the requested properties
		*/
		uint32_t getMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties, VkBool32 *memTypeFound = nullptr)
		{
			for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
			{
				if ((typeBits & 1) == 1)
				{
					if ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
					{
						if (memTypeFound)
						{
							*memTypeFound = true;
						}
						return i;
					}
				}
				typeBits >>= 1;
			}

#if defined(__ANDROID__)
			//todo : Exceptions are disabled by default on Android (need to add LOCAL_CPP_FEATURES += exceptions to Android.mk), so for now just return zero
			if (memTypeFound)
			{
				*memTypeFound = false;
			}
			return 0;
#else
			if (memTypeFound)
			{
				*memTypeFound = false;
			}
			else
			{
				throw std::runtime_error("Could not find a matching memory type");
			}
#endif
		}

		/**
		* Get the index of a queue family that supports the requested queue flags
		*
		* @param queueFlags Queue flags to find a queue family index for
		*
		* @return INdex
		*
		* @throw Throws an exception if no queue family index could be found that supports the requested flags
		*/
		uint32_t getQueueFamiliyIndex(VkQueueFlagBits queueFlags)
		{
			uint32_t queueCount;

			// Get number of available queue families on this device
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, NULL);
			assert(queueCount >= 1);

			// Get available queue families
			std::vector<VkQueueFamilyProperties> queueProps;
			queueProps.resize(queueCount);
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, queueProps.data());

			for (uint32_t i = 0; i < queueCount; i++)
			{
				if (queueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
				{
					return i;
					break;
				}
			}

			// todo: Advanced search for devices that have dedicated queues for compute and transfer
			//       Try to find queues with only the requested flags or (if not present) with as few 
			//       other flags set as possible (example: http://vulkan.gpuinfo.org/displayreport.php?id=509#queuefamilies)


#if defined(__ANDROID__)
			//todo : Exceptions are disabled by default on Android (need to add LOCAL_CPP_FEATURES += exceptions to Android.mk), so for now just return zero
			return 0;
#else
			throw std::runtime_error("Could not find a matching queue family index");
#endif
		}

		/**
		* Create the logical device based on the assigned physical device, also gets default queue family indices
		*
		* @param enabledFeatures Can be used to enable certain features upon device creation
		* @param useSwapChain Set to false for headless rendering to omit the swapchain device extensions
		*
		* @return VkResult of the device creation call
		*/
		VkResult createLogicalDevice(VkPhysicalDeviceFeatures enabledFeatures, bool useSwapChain = true)
		{
			// Get queue family indices for graphics and compute
			// Note that the indices may overlap depending on the implementation
			queueFamilyIndices.graphics = getQueueFamiliyIndex(VK_QUEUE_GRAPHICS_BIT);
			queueFamilyIndices.compute = getQueueFamiliyIndex(VK_QUEUE_COMPUTE_BIT);
			//todo: Transfer?

			// Pass queue information for graphics and compute, so examples can later on request queues from both
			std::vector<float> queuePriorities;

			std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};
			// We need one queue create info per queue family index
			// If graphics and compute share the same queue family index we only need one queue create info but
			// with two queues to request
			queueCreateInfos.resize(1);
			// Graphics
			queuePriorities.push_back(0.0f);
			queueCreateInfos[0] = {};
			queueCreateInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfos[0].queueFamilyIndex = queueFamilyIndices.graphics;
			queueCreateInfos[0].queueCount = 1;
			// Compute
			// If compute has a different queue family index, add another create info, else just add
			if (queueFamilyIndices.graphics != queueFamilyIndices.compute)
			{
				queueCreateInfos.resize(2);
				queueCreateInfos[1] = {};
				queueCreateInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queueCreateInfos[1].queueFamilyIndex = queueFamilyIndices.compute;
				queueCreateInfos[1].queueCount = 1;
				queueCreateInfos[1].pQueuePriorities = queuePriorities.data();
			}
			else
			{
				queueCreateInfos[0].queueCount++;
				queuePriorities.push_back(0.0f);
			}
			queueCreateInfos[0].pQueuePriorities = queuePriorities.data();

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

			return vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &logicalDevice);

			// Create a default command pool for graphics command buffers
			commandPool = createCommandPool(queueFamilyIndices.graphics);
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
			bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			VK_CHECK_RESULT(vkCreateBuffer(logicalDevice, &bufferCreateInfo, nullptr, buffer));

			// Create the memory backing up the buffer handle
			VkMemoryRequirements memReqs;
			VkMemoryAllocateInfo memAlloc = vkTools::initializers::memoryAllocateInfo();
			vkGetBufferMemoryRequirements(logicalDevice, *buffer, &memReqs);
			memAlloc.allocationSize = memReqs.size;
			// Find a memory type index that fits the properties of the buffer
			memAlloc.memoryTypeIndex = getMemoryType(memReqs.memoryTypeBits, memoryPropertyFlags);
			VK_CHECK_RESULT(vkAllocateMemory(logicalDevice, &memAlloc, nullptr, memory));
			
			// If a pointer to the buffer data has been passed, map the buffer and copy over the data
			if (data != nullptr)
			{
				void *mapped;
				VK_CHECK_RESULT(vkMapMemory(logicalDevice, *memory, 0, size, 0, &mapped));
				memcpy(mapped, data, size);
				vkUnmapMemory(logicalDevice, *memory);
			}

			// Attach the memory to the buffer object
			VK_CHECK_RESULT(vkBindBufferMemory(logicalDevice, *buffer, *memory, 0));

			return VK_SUCCESS;
		}

		/**
		* Create a buffer on the device
		*
		* @param usageFlags Usage flag bitmask for the buffer (i.e. index, vertex, uniform buffer)
		* @param memoryPropertyFlags Memory properties for this buffer (i.e. device local, host visible, coherent)
		* @param buffer Pointer to a vk::Vulkan buffer object
		* @param size Size of the buffer in byes
		* @param data Pointer to the data that should be copied to the buffer after creation (optional, if not set, no data is copied over)
		*
		* @return VK_SUCCESS if buffer handle and memory have been created and (optionally passed) data has been copied
		*/
		VkResult createBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, vk::Buffer *buffer, VkDeviceSize size, void *data = nullptr)
		{
			buffer->device = logicalDevice;

			// Create the buffer handle
			VkBufferCreateInfo bufferCreateInfo = vkTools::initializers::bufferCreateInfo(usageFlags, size);
			VK_CHECK_RESULT(vkCreateBuffer(logicalDevice, &bufferCreateInfo, nullptr, &buffer->buffer));

			// Create the memory backing up the buffer handle
			VkMemoryRequirements memReqs;
			VkMemoryAllocateInfo memAlloc = vkTools::initializers::memoryAllocateInfo();
			vkGetBufferMemoryRequirements(logicalDevice, buffer->buffer, &memReqs);
			memAlloc.allocationSize = memReqs.size;
			// Find a memory type index that fits the properties of the buffer
			memAlloc.memoryTypeIndex = getMemoryType(memReqs.memoryTypeBits, memoryPropertyFlags);
			VK_CHECK_RESULT(vkAllocateMemory(logicalDevice, &memAlloc, nullptr, &buffer->memory));

			buffer->alignment = memReqs.alignment;
			buffer->size = memAlloc.allocationSize;
			buffer->usageFlags = usageFlags;
			buffer->memoryPropertyFlags = memoryPropertyFlags;

			// If a pointer to the buffer data has been passed, map the buffer and copy over the data
			if (data != nullptr)
			{
				VK_CHECK_RESULT(buffer->map());
				memcpy(buffer->mapped, data, size);
				buffer->unmap();
			}

			// Initialize a default descriptor that covers the whole buffer size
			buffer->setupDescriptor();

			// Attach the memory to the buffer object
			return buffer->bind();
		}

		/** 
		* Create a command pool for allocation command buffers from
		* 
		* @param queueFamilyIndex Family index of the queue to create the command pool for
		* @param createFlags (Optional) Command pool creation flags (Defaults to VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)
		*
		* @note Command buffers allocated from the created pool can only be submitted to a queue with the same family index
		*
		* @return A handle to the created command buffer
		*/
		VkCommandPool createCommandPool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags createFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)
		{
			VkCommandPoolCreateInfo cmdPoolInfo = {};
			cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			cmdPoolInfo.queueFamilyIndex = queueFamilyIndex;
			cmdPoolInfo.flags = createFlags;
			VkCommandPool cmdPool;
			VK_CHECK_RESULT(vkCreateCommandPool(logicalDevice, &cmdPoolInfo, nullptr, &cmdPool));
			return cmdPool;
		}

	};
}
