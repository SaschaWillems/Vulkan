/*
* Vulkan Example - Sparse texture residency example
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

/*
todos: 
- check sparse binding support on queue
- residencyNonResidentStrict
- meta data
- Run-time image data upload
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <vector>
#include <algorithm>
#include <random>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vulkan/vulkan.h>
#include "vulkanexamplebase.h"
#include "vulkandevice.hpp"
#include "vulkanbuffer.hpp"

#define VERTEX_BUFFER_BIND_ID 0
#define ENABLE_VALIDATION false

// Vertex layout for this example
struct Vertex {
	float pos[3];
	float uv[2];
	float normal[3];
};

// Virtual texture page as a part of the partially resident texture
// Contains memory bindings, offsets and status information
struct VirtualTexturePage
{	
	VkOffset3D offset;
	VkExtent3D extent;
	VkSparseImageMemoryBind imageMemoryBind;							// Sparse image memory bind for this page
	VkDeviceSize size;													// Page (memory) size in bytes
	uint32_t mipLevel;													// Mip level that this page belongs to
	uint32_t layer;														// Array layer that this page belongs to
	uint32_t index;	

	VirtualTexturePage()
	{
		imageMemoryBind.memory = VK_NULL_HANDLE;						// Page initially not backed up by memory
	}

	// Allocate Vulkan memory for the virtual page
	void allocate(VkDevice device, uint32_t memoryTypeIndex)
	{
		if (imageMemoryBind.memory != VK_NULL_HANDLE)
		{
			//std::cout << "Page " << index << " already allocated" << std::endl;
			return;
		};

		imageMemoryBind = {};

		VkMemoryAllocateInfo allocInfo = vkTools::initializers::memoryAllocateInfo();
		allocInfo.allocationSize = size;
		allocInfo.memoryTypeIndex = memoryTypeIndex;
		VK_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, &imageMemoryBind.memory));

		VkImageSubresource subResource{};
		subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subResource.mipLevel = mipLevel;
		subResource.arrayLayer = layer;

		// Sparse image memory binding
		imageMemoryBind.subresource = subResource;
		imageMemoryBind.extent = extent;
		imageMemoryBind.offset = offset;
	}

	// Release Vulkan memory allocated for this page
	void release(VkDevice device)
	{
		if (imageMemoryBind.memory != VK_NULL_HANDLE)
		{
			vkFreeMemory(device, imageMemoryBind.memory, nullptr);
			imageMemoryBind.memory = VK_NULL_HANDLE;
			//std::cout << "Page " << index << " released" << std::endl;
		}
	}
};

// Virtual texture object containing all pages 
struct VirtualTexture
{
	VkDevice device;
	VkImage image;														// Texture image handle
	VkBindSparseInfo bindSparseInfo;									// Sparse queue binding information
	std::vector<VirtualTexturePage> pages;								// Contains all virtual pages of the texture
	std::vector<VkSparseImageMemoryBind> sparseImageMemoryBinds;		// Sparse image memory bindings of all memory-backed virtual tables
	std::vector<VkSparseMemoryBind>	opaqueMemoryBinds;					// Sparse ópaque memory bindings for the mip tail (if present)
	VkSparseImageMemoryBindInfo imageMemoryBindInfo;					// Sparse image memory bind info 
	VkSparseImageOpaqueMemoryBindInfo opaqueMemoryBindInfo;				// Sparse image opaque memory bind info (mip tail)
	
	VirtualTexturePage* addPage(VkOffset3D offset, VkExtent3D extent, const VkDeviceSize size, const uint32_t mipLevel, uint32_t layer)
	{
		VirtualTexturePage newPage;
		newPage.offset = offset;
		newPage.extent = extent;
		newPage.size = size;
		newPage.mipLevel = mipLevel;
		newPage.layer = layer;
		newPage.index = static_cast<uint32_t>(pages.size());
		newPage.imageMemoryBind.offset = offset;
		newPage.imageMemoryBind.extent = extent;
		pages.push_back(newPage);
		return &pages.back();
	}

	// Call before sparse binding to update memory bind list etc.
	void updateSparseBindInfo()
	{
		// Update list of memory-backed sparse image memory binds
		sparseImageMemoryBinds.clear();
		for (auto page : pages)
		{
			sparseImageMemoryBinds.push_back(page.imageMemoryBind);
		}
		// Update sparse bind info
		bindSparseInfo = vkTools::initializers::bindSparseInfo();
		// todo: Semaphore for queue submission
		// bindSparseInfo.signalSemaphoreCount = 1;
		// bindSparseInfo.pSignalSemaphores = &bindSparseSemaphore;

		// Image memory binds
		imageMemoryBindInfo.image = image;
		imageMemoryBindInfo.bindCount = static_cast<uint32_t>(sparseImageMemoryBinds.size());
		imageMemoryBindInfo.pBinds = sparseImageMemoryBinds.data();
		bindSparseInfo.imageBindCount = (imageMemoryBindInfo.bindCount > 0) ? 1 : 0;
		bindSparseInfo.pImageBinds = &imageMemoryBindInfo;

		// Opaque image memory binds (mip tail)
		opaqueMemoryBindInfo.image = image;
		opaqueMemoryBindInfo.bindCount = static_cast<uint32_t>(opaqueMemoryBinds.size());
		opaqueMemoryBindInfo.pBinds = opaqueMemoryBinds.data();
		bindSparseInfo.imageOpaqueBindCount = (opaqueMemoryBindInfo.bindCount > 0) ? 1 : 0;
		bindSparseInfo.pImageOpaqueBinds = &opaqueMemoryBindInfo;

		uint32_t memBackedPages(0);
		for (auto page : pages)
		{
			if (page.imageMemoryBind.memory != VK_NULL_HANDLE)
			{
				memBackedPages++;
			}
		}
		std::cout << "Bound " << memBackedPages << " memory backed virtual pages " << std::endl;
	}

	// Release all Vulkan resources
	void destroy()
	{
		for (auto page : pages)
		{
			page.release(device);
		}
		for (auto bind : opaqueMemoryBinds)
		{
			vkFreeMemory(device, bind.memory, nullptr);
		}
	}
};

uint32_t memoryTypeIndex;

class VulkanExample : public VulkanExampleBase
{
public:
	//todo: comments
	struct SparseTexture : VirtualTexture {
		VkSampler sampler;
		VkImageLayout imageLayout;
		VkImageView view;
		VkDescriptorImageInfo descriptor;
		VkFormat format;
		uint32_t width, height;
		uint32_t mipLevels;
		uint32_t layerCount;
	} texture;

	struct {
		VkPipelineVertexInputStateCreateInfo inputState;
		std::vector<VkVertexInputBindingDescription> bindingDescriptions;
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
	} vertices;

	vk::Buffer vertexBuffer;
	vk::Buffer indexBuffer;
	uint32_t indexCount;

	vk::Buffer uniformBufferVS;

	struct UboVS {
		glm::mat4 projection;
		glm::mat4 model;
		glm::vec4 viewPos;
		float lodBias = 0.0f;
	} uboVS;

	struct {
		VkPipeline solid;
	} pipelines;

	VkPipelineLayout pipelineLayout;
	VkDescriptorSet descriptorSet;
	VkDescriptorSetLayout descriptorSetLayout;

	//todo: comment
	VkSemaphore bindSparseSemaphore = VK_NULL_HANDLE;

	// Device features to be enabled for this example 
	static VkPhysicalDeviceFeatures getEnabledFeatures()
	{
		VkPhysicalDeviceFeatures enabledFeatures = {};
		enabledFeatures.shaderResourceResidency = VK_TRUE;
		return enabledFeatures;
	}

	VulkanExample() : VulkanExampleBase(ENABLE_VALIDATION, getEnabledFeatures)
	{
		zoom = -2.5f;
		rotation = { 0.0f, 15.0f, 0.0f };
		title = "Vulkan Example - Sparse textures residency";
		enableTextOverlay = true;
		std::cout.imbue(std::locale(""));
		//todo: check if GPU supports sparse binding feature
	}

	~VulkanExample()
	{
		// Clean up used Vulkan resources 
		// Note : Inherited destructor cleans up resources stored in base class

		destroyTextureImage(texture);

		vkDestroySemaphore(device, bindSparseSemaphore, nullptr);

		vkDestroyPipeline(device, pipelines.solid, nullptr);

		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

		vertexBuffer.destroy();
		indexBuffer.destroy();
		uniformBufferVS.destroy();
	}

	glm::uvec3 alignedDivision(const VkExtent3D& extent, const VkExtent3D& granularity)
	{
		glm::uvec3 res;
		res.x = extent.width / granularity.width + ((extent.width  % granularity.width) ? 1u : 0u);
		res.y = extent.height / granularity.height + ((extent.height % granularity.height) ? 1u : 0u);
		res.z = extent.depth / granularity.depth + ((extent.depth  % granularity.depth) ? 1u : 0u);
		return res;
	}

	void prepareSparseTexture(uint32_t width, uint32_t height, uint32_t layerCount, VkFormat format)
	{
		texture.device = vulkanDevice->logicalDevice;
		texture.width = width;
		texture.height = height;
		texture.mipLevels = floor(log2(std::max(width, height))) + 1; //todo
		texture.layerCount = layerCount;
		texture.format = format;

		// Get device properites for the requested texture format
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);

		// Get sparse image properties
		std::vector<VkSparseImageFormatProperties> sparseProperties(32);
		// Sparse properties count for the desired format
		uint32_t sparsePropertiesCount;
		// todo: Temporary workaround, crashes in NV driver if last param is nullptr (to get just count)
		vkGetPhysicalDeviceSparseImageFormatProperties(
			physicalDevice,
			format,
			VK_IMAGE_TYPE_2D,
			VK_SAMPLE_COUNT_1_BIT,
			VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_IMAGE_TILING_OPTIMAL,
			&sparsePropertiesCount,
			sparseProperties.data());
		sparseProperties.resize(sparsePropertiesCount);

		// Check if sparse is supported for this format
		if (sparsePropertiesCount == 0)
		{
			std::cout << "Error: Requested format does not support sparse features!" << std::endl;
			return;
		}

		std::cout << "Sparse image format properties: " << sparsePropertiesCount << std::endl;
		for (auto props : sparseProperties)
		{
			std::cout << "\t Image granularity: w = " << props.imageGranularity.width << " h = " << props.imageGranularity.height << " d = " << props.imageGranularity.depth << std::endl;
			std::cout << "\t Aspect mask: " << props.aspectMask << std::endl;
			std::cout << "\t Flags: " << props.flags << std::endl;
		}

		// Create sparse image
		VkImageCreateInfo sparseImageCreateInfo = vkTools::initializers::imageCreateInfo();
		sparseImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		sparseImageCreateInfo.format = texture.format;
		sparseImageCreateInfo.mipLevels = texture.mipLevels;
		sparseImageCreateInfo.arrayLayers = texture.layerCount;
		sparseImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		sparseImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		sparseImageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
		sparseImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		sparseImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		sparseImageCreateInfo.extent = { texture.width, texture.height, 1 };
		sparseImageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		sparseImageCreateInfo.flags = VK_IMAGE_CREATE_SPARSE_BINDING_BIT | VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT;
		VK_CHECK_RESULT(vkCreateImage(device, &sparseImageCreateInfo, nullptr, &texture.image));

		// Get memory requirements
		VkMemoryRequirements sparseImageMemoryReqs;
		vkGetImageMemoryRequirements(device, texture.image, &sparseImageMemoryReqs);

		std::cout << "Image memory requirements:" << std::endl;
		std::cout << "\t Size: " << sparseImageMemoryReqs.size << std::endl;
		std::cout << "\t Alignment: " << sparseImageMemoryReqs.alignment << std::endl;

		// Check requested image size against hardware sparse limit
		if (sparseImageMemoryReqs.size > vulkanDevice->properties.limits.sparseAddressSpaceSize)
		{
			std::cout << "Error: Requested sparse image size exceeds supportes sparse address space size!" << std::endl;
			return;
		};

		// Get sparse memory requirements
		uint32_t sparseMemoryReqsCount;
		std::vector<VkSparseImageMemoryRequirements> sparseMemoryReqs(32);
		// todo: Temporary workaround, crashes in NV driver if last param is nullptr (to get just count)
		vkGetImageSparseMemoryRequirements(device, texture.image, &sparseMemoryReqsCount, sparseMemoryReqs.data());
		if (sparseMemoryReqsCount == 0)
		{
			std::cout << "Error: No memory requirements for the sparse image!" << std::endl;
			return;
		}
		sparseMemoryReqs.resize(sparseMemoryReqsCount);

		std::cout << "Sparse image memory requirements: " << sparseMemoryReqsCount << std::endl;
		for (auto reqs : sparseMemoryReqs)
		{
			std::cout << "\t Image granularity: w = " << reqs.formatProperties.imageGranularity.width << " h = " << reqs.formatProperties.imageGranularity.height << " d = " << reqs.formatProperties.imageGranularity.depth << std::endl;
			std::cout << "\t Mip tail first LOD: " << reqs.imageMipTailFirstLod << std::endl;
			std::cout << "\t Mip tail size: " << reqs.imageMipTailSize << std::endl;
			std::cout << "\t Mip tail offset: " << reqs.imageMipTailOffset << std::endl;
			std::cout << "\t Mip tail stride: " << reqs.imageMipTailStride << std::endl;
		}

		// Get sparse image requirements for the color aspect
		VkSparseImageMemoryRequirements sparseMemoryReq;
		bool colorAspectFound = false;
		for (auto reqs : sparseMemoryReqs)
		{
			if (reqs.formatProperties.aspectMask & VK_IMAGE_ASPECT_COLOR_BIT)
			{
				sparseMemoryReq = reqs;
				colorAspectFound = true;
				break;
			}
		}
		if (!colorAspectFound)
		{
			std::cout << "Error: Could not find sparse image memory requirements for color aspect bit!" << std::endl;
			return;
		}

		// todo:
		// Calculate number of required sparse memory bindings by alignment
		assert((sparseImageMemoryReqs.size % sparseImageMemoryReqs.alignment) == 0);
		memoryTypeIndex = vulkanDevice->getMemoryType(sparseImageMemoryReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		// Get sparse bindings
		uint32_t sparseBindsCount = static_cast<uint32_t>(sparseImageMemoryReqs.size / sparseImageMemoryReqs.alignment);		
		std::vector<VkSparseMemoryBind>	sparseMemoryBinds(sparseBindsCount);

		// Check if the format has a single mip tail for all layers or one mip tail for each layer
		// The mip tail contains all mip levels > sparseMemoryReq.imageMipTailFirstLod
		bool singleMipTail = sparseMemoryReq.formatProperties.flags & VK_SPARSE_IMAGE_FORMAT_SINGLE_MIPTAIL_BIT;

		// Sparse bindings for each mip level of all layers outside of the mip tail
		for (uint32_t layer = 0; layer < texture.layerCount; layer++)
		{
			// sparseMemoryReq.imageMipTailFirstLod is the first mip level that's stored inside the mip tail
			for (uint32_t mipLevel = 0; mipLevel < sparseMemoryReq.imageMipTailFirstLod; mipLevel++)
			{
				VkExtent3D extent;
				extent.width = std::max(sparseImageCreateInfo.extent.width >> mipLevel, 1u);
				extent.height = std::max(sparseImageCreateInfo.extent.height >> mipLevel, 1u);
				extent.depth = std::max(sparseImageCreateInfo.extent.depth >> mipLevel, 1u);

				VkImageSubresource subResource{};
				subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				subResource.mipLevel = mipLevel;
				subResource.arrayLayer = layer;

				// Aligned sizes by image granularity
				VkExtent3D imageGranularity = sparseMemoryReq.formatProperties.imageGranularity;
				glm::uvec3 sparseBindCounts = alignedDivision(extent, imageGranularity);
				glm::uvec3 lastBlockExtent;
				lastBlockExtent.x = (extent.width % imageGranularity.width) ? extent.width % imageGranularity.width : imageGranularity.width;
				lastBlockExtent.y = (extent.height % imageGranularity.height) ? extent.height % imageGranularity.height : imageGranularity.height;
				lastBlockExtent.z = (extent.depth % imageGranularity.depth) ? extent.depth % imageGranularity.depth : imageGranularity.depth;

				// Alllocate memory for some blocks
				uint32_t index = 0;
				for (uint32_t z = 0; z < sparseBindCounts.z; z++)
				{
					for (uint32_t y = 0; y < sparseBindCounts.y; y++)
					{
						for (uint32_t x = 0; x < sparseBindCounts.x; x++)
						{
							// Offset 
							VkOffset3D offset;
							offset.x = x * imageGranularity.width;
							offset.y = y * imageGranularity.height;
							offset.z = z * imageGranularity.depth;
							// Size of the page
							VkExtent3D extent;
							extent.width = (x == sparseBindCounts.x - 1) ? lastBlockExtent.x : imageGranularity.width;
							extent.height = (y == sparseBindCounts.y - 1) ? lastBlockExtent.y : imageGranularity.height;
							extent.depth = (z == sparseBindCounts.z - 1) ? lastBlockExtent.z : imageGranularity.depth;

							// Add new virtual page
							VirtualTexturePage *newPage = texture.addPage(offset, extent, sparseImageMemoryReqs.alignment, mipLevel, layer);
							newPage->imageMemoryBind.subresource = subResource;

							if ((x % 2 == 1) || (y % 2 == 1))
							{
								// Allocate memory for this virtual page
								newPage->allocate(device, memoryTypeIndex);
							}

							index++;
						}
					}
				}
			}

			// Check if format has one mip tail per layer
			if ((!singleMipTail) && (sparseMemoryReq.imageMipTailFirstLod < texture.mipLevels))
			{
				// Allocate memory for the mip tail
				VkMemoryAllocateInfo allocInfo = vkTools::initializers::memoryAllocateInfo();
				allocInfo.allocationSize = sparseMemoryReq.imageMipTailSize;
				allocInfo.memoryTypeIndex = memoryTypeIndex;

				VkDeviceMemory deviceMemory;
				VK_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, &deviceMemory));

				// (Opaque) sparse memory binding
				VkSparseMemoryBind sparseMemoryBind{};
				sparseMemoryBind.resourceOffset = sparseMemoryReq.imageMipTailOffset + layer * sparseMemoryReq.imageMipTailStride;
				sparseMemoryBind.size = sparseMemoryReq.imageMipTailSize;
				sparseMemoryBind.memory = deviceMemory;

				texture.opaqueMemoryBinds.push_back(sparseMemoryBind);
			}
		} // end layers and mips

		std::cout << "Texture info:" << std::endl;
		std::cout << "\tDim: " << texture.width << " x " << texture.height << std::endl;
		std::cout << "\tVirtual pages: " << texture.pages.size() << std::endl;

		// Check if format has one mip tail for all layers
		if ((sparseMemoryReq.formatProperties.flags & VK_SPARSE_IMAGE_FORMAT_SINGLE_MIPTAIL_BIT) && (sparseMemoryReq.imageMipTailFirstLod < texture.mipLevels))
		{
			// Allocate memory for the mip tail
			VkMemoryAllocateInfo allocInfo = vkTools::initializers::memoryAllocateInfo();
			allocInfo.allocationSize = sparseMemoryReq.imageMipTailSize;
			allocInfo.memoryTypeIndex = memoryTypeIndex;

			VkDeviceMemory deviceMemory;
			VK_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, &deviceMemory));

			// (Opaque) sparse memory binding
			VkSparseMemoryBind sparseMemoryBind{};
			sparseMemoryBind.resourceOffset = sparseMemoryReq.imageMipTailOffset;
			sparseMemoryBind.size = sparseMemoryReq.imageMipTailSize;
			sparseMemoryBind.memory = deviceMemory;

			texture.opaqueMemoryBinds.push_back(sparseMemoryBind);
		}

		// Create signal semaphore for sparse binding
		VkSemaphoreCreateInfo semaphoreCreateInfo = vkTools::initializers::semaphoreCreateInfo();
		VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &bindSparseSemaphore));

		// Prepare bind sparse info for reuse in queue submission
		texture.updateSparseBindInfo();

		// Bind to queue
		// todo: in draw?
		vkQueueBindSparse(queue, 1, &texture.bindSparseInfo, VK_NULL_HANDLE);
		//todo: use sparse bind semaphore
		vkQueueWaitIdle(queue);

		// Create sampler
		VkSamplerCreateInfo sampler = vkTools::initializers::samplerCreateInfo();
		sampler.magFilter = VK_FILTER_LINEAR;
		sampler.minFilter = VK_FILTER_LINEAR;
		sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler.mipLodBias = 0.0f;
		sampler.compareOp = VK_COMPARE_OP_NEVER;
		sampler.minLod = 0.0f;
		sampler.maxLod = static_cast<float>(texture.mipLevels);
		sampler.anisotropyEnable = vulkanDevice->features.samplerAnisotropy;
		sampler.maxAnisotropy = vulkanDevice->features.samplerAnisotropy ? vulkanDevice->properties.limits.maxSamplerAnisotropy : 1.0f;
		sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		VK_CHECK_RESULT(vkCreateSampler(device, &sampler, nullptr, &texture.sampler));

		// Create image view
		VkImageViewCreateInfo view = vkTools::initializers::imageViewCreateInfo();
		view.image = VK_NULL_HANDLE;
		view.viewType = VK_IMAGE_VIEW_TYPE_2D;
		view.format = format;
		view.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		view.subresourceRange.baseMipLevel = 0;
		view.subresourceRange.baseArrayLayer = 0;
		view.subresourceRange.layerCount = 1;
		view.subresourceRange.levelCount = texture.mipLevels;
		view.image = texture.image;
		VK_CHECK_RESULT(vkCreateImageView(device, &view, nullptr, &texture.view));

		// Fill image descriptor image info that can be used during the descriptor set setup
		texture.descriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		texture.descriptor.imageView = texture.view;
		texture.descriptor.sampler = texture.sampler;
	}

	// Free all Vulkan resources used a texture object
	void destroyTextureImage(SparseTexture texture)
	{
		vkDestroyImageView(device, texture.view, nullptr);
		vkDestroyImage(device, texture.image, nullptr);
		vkDestroySampler(device, texture.sampler, nullptr);
		texture.destroy();
	}

	void buildCommandBuffers()
	{
		VkCommandBufferBeginInfo cmdBufInfo = vkTools::initializers::commandBufferBeginInfo();

		VkClearValue clearValues[2];
		clearValues[0].color = defaultClearColor;
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderPassBeginInfo = vkTools::initializers::renderPassBeginInfo();
		renderPassBeginInfo.renderPass = renderPass;
		renderPassBeginInfo.renderArea.offset.x = 0;
		renderPassBeginInfo.renderArea.offset.y = 0;
		renderPassBeginInfo.renderArea.extent.width = width;
		renderPassBeginInfo.renderArea.extent.height = height;
		renderPassBeginInfo.clearValueCount = 2;
		renderPassBeginInfo.pClearValues = clearValues;

		for (int32_t i = 0; i < drawCmdBuffers.size(); ++i)
		{
			// Set target frame buffer
			renderPassBeginInfo.framebuffer = frameBuffers[i];

			VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo));

			vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport viewport = vkTools::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
			vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

			VkRect2D scissor = vkTools::initializers::rect2D(width, height, 0, 0);
			vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

			vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);
			vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.solid);

			VkDeviceSize offsets[1] = { 0 };
			vkCmdBindVertexBuffers(drawCmdBuffers[i], VERTEX_BUFFER_BIND_ID, 1, &vertexBuffer.buffer, offsets);
			vkCmdBindIndexBuffer(drawCmdBuffers[i], indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

			vkCmdDrawIndexed(drawCmdBuffers[i], indexCount, 1, 0, 0, 0);

			vkCmdEndRenderPass(drawCmdBuffers[i]);

			VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
		}
	}

	void draw()
	{
		VulkanExampleBase::prepareFrame();

		// Sparse bindings
//		vkQueueBindSparse(queue, 1, &bindSparseInfo, VK_NULL_HANDLE);
		//todo: use sparse bind semaphore
//		vkQueueWaitIdle(queue);

		// Command buffer to be sumitted to the queue
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];

		// Submit to queue
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

		VulkanExampleBase::submitFrame();
	}

	void generateQuad()
	{
		// Setup vertices for a single uv-mapped quad made from two triangles
		std::vector<Vertex> vertices =
		{
			{ {  1.0f,  1.0f, 0.0f }, { 1.0f, 1.0f },{ 0.0f, 0.0f, 1.0f } },
			{ { -1.0f,  1.0f, 0.0f }, { 0.0f, 1.0f },{ 0.0f, 0.0f, 1.0f } },
			{ { -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f },{ 0.0f, 0.0f, 1.0f } },
			{ {  1.0f, -1.0f, 0.0f }, { 1.0f, 0.0f },{ 0.0f, 0.0f, 1.0f } }
		};

		// Setup indices
		std::vector<uint32_t> indices = { 0,1,2, 2,3,0 };
		indexCount = static_cast<uint32_t>(indices.size());

		// Create buffers
		// For the sake of simplicity we won't stage the vertex data to the gpu memory
		// Vertex buffer
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&vertexBuffer,
			vertices.size() * sizeof(Vertex),
			vertices.data()));
		// Index buffer
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&indexBuffer,
			indices.size() * sizeof(uint32_t),
			indices.data()));
	}

	void setupVertexDescriptions()
	{
		// Binding description
		vertices.bindingDescriptions.resize(1);
		vertices.bindingDescriptions[0] =
			vkTools::initializers::vertexInputBindingDescription(
				VERTEX_BUFFER_BIND_ID, 
				sizeof(Vertex), 
				VK_VERTEX_INPUT_RATE_VERTEX);

		// Attribute descriptions
		// Describes memory layout and shader positions
		vertices.attributeDescriptions.resize(3);
		// Location 0 : Position
		vertices.attributeDescriptions[0] =
			vkTools::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				0,
				VK_FORMAT_R32G32B32_SFLOAT,
				offsetof(Vertex, pos));			
		// Location 1 : Texture coordinates
		vertices.attributeDescriptions[1] =
			vkTools::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				1,
				VK_FORMAT_R32G32_SFLOAT,
				offsetof(Vertex, uv));
		// Location 1 : Vertex normal
		vertices.attributeDescriptions[2] =
			vkTools::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				2,
				VK_FORMAT_R32G32B32_SFLOAT,
				offsetof(Vertex, normal));

		vertices.inputState = vkTools::initializers::pipelineVertexInputStateCreateInfo();
		vertices.inputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertices.bindingDescriptions.size());
		vertices.inputState.pVertexBindingDescriptions = vertices.bindingDescriptions.data();
		vertices.inputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertices.attributeDescriptions.size());
		vertices.inputState.pVertexAttributeDescriptions = vertices.attributeDescriptions.data();
	}

	void setupDescriptorPool()
	{
		// Example uses one ubo and one image sampler
		std::vector<VkDescriptorPoolSize> poolSizes =
		{
			vkTools::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1),
			vkTools::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
		};

		VkDescriptorPoolCreateInfo descriptorPoolInfo = 
			vkTools::initializers::descriptorPoolCreateInfo(
				static_cast<uint32_t>(poolSizes.size()),
				poolSizes.data(),
				2);

		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));
	}

	void setupDescriptorSetLayout()
	{
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = 
		{
			// Binding 0 : Vertex shader uniform buffer
			vkTools::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 
				VK_SHADER_STAGE_VERTEX_BIT, 
				0),
			// Binding 1 : Fragment shader image sampler
			vkTools::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 
				VK_SHADER_STAGE_FRAGMENT_BIT, 
				1)
		};

		VkDescriptorSetLayoutCreateInfo descriptorLayout = 
			vkTools::initializers::descriptorSetLayoutCreateInfo(
				setLayoutBindings.data(),
				static_cast<uint32_t>(setLayoutBindings.size()));

		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayout));

		VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo =
			vkTools::initializers::pipelineLayoutCreateInfo(
				&descriptorSetLayout,
				1);

		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &pipelineLayout));
	}

	void setupDescriptorSet()
	{
		VkDescriptorSetAllocateInfo allocInfo = 
			vkTools::initializers::descriptorSetAllocateInfo(
				descriptorPool,
				&descriptorSetLayout,
				1);

		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));

		std::vector<VkWriteDescriptorSet> writeDescriptorSets =
		{
			// Binding 0 : Vertex shader uniform buffer
			vkTools::initializers::writeDescriptorSet(
				descriptorSet, 
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 
				0, 
				&uniformBufferVS.descriptor),
			// Binding 1 : Fragment shader texture sampler
			vkTools::initializers::writeDescriptorSet(
				descriptorSet, 
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 
				1, 
				&texture.descriptor)
		};

		vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, NULL);
	}

	void preparePipelines()
	{
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
			vkTools::initializers::pipelineInputAssemblyStateCreateInfo(
				VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
				0,
				VK_FALSE);

		VkPipelineRasterizationStateCreateInfo rasterizationState =
			vkTools::initializers::pipelineRasterizationStateCreateInfo(
				VK_POLYGON_MODE_FILL,
				VK_CULL_MODE_NONE,
				VK_FRONT_FACE_COUNTER_CLOCKWISE,
				0);

		VkPipelineColorBlendAttachmentState blendAttachmentState =
			vkTools::initializers::pipelineColorBlendAttachmentState(
				0xf,
				VK_FALSE);

		VkPipelineColorBlendStateCreateInfo colorBlendState =
			vkTools::initializers::pipelineColorBlendStateCreateInfo(
				1, 
				&blendAttachmentState);

		VkPipelineDepthStencilStateCreateInfo depthStencilState =
			vkTools::initializers::pipelineDepthStencilStateCreateInfo(
				VK_TRUE,
				VK_TRUE,
				VK_COMPARE_OP_LESS_OR_EQUAL);

		VkPipelineViewportStateCreateInfo viewportState =
			vkTools::initializers::pipelineViewportStateCreateInfo(1, 1, 0);

		VkPipelineMultisampleStateCreateInfo multisampleState =
			vkTools::initializers::pipelineMultisampleStateCreateInfo(
				VK_SAMPLE_COUNT_1_BIT,
				0);

		std::vector<VkDynamicState> dynamicStateEnables = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};
		VkPipelineDynamicStateCreateInfo dynamicState =
			vkTools::initializers::pipelineDynamicStateCreateInfo(
				dynamicStateEnables.data(),
				static_cast<uint32_t>(dynamicStateEnables.size()),
				0);

		// Load shaders
		std::array<VkPipelineShaderStageCreateInfo,2> shaderStages;

		shaderStages[0] = loadShader(getAssetPath() + "shaders/texturesparseresidency/sparseresidency.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getAssetPath() + "shaders/texturesparseresidency/sparseresidency.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

		VkGraphicsPipelineCreateInfo pipelineCreateInfo =
			vkTools::initializers::pipelineCreateInfo(
				pipelineLayout,
				renderPass,
				0);

		pipelineCreateInfo.pVertexInputState = &vertices.inputState;
		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
		pipelineCreateInfo.pRasterizationState = &rasterizationState;
		pipelineCreateInfo.pColorBlendState = &colorBlendState;
		pipelineCreateInfo.pMultisampleState = &multisampleState;
		pipelineCreateInfo.pViewportState = &viewportState;
		pipelineCreateInfo.pDepthStencilState = &depthStencilState;
		pipelineCreateInfo.pDynamicState = &dynamicState;
		pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineCreateInfo.pStages = shaderStages.data();

		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.solid));
	}

	// Prepare and initialize uniform buffer containing shader uniforms
	void prepareUniformBuffers()
	{
		// Vertex shader uniform buffer block
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&uniformBufferVS,
			sizeof(uboVS),
			&uboVS));

		updateUniformBuffers();
	}

	void updateUniformBuffers()
	{
		// Vertex shader
		uboVS.projection = glm::perspective(glm::radians(60.0f), (float)width / (float)height, 0.001f, 256.0f);
		glm::mat4 viewMatrix = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, zoom));

		uboVS.model = viewMatrix * glm::translate(glm::mat4(), cameraPos);
		uboVS.model = glm::rotate(uboVS.model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		uboVS.model = glm::rotate(uboVS.model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		uboVS.model = glm::rotate(uboVS.model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

		uboVS.viewPos = glm::vec4(0.0f, 0.0f, -zoom, 0.0f);

		VK_CHECK_RESULT(uniformBufferVS.map());
		memcpy(uniformBufferVS.mapped, &uboVS, sizeof(uboVS));
		uniformBufferVS.unmap();
	}

	void prepare()
	{
		VulkanExampleBase::prepare();
		generateQuad();
		setupVertexDescriptions();
		prepareUniformBuffers();
		prepareSparseTexture(8192, 8192, 1, VK_FORMAT_R8G8B8A8_UNORM);
		setupDescriptorSetLayout();
		preparePipelines();
		setupDescriptorPool();
		setupDescriptorSet();
		buildCommandBuffers();
		prepared = true;
	}

	virtual void render()
	{
		if (!prepared)
			return;
		draw();
	}

	virtual void viewChanged()
	{
		updateUniformBuffers();
	}

	void changeLodBias(float delta)
	{
		uboVS.lodBias += delta;
		if (uboVS.lodBias < 0.0f)
		{
			uboVS.lodBias = 0.0f;
		}
		if (uboVS.lodBias > texture.mipLevels)
		{
			uboVS.lodBias = (float)texture.mipLevels;
		}
		updateUniformBuffers();
		updateTextOverlay();
	}

	// Clear all pages of the virtual texture
	// todo: just for testing
	void flushVirtualTexture()
	{
		vkDeviceWaitIdle(device);
		for (auto& page : texture.pages)
		{
			page.release(device);
		}
		texture.updateSparseBindInfo();
		vkQueueBindSparse(queue, 1, &texture.bindSparseInfo, VK_NULL_HANDLE);
		//todo: use sparse bind semaphore
		vkQueueWaitIdle(queue);
	}

	// Randomly fill pages
	// todo: just for testing
	void fillVirtualTexture()
	{
		vkDeviceWaitIdle(device);
		std::default_random_engine rndEngine(std::random_device{}());
		std::uniform_real_distribution<float> rndDist(0.0f, 1.0f);
		for (auto& page : texture.pages)
		{
			if (rndDist(rndEngine) < 0.5f)
			{
				page.allocate(device, memoryTypeIndex);
			}
		}
		texture.updateSparseBindInfo();
		vkQueueBindSparse(queue, 1, &texture.bindSparseInfo, VK_NULL_HANDLE);
		//todo: use sparse bind semaphore
		vkQueueWaitIdle(queue);
	}

	virtual void keyPressed(uint32_t keyCode)
	{
		switch (keyCode)
		{
		case KEY_KPADD:
		case GAMEPAD_BUTTON_R1:
			changeLodBias(0.1f);
			break;
		case KEY_KPSUB:
		case GAMEPAD_BUTTON_L1:
			changeLodBias(-0.1f);
			break;
		case KEY_F:
			flushVirtualTexture();
			break;
		case KEY_N:
			fillVirtualTexture();
			break;
		}
	}

	virtual void getOverlayText(VulkanTextOverlay *textOverlay)
	{
		std::stringstream ss;
		ss << std::setprecision(2) << std::fixed << uboVS.lodBias;
#if defined(__ANDROID__)
		textOverlay->addText("LOD bias: " + ss.str() + " (Buttons L1/R1 to change)", 5.0f, 85.0f, VulkanTextOverlay::alignLeft);
#else
		textOverlay->addText("LOD bias: " + ss.str() + " (numpad +/- to change)", 5.0f, 85.0f, VulkanTextOverlay::alignLeft);
#endif
	}
};

VULKAN_EXAMPLE_MAIN()
