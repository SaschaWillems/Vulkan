/*
* Texture loader for Vulkan
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#include <vulkan/vulkan.h>
#include <gli/gli.hpp>

#if defined(__ANDROID__)
#include <android/asset_manager.h>
#endif

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
		uint32_t layerCount;
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
#if defined(__ANDROID__)
		AAssetManager* assetManager = nullptr;
#endif
		// Load a 2D texture
		void loadTexture(std::string filename, VkFormat format, VulkanTexture *texture)
		{
			loadTexture(filename, format, texture, false);
		}

		// Load a 2D texture
		void loadTexture(std::string filename, VkFormat format, VulkanTexture *texture, bool forceLinear)
		{
			loadTexture(filename, format, texture, forceLinear, VK_IMAGE_USAGE_SAMPLED_BIT);
		}

		// Load a 2D texture
		void loadTexture(std::string filename, VkFormat format, VulkanTexture *texture, bool forceLinear, VkImageUsageFlags imageUsageFlags)
		{
#if defined(__ANDROID__)
			assert(assetManager != nullptr);

			// Textures are stored inside the apk on Android (compressed)
			// So they need to be loaded via the asset manager
			AAsset* asset = AAssetManager_open(assetManager, filename.c_str(), AASSET_MODE_STREAMING);
			assert(asset);
			size_t size = AAsset_getLength(asset);
			assert(size > 0);

			void *textureData = malloc(size);
			AAsset_read(asset, textureData, size);
			AAsset_close(asset);

			gli::texture2D tex2D(gli::load((const char*)textureData, size));

			free(textureData);
#else
			gli::texture2D tex2D(gli::load(filename.c_str()));
#endif		
			assert(!tex2D.empty());

			texture->width = (uint32_t)tex2D[0].dimensions().x;
			texture->height = (uint32_t)tex2D[0].dimensions().y;
			texture->mipLevels = tex2D.levels();

			// Get device properites for the requested texture format
			VkFormatProperties formatProperties;
			vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);

			// Only use linear tiling if requested (and supported by the device)
			// Support for linear tiling is mostly limited, so prefer to use
			// optimal tiling instead
			// On most implementations linear tiling will only support a very
			// limited amount of formats and features (mip maps, cubemaps, arrays, etc.)
			VkBool32 useStaging = !forceLinear;

			VkImageCreateInfo imageCreateInfo = vkTools::initializers::imageCreateInfo();
			imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
			imageCreateInfo.format = format;
			imageCreateInfo.extent = { texture->width, texture->height, 1 };
			imageCreateInfo.mipLevels = 1;
			imageCreateInfo.arrayLayers = 1;
			imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
			imageCreateInfo.usage = (useStaging) ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : VK_IMAGE_USAGE_SAMPLED_BIT;
			imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;

			VkMemoryAllocateInfo memAllocInfo = vkTools::initializers::memoryAllocateInfo();
			VkMemoryRequirements memReqs;

			// Use a separate command buffer for texture loading
			VkCommandBufferBeginInfo cmdBufInfo = vkTools::initializers::commandBufferBeginInfo();
			vkTools::checkResult(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));

			if (useStaging)
			{
				// Load all available mip levels into linear textures
				// and copy to optimal tiling target
				struct MipLevel {
					VkImage image;
					VkDeviceMemory memory;
				};
				std::vector<MipLevel> mipLevels;
				mipLevels.resize(texture->mipLevels);

				// Copy mip levels
				for (uint32_t level = 0; level < texture->mipLevels; ++level)
				{
					imageCreateInfo.extent.width = tex2D[level].dimensions().x;
					imageCreateInfo.extent.height = tex2D[level].dimensions().y;
					imageCreateInfo.extent.depth = 1;

					vkTools::checkResult(vkCreateImage(device, &imageCreateInfo, nullptr, &mipLevels[level].image));

					vkGetImageMemoryRequirements(device, mipLevels[level].image, &memReqs);
					memAllocInfo.allocationSize = memReqs.size;
					getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memAllocInfo.memoryTypeIndex);
					vkTools::checkResult(vkAllocateMemory(device, &memAllocInfo, nullptr, &mipLevels[level].memory));
					vkTools::checkResult(vkBindImageMemory(device, mipLevels[level].image, mipLevels[level].memory, 0));

					VkImageSubresource subRes = {};
					subRes.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

					VkSubresourceLayout subResLayout;
					void *data;

					vkGetImageSubresourceLayout(device, mipLevels[level].image, &subRes, &subResLayout);
					vkTools::checkResult(vkMapMemory(device, mipLevels[level].memory, 0, memReqs.size, 0, &data));
					memcpy(data, tex2D[level].data(), tex2D[level].size());
					vkUnmapMemory(device, mipLevels[level].memory);

					// Image barrier for linear image (base)
					// Linear image will be used as a source for the copy
					setImageLayout(
						cmdBuffer,
						mipLevels[level].image,
						VK_IMAGE_ASPECT_COLOR_BIT,
						VK_IMAGE_LAYOUT_PREINITIALIZED,
						VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
				}

				// Setup texture as blit target with optimal tiling
				imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
				imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | imageUsageFlags;
				imageCreateInfo.mipLevels = texture->mipLevels;
				imageCreateInfo.extent = { texture->width, texture->height, 1 };

				vkTools::checkResult(vkCreateImage(device, &imageCreateInfo, nullptr, &texture->image));

				vkGetImageMemoryRequirements(device, texture->image, &memReqs);

				memAllocInfo.allocationSize = memReqs.size;

				getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memAllocInfo.memoryTypeIndex);
				vkTools::checkResult(vkAllocateMemory(device, &memAllocInfo, nullptr, &texture->deviceMemory));
				vkTools::checkResult(vkBindImageMemory(device, texture->image, texture->deviceMemory, 0));

				VkImageSubresourceRange subresourceRange = {};
				subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				subresourceRange.baseMipLevel = 0;
				subresourceRange.levelCount = texture->mipLevels;
				subresourceRange.layerCount = 1;

				// Image barrier for optimal image (target)
				// Optimal image will be used as destination for the copy
				setImageLayout(
					cmdBuffer,
					texture->image,
					VK_IMAGE_ASPECT_COLOR_BIT,
					VK_IMAGE_LAYOUT_PREINITIALIZED,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					subresourceRange);

				// Copy mip levels one by one
				for (uint32_t level = 0; level < texture->mipLevels; ++level)
				{
					// Copy region for image blit
					VkImageCopy copyRegion = {};

					copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					copyRegion.srcSubresource.baseArrayLayer = 0;
					copyRegion.srcSubresource.mipLevel = 0;
					copyRegion.srcSubresource.layerCount = 1;
					copyRegion.srcOffset = { 0, 0, 0 };

					copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					copyRegion.dstSubresource.baseArrayLayer = 0;
					// Set mip level to copy the linear image to
					copyRegion.dstSubresource.mipLevel = level;
					copyRegion.dstSubresource.layerCount = 1;
					copyRegion.dstOffset = { 0, 0, 0 };

					copyRegion.extent.width = tex2D[level].dimensions().x;
					copyRegion.extent.height = tex2D[level].dimensions().y;
					copyRegion.extent.depth = 1;

					// Put image copy into command buffer
					vkCmdCopyImage(
						cmdBuffer,
						mipLevels[level].image, 
						VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
						texture->image, 
						VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						1, 
						&copyRegion);
				}

				// Change texture image layout to shader read for all mip levels after the copy
				texture->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				setImageLayout(
					cmdBuffer,
					texture->image,
					VK_IMAGE_ASPECT_COLOR_BIT,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					texture->imageLayout,
					subresourceRange);

				// Submit command buffer containing copy and image layout commands
				vkTools::checkResult(vkEndCommandBuffer(cmdBuffer));

				// Create a fence to make sure that the copies have finished before continuing
				VkFence copyFence;
				VkFenceCreateInfo fenceCreateInfo = vkTools::initializers::fenceCreateInfo(VK_FLAGS_NONE);
				vkTools::checkResult(vkCreateFence(device, &fenceCreateInfo, nullptr, &copyFence));

				VkSubmitInfo submitInfo = vkTools::initializers::submitInfo();
				submitInfo.commandBufferCount = 1;
				submitInfo.pCommandBuffers = &cmdBuffer;

				vkTools::checkResult(vkQueueSubmit(queue, 1, &submitInfo, copyFence));

				vkTools::checkResult(vkWaitForFences(device, 1, &copyFence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));

				vkDestroyFence(device, copyFence, nullptr);

				// Destroy linear images used as staging buffers after copies have been finished
				//for (auto& level : mipLevels)
				for (uint32_t i = 0; i < mipLevels.size(); i++)
				{
					vkDestroyImage(device, mipLevels[i].image, nullptr);
					vkFreeMemory(device, mipLevels[i].memory, nullptr);
				}
			}
			else
			{
				// Prefer using optimal tiling, as linear tiling 
				// may support only a small set of features 
				// depending on implementation (e.g. no mip maps, only one layer, etc.)

				// Check if this support is supported for linear tiling
				assert(formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);

				VkImage mappableImage;
				VkDeviceMemory mappableMemory;

				// Load mip map level 0 to linear tiling image
				vkTools::checkResult(vkCreateImage(device, &imageCreateInfo, nullptr, &mappableImage));

				// Get memory requirements for this image 
				// like size and alignment
				vkGetImageMemoryRequirements(device, mappableImage, &memReqs);
				// Set memory allocation size to required memory size
				memAllocInfo.allocationSize = memReqs.size;

				// Get memory type that can be mapped to host memory
				getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memAllocInfo.memoryTypeIndex);

				// Allocate host memory
				vkTools::checkResult(vkAllocateMemory(device, &memAllocInfo, nullptr, &mappableMemory));

				// Bind allocated image for use
				vkTools::checkResult(vkBindImageMemory(device, mappableImage, mappableMemory, 0));

				// Get sub resource layout
				// Mip map count, array layer, etc.
				VkImageSubresource subRes = {};
				subRes.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				subRes.mipLevel = 0;

				VkSubresourceLayout subResLayout;
				void *data;

				// Get sub resources layout 
				// Includes row pitch, size offsets, etc.
				vkGetImageSubresourceLayout(device, mappableImage, &subRes, &subResLayout);

				// Map image memory
				vkTools::checkResult(vkMapMemory(device, mappableMemory, 0, memReqs.size, 0, &data));

				// Copy image data into memory
				memcpy(data, tex2D[subRes.mipLevel].data(), tex2D[subRes.mipLevel].size());

				vkUnmapMemory(device, mappableMemory);

				// Linear tiled images don't need to be staged
				// and can be directly used as textures
				texture->image = mappableImage;
				texture->deviceMemory = mappableMemory;
				texture->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

				// Setup image memory barrier
				setImageLayout(
					cmdBuffer,
					texture->image, 
					VK_IMAGE_ASPECT_COLOR_BIT, 
					VK_IMAGE_LAYOUT_PREINITIALIZED, 
					texture->imageLayout);

				// Submit command buffer containing copy and image layout commands
				vkTools::checkResult(vkEndCommandBuffer(cmdBuffer));

				VkFence nullFence = { VK_NULL_HANDLE };

				VkSubmitInfo submitInfo = vkTools::initializers::submitInfo();
				submitInfo.waitSemaphoreCount = 0;
				submitInfo.commandBufferCount = 1;
				submitInfo.pCommandBuffers = &cmdBuffer;

				vkTools::checkResult(vkQueueSubmit(queue, 1, &submitInfo, nullFence));
				vkTools::checkResult(vkQueueWaitIdle(queue));
			}

			// Create sampler
			VkSamplerCreateInfo sampler = {};
			sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			sampler.magFilter = VK_FILTER_LINEAR;
			sampler.minFilter = VK_FILTER_LINEAR;
			sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			sampler.mipLodBias = 0.0f;
			sampler.compareOp = VK_COMPARE_OP_NEVER;
			sampler.minLod = 0.0f;
			// Max level-of-detail should match mip level count
			sampler.maxLod = (useStaging) ? (float)texture->mipLevels : 0.0f;
			// Enable anisotropic filtering
			sampler.maxAnisotropy = 8;
			sampler.anisotropyEnable = VK_TRUE;
			sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
			vkTools::checkResult(vkCreateSampler(device, &sampler, nullptr, &texture->sampler));
			
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
			// Linear tiling usually won't support mip maps
			// Only set mip map count if optimal tiling is used
			view.subresourceRange.levelCount = (useStaging) ? texture->mipLevels : 1;
			view.image = texture->image;
			vkTools::checkResult(vkCreateImageView(device, &view, nullptr, &texture->view));
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

			vkTools::checkResult(vkAllocateCommandBuffers(device, &cmdBufInfo, &cmdBuffer));
		}

		~VulkanTextureLoader()
		{
			vkFreeCommandBuffers(device, cmdPool, 1, &cmdBuffer);
		}

		// Load a cubemap texture (single file)
		void loadCubemap(std::string filename, VkFormat format, VulkanTexture *texture)
		{
#if defined(__ANDROID__)
			assert(assetManager != nullptr);

			// Textures are stored inside the apk on Android (compressed)
			// So they need to be loaded via the asset manager
			AAsset* asset = AAssetManager_open(assetManager, filename.c_str(), AASSET_MODE_STREAMING);
			assert(asset);
			size_t size = AAsset_getLength(asset);
			assert(size > 0);

			void *textureData = malloc(size);
			AAsset_read(asset, textureData, size);
			AAsset_close(asset);

			gli::textureCube texCube(gli::load((const char*)textureData, size));

			free(textureData);
#else
			gli::textureCube texCube(gli::load(filename));
#endif	
			assert(!texCube.empty());

			texture->width = (uint32_t)texCube[0].dimensions().x;
			texture->height = (uint32_t)texCube[0].dimensions().y;

			// Get device properites for the requested texture format
			VkFormatProperties formatProperties;
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
			imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;

			VkMemoryAllocateInfo memAllocInfo = vkTools::initializers::memoryAllocateInfo();
			VkMemoryRequirements memReqs;

			struct {
				VkImage image;
				VkDeviceMemory memory;
			} cubeFace[6];

			VkCommandBufferBeginInfo cmdBufInfo = vkTools::initializers::commandBufferBeginInfo();

			vkTools::checkResult(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));

			// Load separate cube map faces into linear tiled textures
			for (uint32_t face = 0; face < 6; ++face)
			{
				vkTools::checkResult(vkCreateImage(device, &imageCreateInfo, nullptr, &cubeFace[face].image));

				vkGetImageMemoryRequirements(device, cubeFace[face].image, &memReqs);
				memAllocInfo.allocationSize = memReqs.size;
				getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memAllocInfo.memoryTypeIndex);
				vkTools::checkResult(vkAllocateMemory(device, &memAllocInfo, nullptr, &cubeFace[face].memory));
				vkTools::checkResult(vkBindImageMemory(device, cubeFace[face].image, cubeFace[face].memory, 0));

				VkImageSubresource subRes = {};
				subRes.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

				VkSubresourceLayout subResLayout;
				void *data;

				vkGetImageSubresourceLayout(device, cubeFace[face].image, &subRes, &subResLayout);
				vkTools::checkResult(vkMapMemory(device, cubeFace[face].memory, 0, memReqs.size, 0, &data));
				memcpy(data, texCube[face][subRes.mipLevel].data(), texCube[face][subRes.mipLevel].size());
				vkUnmapMemory(device, cubeFace[face].memory);

				// Image barrier for linear image (base)
				// Linear image will be used as a source for the copy
				setImageLayout(
					cmdBuffer,
					cubeFace[face].image,
					VK_IMAGE_ASPECT_COLOR_BIT,
					VK_IMAGE_LAYOUT_PREINITIALIZED,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
			}

			// Transfer cube map faces to optimal tiling

			// Setup texture as blit target with optimal tiling
			imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
			imageCreateInfo.arrayLayers = 6;

			vkTools::checkResult(vkCreateImage(device, &imageCreateInfo, nullptr, &texture->image));

			vkGetImageMemoryRequirements(device, texture->image, &memReqs);

			memAllocInfo.allocationSize = memReqs.size;

			getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memAllocInfo.memoryTypeIndex);
			vkTools::checkResult(vkAllocateMemory(device, &memAllocInfo, nullptr, &texture->deviceMemory));
			vkTools::checkResult(vkBindImageMemory(device, texture->image, texture->deviceMemory, 0));

			// Image barrier for optimal image (target)
			// Optimal image will be used as destination for the copy

			// Set initial layout for all array layers of the optimal (target) tiled texture
			VkImageSubresourceRange subresourceRange = {};
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = 1;
			subresourceRange.layerCount = 6;

			vkTools::setImageLayout(
				cmdBuffer,
				texture->image,
				VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_PREINITIALIZED,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				subresourceRange);

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

			}

			// Change texture image layout to shader read after the copy
			texture->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			setImageLayout(
				cmdBuffer,
				texture->image,
				VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				texture->imageLayout,
				subresourceRange);

			vkTools::checkResult(vkEndCommandBuffer(cmdBuffer));

			// Create a fence to make sure that the copies have finished before continuing
			VkFence copyFence;
			VkFenceCreateInfo fenceCreateInfo = vkTools::initializers::fenceCreateInfo(VK_FLAGS_NONE);
			vkTools::checkResult(vkCreateFence(device, &fenceCreateInfo, nullptr, &copyFence));

			VkSubmitInfo submitInfo = vkTools::initializers::submitInfo();
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &cmdBuffer;

			vkTools::checkResult(vkQueueSubmit(queue, 1, &submitInfo, copyFence));

			vkTools::checkResult(vkWaitForFences(device, 1, &copyFence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));

			vkDestroyFence(device, copyFence, nullptr);

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
			vkTools::checkResult(vkCreateSampler(device, &sampler, nullptr, &texture->sampler));

			// Create image view
			VkImageViewCreateInfo view = vkTools::initializers::imageViewCreateInfo();
			view.image = VK_NULL_HANDLE;
			view.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
			view.format = format;
			view.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
			view.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
			view.subresourceRange.layerCount = 6;
			view.image = texture->image;
			vkTools::checkResult(vkCreateImageView(device, &view, nullptr, &texture->view));

			// Cleanup
			for (auto& face : cubeFace)
			{
				vkDestroyImage(device, face.image, nullptr);
				vkFreeMemory(device, face.memory, nullptr);
			}
		}

		// Load an array texture (single file)
		void loadTextureArray(std::string filename, VkFormat format, VulkanTexture *texture)
		{
#if defined(__ANDROID__)
			assert(assetManager != nullptr);

			// Textures are stored inside the apk on Android (compressed)
			// So they need to be loaded via the asset manager
			AAsset* asset = AAssetManager_open(assetManager, filename.c_str(), AASSET_MODE_STREAMING);
			assert(asset);
			size_t size = AAsset_getLength(asset);
			assert(size > 0);

			void *textureData = malloc(size);
			AAsset_read(asset, textureData, size);
			AAsset_close(asset);

			gli::texture2DArray tex2DArray(gli::load((const char*)textureData, size));

			free(textureData);
#else
			gli::texture2DArray tex2DArray(gli::load(filename));
#endif	

			assert(!tex2DArray.empty());

			texture->width = tex2DArray.dimensions().x;
			texture->height = tex2DArray.dimensions().y;
			texture->layerCount = tex2DArray.layers();

			// Get device properites for the requested texture format
			VkFormatProperties formatProperties;
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
			imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;

			VkMemoryAllocateInfo memAllocInfo = vkTools::initializers::memoryAllocateInfo();
			VkMemoryRequirements memReqs;

			struct Layer {
				VkImage image;
				VkDeviceMemory memory;
			};
			std::vector<Layer> arrayLayer;
			arrayLayer.resize(texture->layerCount);

			// Allocate command buffer for image copies and layouts
			VkCommandBuffer cmdBuffer;
			VkCommandBufferAllocateInfo cmdBufAlllocatInfo =
				vkTools::initializers::commandBufferAllocateInfo(
					cmdPool,
					VK_COMMAND_BUFFER_LEVEL_PRIMARY,
					1);
			vkTools::checkResult(vkAllocateCommandBuffers(device, &cmdBufAlllocatInfo, &cmdBuffer));

			VkCommandBufferBeginInfo cmdBufInfo =
				vkTools::initializers::commandBufferBeginInfo();

			vkTools::checkResult(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));

			// Load separate cube map faces into linear tiled textures
			for (uint32_t i = 0; i < texture->layerCount; ++i)
			{
				vkTools::checkResult(vkCreateImage(device, &imageCreateInfo, nullptr, &arrayLayer[i].image));

				vkGetImageMemoryRequirements(device, arrayLayer[i].image, &memReqs);
				memAllocInfo.allocationSize = memReqs.size;
				getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memAllocInfo.memoryTypeIndex);
				vkTools::checkResult(vkAllocateMemory(device, &memAllocInfo, nullptr, &arrayLayer[i].memory));
				vkTools::checkResult(vkBindImageMemory(device, arrayLayer[i].image, arrayLayer[i].memory, 0));

				VkImageSubresource subRes = {};
				subRes.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

				VkSubresourceLayout subResLayout;
				void *data;

				vkGetImageSubresourceLayout(device, arrayLayer[i].image, &subRes, &subResLayout);
				vkTools::checkResult(vkMapMemory(device, arrayLayer[i].memory, 0, memReqs.size, 0, &data));
				memcpy(data, tex2DArray[i].data(), tex2DArray[i].size());
				vkUnmapMemory(device, arrayLayer[i].memory);

				// Image barrier for linear image (base)
				// Linear image will be used as a source for the copy
				vkTools::setImageLayout(
					cmdBuffer,
					arrayLayer[i].image,
					VK_IMAGE_ASPECT_COLOR_BIT,
					VK_IMAGE_LAYOUT_PREINITIALIZED,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
			}

			// Transfer cube map faces to optimal tiling

			// Setup texture as blit target with optimal tiling
			imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			imageCreateInfo.arrayLayers = texture->layerCount;

			vkTools::checkResult(vkCreateImage(device, &imageCreateInfo, nullptr, &texture->image));

			vkGetImageMemoryRequirements(device, texture->image, &memReqs);

			memAllocInfo.allocationSize = memReqs.size;

			getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memAllocInfo.memoryTypeIndex);
			vkTools::checkResult(vkAllocateMemory(device, &memAllocInfo, nullptr, &texture->deviceMemory));
			vkTools::checkResult(vkBindImageMemory(device, texture->image, texture->deviceMemory, 0));

			// Image barrier for optimal image (target)
			// Optimal image will be used as destination for the copy

			// Set initial layout for all array layers of the optimal (target) tiled texture
			VkImageSubresourceRange subresourceRange = {};
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = 1;
			subresourceRange.layerCount = texture->layerCount;

			vkTools::setImageLayout(
				cmdBuffer,
				texture->image,
				VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_PREINITIALIZED,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				subresourceRange);

			// Copy cube map faces one by one
			for (uint32_t i = 0; i < texture->layerCount; ++i)
			{
				// Copy region for image blit
				VkImageCopy copyRegion = {};

				copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				copyRegion.srcSubresource.baseArrayLayer = 0;
				copyRegion.srcSubresource.mipLevel = 0;
				copyRegion.srcSubresource.layerCount = 1;
				copyRegion.srcOffset = { 0, 0, 0 };

				copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				copyRegion.dstSubresource.baseArrayLayer = i;
				copyRegion.dstSubresource.mipLevel = 0;
				copyRegion.dstSubresource.layerCount = 1;
				copyRegion.dstOffset = { 0, 0, 0 };

				copyRegion.extent.width = texture->width;
				copyRegion.extent.height = texture->height;
				copyRegion.extent.depth = 1;

				// Put image copy into command buffer
				vkCmdCopyImage(
					cmdBuffer,
					arrayLayer[i].image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					texture->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1, &copyRegion);
			}

			// Change texture image layout to shader read after the copy
			texture->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			vkTools::setImageLayout(
				cmdBuffer,
				texture->image,
				VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				texture->imageLayout,
				subresourceRange);

			vkTools::checkResult(vkEndCommandBuffer(cmdBuffer));

			// Create a fence to make sure that the copies have finished before continuing
			VkFence copyFence;
			VkFenceCreateInfo fenceCreateInfo = vkTools::initializers::fenceCreateInfo(VK_FLAGS_NONE);
			vkTools::checkResult(vkCreateFence(device, &fenceCreateInfo, nullptr, &copyFence));

			VkSubmitInfo submitInfo = vkTools::initializers::submitInfo();
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &cmdBuffer;

			vkTools::checkResult(vkQueueSubmit(queue, 1, &submitInfo, copyFence));

			vkTools::checkResult(vkWaitForFences(device, 1, &copyFence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));

			vkDestroyFence(device, copyFence, nullptr);

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
			vkTools::checkResult(vkCreateSampler(device, &sampler, nullptr, &texture->sampler));

			// Create image view
			VkImageViewCreateInfo view = vkTools::initializers::imageViewCreateInfo();
			view.image = VK_NULL_HANDLE;
			view.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
			view.format = format;
			view.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
			view.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
			view.subresourceRange.layerCount = texture->layerCount;
			view.image = texture->image;
			vkTools::checkResult(vkCreateImageView(device, &view, nullptr, &texture->view));

			// Cleanup
			for (auto& layer : arrayLayer)
			{
				vkDestroyImage(device, layer.image, nullptr);
				vkFreeMemory(device, layer.memory, nullptr);
			}
		}



	};

};
