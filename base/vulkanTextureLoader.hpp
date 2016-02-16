/*
* Simple texture loader for Vulkan
*
* Note : No mip maps (yet), only uses optimal tiling (unless linear is forced)
*
* Copyright (C) 2015 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#include <vulkan/vulkan.h>
#include <gli/gli.hpp>

namespace vkTools 
{

	struct VulkanTexture
	{
		VkSampler sampler;
		VkImage image;
		VkImageLayout imageLayout;
		VkDeviceMemory deviceMemory;
		VkImageView view;
		uint32_t width, height;
		uint32_t mipLevels;
	};

	class VulkanTextureLoader
	{
	private:
		VkPhysicalDevice physicalDevice;
		VkDevice device;
		VkQueue queue;
		VkCommandBuffer cmdBuffer;
		VkCommandPool cmdPool;
		VkPhysicalDeviceMemoryProperties deviceMemoryProperties;

		// Try to find appropriate memory type for a memory allocation
		VkBool32 getMemoryType(uint32_t typeBits, VkFlags properties, uint32_t *typeIndex)
		{
			for (int i = 0; i < 32; i++) {
				if ((typeBits & 1) == 1) {
					if ((deviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
					{
						*typeIndex = i;
						return true;
					}
				}
				typeBits >>= 1;
			}
			return false;
		}
	public:
		// Load a 2D texture
		void loadTexture(const char* filename, VkFormat format, VulkanTexture *texture)
		{
			loadTexture(filename, format, texture, false);
		}

		// Load a 2D texture
		void loadTexture(const char* filename, VkFormat format, VulkanTexture *texture, bool forceLinear)
		{
			gli::texture2D tex2D(gli::load(filename));
			assert(!tex2D.empty());

			texture->width = (uint32_t)tex2D[0].dimensions().x;
			texture->height = (uint32_t)tex2D[0].dimensions().y;

			VkResult err;

			// Get device properites for the requested texture format
			VkFormatProperties formatProperties;
			vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);

			// Only use linear tiling if requested (and supported by the device)
			// Support for linear tiling is mostly limited, so prefer to use
			// optimal tiling instead
			// On most implementations linear tiling will only support a very
			// limited amount of formats and features (mip maps, cubemaps, arrays, etc.)
			VkBool32 useStaging = true;

			// Only use linear tiling if forced
			if (forceLinear)
			{
				useStaging = formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
			}

			VkImageCreateInfo imageCreateInfo = {};
			imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageCreateInfo.pNext = NULL;
			imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
			imageCreateInfo.format = format;
			imageCreateInfo.extent = { texture->width, texture->height, 1 };
			imageCreateInfo.mipLevels = 1;
			imageCreateInfo.arrayLayers = 1;
			imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
			imageCreateInfo.usage = (useStaging) ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : VK_IMAGE_USAGE_SAMPLED_BIT;
			imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageCreateInfo.flags = 0;

			VkMemoryAllocateInfo memAllocInfo = {};
			memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			memAllocInfo.pNext = NULL;
			memAllocInfo.allocationSize = 0;
			memAllocInfo.memoryTypeIndex = 0;

			VkImage mappableImage;
			VkDeviceMemory mappableMemory;

			// Create base image, if linear texturing is forced
			// this can directly be used
			err = vkCreateImage(device, &imageCreateInfo, nullptr, &mappableImage);
			assert(!err);

			// Get memory requirements for this image 
			// like size and alignment
			VkMemoryRequirements memReqs;
			vkGetImageMemoryRequirements(device, mappableImage, &memReqs);
			// Set memory allocation size to required memory size
			memAllocInfo.allocationSize = memReqs.size;

			// Get memory type that can be mapped to host memory
			getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memAllocInfo.memoryTypeIndex);

			// Allocate host memory
			err = vkAllocateMemory(device, &memAllocInfo, nullptr, &(mappableMemory));
			assert(!err);

			// Bind allocated image for use
			err = vkBindImageMemory(device, mappableImage, mappableMemory, 0);
			assert(!err);

			// Get sub resource layout
			// Mip map count, array layer, etc.
			VkImageSubresource subRes = {};
			subRes.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subRes.mipLevel = 0;
			subRes.arrayLayer = 0;

			VkSubresourceLayout subResLayout;
			void *data;

			// Get sub resources layout 
			// Includes row pitch, size offsets, etc.
			vkGetImageSubresourceLayout(device, mappableImage, &subRes, &subResLayout);

			// Map image memory
			err = vkMapMemory(device, mappableMemory, 0, memReqs.size, 0, &data);
			assert(!err);

			// Copy image data into memory
			memcpy(data, tex2D[subRes.mipLevel].data(), tex2D[subRes.mipLevel].size());

			vkUnmapMemory(device, mappableMemory);

			if (useStaging)
			{
				VkCommandBufferBeginInfo cmdBufInfo = {};
				cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				cmdBufInfo.pNext = NULL;

				err = vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo);
				assert(!err);

				// Setup texture as blit target with optimal tiling
				imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
				imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

				err = vkCreateImage(device, &imageCreateInfo, nullptr, &texture->image);
				assert(!err);

				vkGetImageMemoryRequirements(device, texture->image, &memReqs);

				memAllocInfo.allocationSize = memReqs.size;

				// Get device only memory type 
				getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memAllocInfo.memoryTypeIndex);

				// Allocate device memory
				err = vkAllocateMemory(device, &memAllocInfo, nullptr, &texture->deviceMemory);
				assert(!err);

				// Bind allocated image for use
				err = vkBindImageMemory(device, texture->image, texture->deviceMemory, 0);
				assert(!err);

				// Image barrier for linear image (base)
				// Linear image will be used as a source for the blit
				setImageLayout(cmdBuffer, 
					mappableImage,
					VK_IMAGE_ASPECT_COLOR_BIT,
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

				// Image barrier for optimal image (target)
				// Optimal image will be used as a target for the blit
				setImageLayout(cmdBuffer,
					texture->image,
					VK_IMAGE_ASPECT_COLOR_BIT,
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

				// Copy region for image blit
				VkImageCopy copyRegion = {};

				copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				copyRegion.srcSubresource.baseArrayLayer = 0;
				copyRegion.srcSubresource.mipLevel = 0;
				copyRegion.srcSubresource.layerCount = 1;
				copyRegion.srcOffset = { 0, 0, 0 };

				copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				copyRegion.dstSubresource.baseArrayLayer = 0;
				copyRegion.dstSubresource.mipLevel = 0;
				copyRegion.dstSubresource.layerCount = 1;
				copyRegion.dstOffset = { 0, 0, 0 };

				copyRegion.extent.width = texture->width;
				copyRegion.extent.height = texture->height;
				copyRegion.extent.depth = 1;

				// Put image copy into command buffer
				vkCmdCopyImage(cmdBuffer,
					mappableImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					texture->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1, &copyRegion);

				// Change texture image layout to shader read after the copy
				texture->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				setImageLayout(cmdBuffer,
					texture->image,
					VK_IMAGE_ASPECT_COLOR_BIT,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					texture->imageLayout);

				err = vkEndCommandBuffer(cmdBuffer);
				assert(!err);

				VkFence nullFence = { VK_NULL_HANDLE };

				VkSubmitInfo submitInfo = vkTools::initializers::submitInfo();
				submitInfo.waitSemaphoreCount = 0;
				submitInfo.commandBufferCount = 1;
				submitInfo.pCommandBuffers = &cmdBuffer;

				err = vkQueueSubmit(queue, 1, &submitInfo, nullFence);
				assert(!err);

				err = vkQueueWaitIdle(queue);
				assert(!err);
			}
			else
			{
				// Linear tiled images don't need to be staged
				// and can be directly used as textures

				texture->image = mappableImage;
				texture->deviceMemory = mappableMemory;
				texture->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

				// Setup image memory barrier
				setImageLayout(cmdBuffer, 
					texture->image, 
					VK_IMAGE_ASPECT_COLOR_BIT, 
					VK_IMAGE_LAYOUT_UNDEFINED, 
					texture->imageLayout);
			}

			// Create sampler
			// In Vulkan textures are accessed by samplers
			// This separates all the sampling information from the 
			// texture data
			// This means you could have multiple sampler objects
			// for the same texture with different settings
			// This is similar to the samplers available with OpenGL 3.3
			VkSamplerCreateInfo sampler = {};
			sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			sampler.magFilter = VK_FILTER_LINEAR;
			sampler.minFilter = VK_FILTER_LINEAR;
			sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			sampler.mipLodBias = 0.0f;
			sampler.maxAnisotropy = 0;
			sampler.compareOp = VK_COMPARE_OP_NEVER;
			sampler.minLod = 0.0f;
			sampler.maxLod = 0.0f;
			sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
			err = vkCreateSampler(device, &sampler, nullptr, &texture->sampler);
			assert(!err);

			// Create image view
			// Textures are not directly accessed by the shaders and
			// are abstracted by image views containing additional
			// information and sub resource ranges
			VkImageViewCreateInfo view = {};
			view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			view.pNext = NULL;
			view.image = VK_NULL_HANDLE;
			view.viewType = VK_IMAGE_VIEW_TYPE_2D;
			view.format = format;
			view.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
			view.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
			view.image = texture->image;
			err = vkCreateImageView(device, &view, nullptr, &texture->view);
			assert(!err);

			if (useStaging)
			{
				vkDestroyImage(device, mappableImage, nullptr);
				vkFreeMemory(device, mappableMemory, nullptr);
			}
		}

		// Clean up vulkan resources used by a texture object
		void destroyTexture(VulkanTexture texture)
		{
			vkDestroyImageView(device, texture.view, nullptr);
			vkDestroyImage(device, texture.image, nullptr);
			vkDestroySampler(device, texture.sampler, nullptr);
			vkFreeMemory(device, texture.deviceMemory, nullptr);
		}

		VulkanTextureLoader(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue queue, VkCommandPool cmdPool)
		{
			this->physicalDevice = physicalDevice;
			this->device = device;
			this->queue = queue;
			this->cmdPool = cmdPool;
			vkGetPhysicalDeviceMemoryProperties(physicalDevice, &deviceMemoryProperties);

			// Create command buffer for submitting image barriers
			// and converting tilings
			VkCommandBufferAllocateInfo cmdBufInfo = {};
			cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			cmdBufInfo.commandPool = cmdPool;
			cmdBufInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			cmdBufInfo.commandBufferCount = 1;

			VkResult vkRes = vkAllocateCommandBuffers(device, &cmdBufInfo, &cmdBuffer);
			assert(vkRes == VK_SUCCESS);
		}

		~VulkanTextureLoader()
		{
			vkFreeCommandBuffers(device, cmdPool, 1, &cmdBuffer);
		}

		// Load a cubemap texture (single file)
		void loadCubemap(const char* filename, VkFormat format, VulkanTexture *texture)
		{
			VkFormatProperties formatProperties;
			VkResult err;

			gli::textureCube texCube(gli::load(filename));
			assert(!texCube.empty());

			texture->width = (uint32_t)texCube[0].dimensions().x;
			texture->height = (uint32_t)texCube[0].dimensions().y;

			// Get device properites for the requested texture format
			vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);

			VkImageCreateInfo imageCreateInfo = vkTools::initializers::imageCreateInfo();
			imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
			imageCreateInfo.format = format;
			imageCreateInfo.extent = { texture->width, texture->height, 1 };
			imageCreateInfo.mipLevels = 1;
			imageCreateInfo.arrayLayers = 1;
			imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
			imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
			imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageCreateInfo.flags = 0;

			VkMemoryAllocateInfo memAllocInfo = vkTools::initializers::memoryAllocateInfo();
			VkMemoryRequirements memReqs;

			struct {
				VkImage image;
				VkDeviceMemory memory;
			} cubeFace[6];

			VkCommandBufferBeginInfo cmdBufInfo = {};
			cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			cmdBufInfo.pNext = NULL;

			err = vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo);
			assert(!err);

			// Load separate cube map faces into linear tiled textures
			for (uint32_t face = 0; face < 6; ++face)
			{
				err = vkCreateImage(device, &imageCreateInfo, nullptr, &cubeFace[face].image);
				assert(!err);

				vkGetImageMemoryRequirements(device, cubeFace[face].image, &memReqs);
				memAllocInfo.allocationSize = memReqs.size;
				getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memAllocInfo.memoryTypeIndex);
				err = vkAllocateMemory(device, &memAllocInfo, nullptr, &cubeFace[face].memory);
				assert(!err);
				err = vkBindImageMemory(device, cubeFace[face].image, cubeFace[face].memory, 0);
				assert(!err);

				VkImageSubresource subRes = {};
				subRes.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

				VkSubresourceLayout subResLayout;
				void *data;

				vkGetImageSubresourceLayout(device, cubeFace[face].image, &subRes, &subResLayout);
				assert(!err);
				err = vkMapMemory(device, cubeFace[face].memory, 0, memReqs.size, 0, &data);
				assert(!err);
				memcpy(data, texCube[face][subRes.mipLevel].data(), texCube[face][subRes.mipLevel].size());
				vkUnmapMemory(device, cubeFace[face].memory);

				// Image barrier for linear image (base)
				// Linear image will be used as a source for the copy
				setImageLayout(
					cmdBuffer,
					cubeFace[face].image,
					VK_IMAGE_ASPECT_COLOR_BIT,
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
			}

			// Transfer cube map faces to optimal tiling

			// Setup texture as blit target with optimal tiling
			imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
			imageCreateInfo.arrayLayers = 6;

			err = vkCreateImage(device, &imageCreateInfo, nullptr, &texture->image);
			assert(!err);

			vkGetImageMemoryRequirements(device, texture->image, &memReqs);

			memAllocInfo.allocationSize = memReqs.size;

			getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memAllocInfo.memoryTypeIndex);
			err = vkAllocateMemory(device, &memAllocInfo, nullptr, &texture->deviceMemory);
			assert(!err);
			err = vkBindImageMemory(device, texture->image, texture->deviceMemory, 0);
			assert(!err);

			// Image barrier for optimal image (target)
			// Optimal image will be used as destination for the copy
			setImageLayout(
				cmdBuffer,
				texture->image,
				VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

			// Copy cube map faces one by one
			for (uint32_t face = 0; face < 6; ++face)
			{
				// Copy region for image blit
				VkImageCopy copyRegion = {};

				copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				copyRegion.srcSubresource.baseArrayLayer = 0;
				copyRegion.srcSubresource.mipLevel = 0;
				copyRegion.srcSubresource.layerCount = 1;
				copyRegion.srcOffset = { 0, 0, 0 };

				copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				copyRegion.dstSubresource.baseArrayLayer = face;
				copyRegion.dstSubresource.mipLevel = 0;
				copyRegion.dstSubresource.layerCount = 1;
				copyRegion.dstOffset = { 0, 0, 0 };

				copyRegion.extent.width = texture->width;
				copyRegion.extent.height = texture->height;
				copyRegion.extent.depth = 1;

				// Put image copy into command buffer
				vkCmdCopyImage(
					cmdBuffer,
					cubeFace[face].image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					texture->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1, &copyRegion);

				// Change texture image layout to shader read after the copy
				texture->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				setImageLayout(
					cmdBuffer,
					texture->image,
					VK_IMAGE_ASPECT_COLOR_BIT,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					texture->imageLayout);
			}

			err = vkEndCommandBuffer(cmdBuffer);
			assert(!err);

			VkFence nullFence = { VK_NULL_HANDLE };

			VkSubmitInfo submitInfo = vkTools::initializers::submitInfo();
			submitInfo.waitSemaphoreCount = 0;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &cmdBuffer;

			err = vkQueueSubmit(queue, 1, &submitInfo, nullFence);
			assert(!err);

			err = vkQueueWaitIdle(queue);
			assert(!err);

			// Create sampler
			VkSamplerCreateInfo sampler = vkTools::initializers::samplerCreateInfo();
			sampler.magFilter = VK_FILTER_LINEAR;
			sampler.minFilter = VK_FILTER_LINEAR;
			sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			sampler.addressModeV = sampler.addressModeU;
			sampler.addressModeW = sampler.addressModeU;
			sampler.mipLodBias = 0.0f;
			sampler.maxAnisotropy = 8;
			sampler.compareOp = VK_COMPARE_OP_NEVER;
			sampler.minLod = 0.0f;
			sampler.maxLod = 0.0f;
			sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
			err = vkCreateSampler(device, &sampler, nullptr, &texture->sampler);
			assert(!err);

			// Create image view
			VkImageViewCreateInfo view = vkTools::initializers::imageViewCreateInfo();
			view.image = VK_NULL_HANDLE;
			view.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
			view.format = format;
			view.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
			view.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
			view.subresourceRange.layerCount = 6;
			view.image = texture->image;
			err = vkCreateImageView(device, &view, nullptr, &texture->view);
			assert(!err);

			// Cleanup
			for (auto& face : cubeFace)
			{
				vkDestroyImage(device, face.image, nullptr);
				vkFreeMemory(device, face.memory, nullptr);
			}
		}


	};

};
