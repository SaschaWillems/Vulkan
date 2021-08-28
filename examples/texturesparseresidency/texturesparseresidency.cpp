/*
* Vulkan Example - Sparse texture residency example
*
* Copyright (C) 2016-2020 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

/*
* Note : This sample is work-in-progress and works basically, but it's not yet finished
*/

#include "texturesparseresidency.h"

/*
	Virtual texture page 
	Contains all functions and objects for a single page of a virtual texture
 */

VirtualTexturePage::VirtualTexturePage()
{
	// Pages are initially not backed up by memory (non-resident)
	imageMemoryBind.memory = VK_NULL_HANDLE;
}

bool VirtualTexturePage::resident()
{
	return (imageMemoryBind.memory != VK_NULL_HANDLE);
}

// Allocate Vulkan memory for the virtual page
void VirtualTexturePage::allocate(VkDevice device, uint32_t memoryTypeIndex)
{
	if (imageMemoryBind.memory != VK_NULL_HANDLE)
	{
		return;
	};

	imageMemoryBind = {};

	VkMemoryAllocateInfo allocInfo = vks::initializers::memoryAllocateInfo();
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
void VirtualTexturePage::release(VkDevice device)
{
	if (imageMemoryBind.memory != VK_NULL_HANDLE)
	{
		vkFreeMemory(device, imageMemoryBind.memory, nullptr);
		imageMemoryBind.memory = VK_NULL_HANDLE;
	}
}

/*
	Virtual texture 
	Contains the virtual pages and memory binding information for a whole virtual texture
 */

VirtualTexturePage* VirtualTexture::addPage(VkOffset3D offset, VkExtent3D extent, const VkDeviceSize size, const uint32_t mipLevel, uint32_t layer)
{
	VirtualTexturePage newPage{};
	newPage.offset = offset;
	newPage.extent = extent;
	newPage.size = size;
	newPage.mipLevel = mipLevel;
	newPage.layer = layer;
	newPage.index = static_cast<uint32_t>(pages.size());
	newPage.imageMemoryBind = {};
	newPage.imageMemoryBind.offset = offset;
	newPage.imageMemoryBind.extent = extent;
	pages.push_back(newPage);
	return &pages.back();
}

// Call before sparse binding to update memory bind list etc.
void VirtualTexture::updateSparseBindInfo()
{
	// Update list of memory-backed sparse image memory binds
	//sparseImageMemoryBinds.resize(pages.size());
	sparseImageMemoryBinds.clear();
	for (auto page : pages)
	{
		sparseImageMemoryBinds.push_back(page.imageMemoryBind);
	}
	// Update sparse bind info
	bindSparseInfo = vks::initializers::bindSparseInfo();
	// todo: Semaphore for queue submission
	// bindSparseInfo.signalSemaphoreCount = 1;
	// bindSparseInfo.pSignalSemaphores = &bindSparseSemaphore;

	// Image memory binds
	imageMemoryBindInfo = {};
	imageMemoryBindInfo.image = image;
	imageMemoryBindInfo.bindCount = static_cast<uint32_t>(sparseImageMemoryBinds.size());
	imageMemoryBindInfo.pBinds = sparseImageMemoryBinds.data();
	bindSparseInfo.imageBindCount = (imageMemoryBindInfo.bindCount > 0) ? 1 : 0;
	bindSparseInfo.pImageBinds = &imageMemoryBindInfo;

	// Opaque image memory binds for the mip tail
	opaqueMemoryBindInfo.image = image;
	opaqueMemoryBindInfo.bindCount = static_cast<uint32_t>(opaqueMemoryBinds.size());
	opaqueMemoryBindInfo.pBinds = opaqueMemoryBinds.data();
	bindSparseInfo.imageOpaqueBindCount = (opaqueMemoryBindInfo.bindCount > 0) ? 1 : 0;
	bindSparseInfo.pImageOpaqueBinds = &opaqueMemoryBindInfo;
}

// Release all Vulkan resources
void VirtualTexture::destroy()
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

/*
	Vulkan Example class
*/
VulkanExample::VulkanExample() : VulkanExampleBase(ENABLE_VALIDATION)
{
	title = "Sparse texture residency";
	std::cout.imbue(std::locale(""));
	camera.type = Camera::CameraType::lookat;
	camera.setPosition(glm::vec3(0.0f, 0.0f, -12.0f));
	camera.setRotation(glm::vec3(-90.0f, 0.0f, 0.0f));
	camera.setPerspective(60.0f, (float)width / (float)height, 0.1f, 256.0f);
}

VulkanExample::~VulkanExample()
{
	// Clean up used Vulkan resources
	// Note : Inherited destructor cleans up resources stored in base class
	destroyTextureImage(texture);
	vkDestroySemaphore(device, bindSparseSemaphore, nullptr);
	vkDestroyPipeline(device, pipeline, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
	uniformBufferVS.destroy();
}

void VulkanExample::getEnabledFeatures()
{
	if (deviceFeatures.sparseBinding && deviceFeatures.sparseResidencyImage2D) {
		enabledFeatures.shaderResourceResidency = VK_TRUE;
		enabledFeatures.shaderResourceMinLod = VK_TRUE;
		enabledFeatures.sparseBinding = VK_TRUE;
		enabledFeatures.sparseResidencyImage2D = VK_TRUE;
	}
	else {
		std::cout << "Sparse binding not supported" << std::endl;
	}
}

glm::uvec3 VulkanExample::alignedDivision(const VkExtent3D& extent, const VkExtent3D& granularity)
{
	glm::uvec3 res;
	res.x = extent.width / granularity.width + ((extent.width % granularity.width) ? 1u : 0u);
	res.y = extent.height / granularity.height + ((extent.height % granularity.height) ? 1u : 0u);
	res.z = extent.depth / granularity.depth + ((extent.depth % granularity.depth) ? 1u : 0u);
	return res;
}

void VulkanExample::prepareSparseTexture(uint32_t width, uint32_t height, uint32_t layerCount, VkFormat format)
{
	texture.device = vulkanDevice->logicalDevice;
	texture.width = width;
	texture.height = height;
	texture.mipLevels = floor(log2(std::max(width, height))) + 1;
	texture.layerCount = layerCount;
	texture.format = format;

	// Get device properties for the requested texture format
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);

	const VkImageType imageType = VK_IMAGE_TYPE_2D;
	const VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT;
	const VkImageUsageFlags imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	const VkImageTiling imageTiling = VK_IMAGE_TILING_OPTIMAL;

	// Get sparse image properties
	std::vector<VkSparseImageFormatProperties> sparseProperties;
	// Sparse properties count for the desired format
	uint32_t sparsePropertiesCount;
	vkGetPhysicalDeviceSparseImageFormatProperties(physicalDevice, format, imageType, sampleCount, imageUsage, imageTiling, &sparsePropertiesCount, nullptr);
	// Check if sparse is supported for this format
	if (sparsePropertiesCount == 0)
	{
		std::cout << "Error: Requested format does not support sparse features!" << std::endl;
		return;
	}

	// Get actual image format properties
	sparseProperties.resize(sparsePropertiesCount);
	vkGetPhysicalDeviceSparseImageFormatProperties(physicalDevice, format, imageType, sampleCount, imageUsage, imageTiling, &sparsePropertiesCount, sparseProperties.data());

	std::cout << "Sparse image format properties: " << sparsePropertiesCount << std::endl;
	for (auto props : sparseProperties)
	{
		std::cout << "\t Image granularity: w = " << props.imageGranularity.width << " h = " << props.imageGranularity.height << " d = " << props.imageGranularity.depth << std::endl;
		std::cout << "\t Aspect mask: " << props.aspectMask << std::endl;
		std::cout << "\t Flags: " << props.flags << std::endl;
	}

	// Create sparse image
	VkImageCreateInfo sparseImageCreateInfo = vks::initializers::imageCreateInfo();
	sparseImageCreateInfo.imageType = imageType;
	sparseImageCreateInfo.format = texture.format;
	sparseImageCreateInfo.mipLevels = texture.mipLevels;
	sparseImageCreateInfo.arrayLayers = texture.layerCount;
	sparseImageCreateInfo.samples = sampleCount;
	sparseImageCreateInfo.tiling = imageTiling;
	sparseImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	sparseImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	sparseImageCreateInfo.extent = { texture.width, texture.height, 1 };
	sparseImageCreateInfo.usage = imageUsage;
	sparseImageCreateInfo.flags = VK_IMAGE_CREATE_SPARSE_BINDING_BIT | VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT;
	VK_CHECK_RESULT(vkCreateImage(device, &sparseImageCreateInfo, nullptr, &texture.image));

	VkCommandBuffer copyCmd = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
	vks::tools::setImageLayout(copyCmd, texture.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	vulkanDevice->flushCommandBuffer(copyCmd, queue);

	// Get memory requirements
	VkMemoryRequirements sparseImageMemoryReqs;
	// Sparse image memory requirement counts
	vkGetImageMemoryRequirements(device, texture.image, &sparseImageMemoryReqs);

	std::cout << "Image memory requirements:" << std::endl;
	std::cout << "\t Size: " << sparseImageMemoryReqs.size << std::endl;
	std::cout << "\t Alignment: " << sparseImageMemoryReqs.alignment << std::endl;

	// Check requested image size against hardware sparse limit
	if (sparseImageMemoryReqs.size > vulkanDevice->properties.limits.sparseAddressSpaceSize)
	{
		std::cout << "Error: Requested sparse image size exceeds supports sparse address space size!" << std::endl;
		return;
	};

	// Get sparse memory requirements
	// Count
	uint32_t sparseMemoryReqsCount = 32;
	std::vector<VkSparseImageMemoryRequirements> sparseMemoryReqs(sparseMemoryReqsCount);
	vkGetImageSparseMemoryRequirements(device, texture.image, &sparseMemoryReqsCount, sparseMemoryReqs.data());
	if (sparseMemoryReqsCount == 0)
	{
		std::cout << "Error: No memory requirements for the sparse image!" << std::endl;
		return;
	}
	sparseMemoryReqs.resize(sparseMemoryReqsCount);
	// Get actual requirements
	vkGetImageSparseMemoryRequirements(device, texture.image, &sparseMemoryReqsCount, sparseMemoryReqs.data());

	std::cout << "Sparse image memory requirements: " << sparseMemoryReqsCount << std::endl;
	for (auto reqs : sparseMemoryReqs)
	{
		std::cout << "\t Image granularity: w = " << reqs.formatProperties.imageGranularity.width << " h = " << reqs.formatProperties.imageGranularity.height << " d = " << reqs.formatProperties.imageGranularity.depth << std::endl;
		std::cout << "\t Mip tail first LOD: " << reqs.imageMipTailFirstLod << std::endl;
		std::cout << "\t Mip tail size: " << reqs.imageMipTailSize << std::endl;
		std::cout << "\t Mip tail offset: " << reqs.imageMipTailOffset << std::endl;
		std::cout << "\t Mip tail stride: " << reqs.imageMipTailStride << std::endl;
		//todo:multiple reqs
		texture.mipTailStart = reqs.imageMipTailFirstLod;
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

	// @todo: proper comment
	// Calculate number of required sparse memory bindings by alignment
	assert((sparseImageMemoryReqs.size % sparseImageMemoryReqs.alignment) == 0);
	texture.memoryTypeIndex = vulkanDevice->getMemoryType(sparseImageMemoryReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// Get sparse bindings
	uint32_t sparseBindsCount = static_cast<uint32_t>(sparseImageMemoryReqs.size / sparseImageMemoryReqs.alignment);
	std::vector<VkSparseMemoryBind>	sparseMemoryBinds(sparseBindsCount);

	texture.sparseImageMemoryRequirements = sparseMemoryReq;

	// The mip tail contains all mip levels > sparseMemoryReq.imageMipTailFirstLod
	// Check if the format has a single mip tail for all layers or one mip tail for each layer
	// @todo: Comment
	texture.mipTailInfo.singleMipTail = sparseMemoryReq.formatProperties.flags & VK_SPARSE_IMAGE_FORMAT_SINGLE_MIPTAIL_BIT;
	texture.mipTailInfo.alingedMipSize = sparseMemoryReq.formatProperties.flags & VK_SPARSE_IMAGE_FORMAT_ALIGNED_MIP_SIZE_BIT;

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

			// @todo: Comment
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
						VirtualTexturePage* newPage = texture.addPage(offset, extent, sparseImageMemoryReqs.alignment, mipLevel, layer);
						newPage->imageMemoryBind.subresource = subResource;

						index++;
					}
				}
			}
		}

		// @todo: proper comment
		// @todo: store in mip tail and properly release
		// @todo: Only one block for single mip tail
		if ((!texture.mipTailInfo.singleMipTail) && (sparseMemoryReq.imageMipTailFirstLod < texture.mipLevels))
		{
			// Allocate memory for the mip tail
			VkMemoryAllocateInfo allocInfo = vks::initializers::memoryAllocateInfo();
			allocInfo.allocationSize = sparseMemoryReq.imageMipTailSize;
			allocInfo.memoryTypeIndex = texture.memoryTypeIndex;

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
		VkMemoryAllocateInfo allocInfo = vks::initializers::memoryAllocateInfo();
		allocInfo.allocationSize = sparseMemoryReq.imageMipTailSize;
		allocInfo.memoryTypeIndex = texture.memoryTypeIndex;

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
	VkSemaphoreCreateInfo semaphoreCreateInfo = vks::initializers::semaphoreCreateInfo();
	VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &bindSparseSemaphore));

	// Prepare bind sparse info for reuse in queue submission
	texture.updateSparseBindInfo();

	// Bind to queue
	// todo: in draw?
	vkQueueBindSparse(queue, 1, &texture.bindSparseInfo, VK_NULL_HANDLE);
	//todo: use sparse bind semaphore
	vkQueueWaitIdle(queue);

	// Create sampler
	VkSamplerCreateInfo sampler = vks::initializers::samplerCreateInfo();
	sampler.magFilter = VK_FILTER_LINEAR;
	sampler.minFilter = VK_FILTER_LINEAR;
	sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler.mipLodBias = 0.0f;
	sampler.compareOp = VK_COMPARE_OP_NEVER;
	sampler.minLod = 0.0f;
	sampler.maxLod = static_cast<float>(texture.mipLevels);
	sampler.maxAnisotropy = vulkanDevice->features.samplerAnisotropy ? vulkanDevice->properties.limits.maxSamplerAnisotropy : 1.0f;
	sampler.anisotropyEnable = false;
	sampler.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
	VK_CHECK_RESULT(vkCreateSampler(device, &sampler, nullptr, &texture.sampler));

	// Create image view
	VkImageViewCreateInfo view = vks::initializers::imageViewCreateInfo();
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
	texture.descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	texture.descriptor.imageView = texture.view;
	texture.descriptor.sampler = texture.sampler;
}

// Free all Vulkan resources used a texture object
void VulkanExample::destroyTextureImage(SparseTexture texture)
{
	vkDestroyImageView(device, texture.view, nullptr);
	vkDestroyImage(device, texture.image, nullptr);
	vkDestroySampler(device, texture.sampler, nullptr);
	texture.destroy();
}

void VulkanExample::buildCommandBuffers()
{
	VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

	VkClearValue clearValues[2];
	clearValues[0].color = defaultClearColor;
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
	renderPassBeginInfo.renderPass = renderPass;
	renderPassBeginInfo.renderArea.offset.x = 0;
	renderPassBeginInfo.renderArea.offset.y = 0;
	renderPassBeginInfo.renderArea.extent.width = width;
	renderPassBeginInfo.renderArea.extent.height = height;
	renderPassBeginInfo.clearValueCount = 2;
	renderPassBeginInfo.pClearValues = clearValues;

	for (int32_t i = 0; i < drawCmdBuffers.size(); ++i)
	{
		renderPassBeginInfo.framebuffer = frameBuffers[i];

		VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo));

		vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
		vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

		VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);
		vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

		vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);
		vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		plane.draw(drawCmdBuffers[i]);

		drawUI(drawCmdBuffers[i]);

		vkCmdEndRenderPass(drawCmdBuffers[i]);

		VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
	}
}

void VulkanExample::draw()
{
	VulkanExampleBase::prepareFrame();
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];
	VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
	VulkanExampleBase::submitFrame();
}

void VulkanExample::loadAssets()
{
	const uint32_t glTFLoadingFlags = vkglTF::FileLoadingFlags::PreTransformVertices | vkglTF::FileLoadingFlags::PreMultiplyVertexColors | vkglTF::FileLoadingFlags::FlipY;
	plane.loadFromFile(getAssetPath() + "models/plane.gltf", vulkanDevice, queue, glTFLoadingFlags);
}

void VulkanExample::setupDescriptorPool()
{
	// Example uses one ubo and one image sampler
	std::vector<VkDescriptorPoolSize> poolSizes =
	{
		vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1),
		vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
	};

	VkDescriptorPoolCreateInfo descriptorPoolInfo =
		vks::initializers::descriptorPoolCreateInfo(
			static_cast<uint32_t>(poolSizes.size()),
			poolSizes.data(),
			2);

	VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));
}

void VulkanExample::setupDescriptorSetLayout()
{
	std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings =
	{
		// Binding 0 : Vertex shader uniform buffer
		vks::initializers::descriptorSetLayoutBinding(
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			VK_SHADER_STAGE_VERTEX_BIT,
			0),
		// Binding 1 : Fragment shader image sampler
		vks::initializers::descriptorSetLayoutBinding(
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			1)
	};

	VkDescriptorSetLayoutCreateInfo descriptorLayout =
		vks::initializers::descriptorSetLayoutCreateInfo(
			setLayoutBindings.data(),
			static_cast<uint32_t>(setLayoutBindings.size()));

	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayout));

	VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo =
		vks::initializers::pipelineLayoutCreateInfo(
			&descriptorSetLayout,
			1);

	VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &pipelineLayout));
}

void VulkanExample::setupDescriptorSet()
{
	VkDescriptorSetAllocateInfo allocInfo =
		vks::initializers::descriptorSetAllocateInfo(
			descriptorPool,
			&descriptorSetLayout,
			1);

	VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));

	std::vector<VkWriteDescriptorSet> writeDescriptorSets =
	{
		// Binding 0 : Vertex shader uniform buffer
		vks::initializers::writeDescriptorSet(
			descriptorSet,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			0,
			&uniformBufferVS.descriptor),
		// Binding 1 : Fragment shader texture sampler
		vks::initializers::writeDescriptorSet(
			descriptorSet,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			1,
			&texture.descriptor)
	};

	vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, NULL);
}

void VulkanExample::preparePipelines()
{
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
	VkPipelineRasterizationStateCreateInfo rasterizationState = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
	VkPipelineColorBlendAttachmentState blendAttachmentState = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
	VkPipelineColorBlendStateCreateInfo colorBlendState = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
	VkPipelineDepthStencilStateCreateInfo depthStencilState = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
	VkPipelineViewportStateCreateInfo viewportState = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
	VkPipelineMultisampleStateCreateInfo multisampleState = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
	std::vector<VkDynamicState> dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
	VkPipelineDynamicStateCreateInfo dynamicState = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
	std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

	VkGraphicsPipelineCreateInfo pipelineCI = vks::initializers::pipelineCreateInfo( pipelineLayout, renderPass);
	pipelineCI.pInputAssemblyState = &inputAssemblyState;
	pipelineCI.pRasterizationState = &rasterizationState;
	pipelineCI.pColorBlendState = &colorBlendState;
	pipelineCI.pMultisampleState = &multisampleState;
	pipelineCI.pViewportState = &viewportState;
	pipelineCI.pDepthStencilState = &depthStencilState;
	pipelineCI.pDynamicState = &dynamicState;
	pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
	pipelineCI.pStages = shaderStages.data();
	pipelineCI.pVertexInputState = vkglTF::Vertex::getPipelineVertexInputState({ vkglTF::VertexComponent::Position, vkglTF::VertexComponent::Normal, vkglTF::VertexComponent::UV });

	shaderStages[0] = loadShader(getShadersPath() + "texturesparseresidency/sparseresidency.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = loadShader(getShadersPath() + "texturesparseresidency/sparseresidency.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipeline));
}

// Prepare and initialize uniform buffer containing shader uniforms
void VulkanExample::prepareUniformBuffers()
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

void VulkanExample::updateUniformBuffers()
{
	uboVS.projection = camera.matrices.perspective;
	uboVS.model = camera.matrices.view;
	uboVS.viewPos = camera.viewPos;

	VK_CHECK_RESULT(uniformBufferVS.map());
	memcpy(uniformBufferVS.mapped, &uboVS, sizeof(uboVS));
	uniformBufferVS.unmap();
}

void VulkanExample::prepare()
{
	VulkanExampleBase::prepare();
	// Check if the GPU supports sparse residency for 2D images
	if (!vulkanDevice->features.sparseResidencyImage2D) {
		vks::tools::exitFatal("Device does not support sparse residency for 2D images!", VK_ERROR_FEATURE_NOT_PRESENT);
	}
	loadAssets();
	prepareUniformBuffers();
	// Create a virtual texture with max. possible dimension (does not take up any VRAM yet)
	prepareSparseTexture(4096, 4096, 1, VK_FORMAT_R8G8B8A8_UNORM);
	setupDescriptorSetLayout();
	preparePipelines();
	setupDescriptorPool();
	setupDescriptorSet();
	buildCommandBuffers();
	prepared = true;
}

void VulkanExample::render()
{
	if (!prepared)
		return;
	draw();
	if (camera.updated) {
		updateUniformBuffers();
	}
}

void VulkanExample::uploadContent(VirtualTexturePage page, VkImage image)
{
	// Generate some random image data and upload as a buffer
	const size_t bufferSize = 4 * page.extent.width * page.extent.height;

	vks::Buffer imageBuffer;
	VK_CHECK_RESULT(vulkanDevice->createBuffer(
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&imageBuffer,
		bufferSize));
	imageBuffer.map();

	// Fill buffer with random colors
	std::random_device rd;
	std::mt19937 rndEngine(rd());
	std::uniform_int_distribution<uint32_t> rndDist(0, 255);
	uint8_t* data = (uint8_t*)imageBuffer.mapped;
	uint8_t rndVal[4] = { 0, 0, 0, 0 };
	while (rndVal[0] + rndVal[1] + rndVal[2] < 10) {
		rndVal[0] = (uint8_t)rndDist(rndEngine);
		rndVal[1] = (uint8_t)rndDist(rndEngine);
		rndVal[2] = (uint8_t)rndDist(rndEngine);
	}
	rndVal[3] = 255;

	for (uint32_t y = 0; y < page.extent.height; y++)
	{
		for (uint32_t x = 0; x < page.extent.width; x++)
		{
			for (uint32_t c = 0; c < 4; c++, ++data)
			{
				*data = rndVal[c];
			}
		}
	}

	VkCommandBuffer copyCmd = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
	vks::tools::setImageLayout(copyCmd, image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
	VkBufferImageCopy region{};
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.layerCount = 1;
	region.imageSubresource.mipLevel = page.mipLevel;
	region.imageOffset = page.offset;
	region.imageExtent = page.extent;
	vkCmdCopyBufferToImage(copyCmd, imageBuffer.buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	vks::tools::setImageLayout(copyCmd, image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
	vulkanDevice->flushCommandBuffer(copyCmd, queue);

	imageBuffer.destroy();
}
	
void VulkanExample::fillRandomPages()
{
	vkDeviceWaitIdle(device);

	std::default_random_engine rndEngine(std::random_device{}());
	std::uniform_real_distribution<float> rndDist(0.0f, 1.0f);

	std::vector<VirtualTexturePage> updatedPages;
	for (auto& page : texture.pages) {
		if (rndDist(rndEngine) < 0.5f) {
			continue;
		}
		page.allocate(device, texture.memoryTypeIndex);
		updatedPages.push_back(page);
	}

	// Update sparse queue binding
	texture.updateSparseBindInfo();
	VkFenceCreateInfo fenceInfo = vks::initializers::fenceCreateInfo(VK_FLAGS_NONE);
	VkFence fence;
	VK_CHECK_RESULT(vkCreateFence(device, &fenceInfo, nullptr, &fence));
	vkQueueBindSparse(queue, 1, &texture.bindSparseInfo, fence);
	vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);

	for (auto &page: updatedPages) {
		uploadContent(page, texture.image);
	}
}

void VulkanExample::fillMipTail()
{
	//@todo: WIP
	VkDeviceSize imageMipTailSize = texture.sparseImageMemoryRequirements.imageMipTailSize;
	VkDeviceSize imageMipTailOffset = texture.sparseImageMemoryRequirements.imageMipTailOffset;
	// Stride between memory bindings for each mip level if not single mip tail (VK_SPARSE_IMAGE_FORMAT_SINGLE_MIPTAIL_BIT not set)
	VkDeviceSize imageMipTailStride = texture.sparseImageMemoryRequirements.imageMipTailStride;

	VkSparseImageMemoryBind mipTailimageMemoryBind{};

	VkMemoryAllocateInfo allocInfo = vks::initializers::memoryAllocateInfo();
	allocInfo.allocationSize = imageMipTailSize;
	allocInfo.memoryTypeIndex = texture.memoryTypeIndex;
	VK_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, &mipTailimageMemoryBind.memory));

	uint32_t mipLevel = texture.sparseImageMemoryRequirements.imageMipTailFirstLod;
	uint32_t width = std::max(texture.width >> texture.sparseImageMemoryRequirements.imageMipTailFirstLod, 1u);
	uint32_t height = std::max(texture.height >> texture.sparseImageMemoryRequirements.imageMipTailFirstLod, 1u);
	uint32_t depth = 1;

	for (uint32_t i = texture.mipTailStart; i < texture.mipLevels; i++) {

		const uint32_t width = std::max(texture.width >> i, 1u);
		const uint32_t height = std::max(texture.height >> i, 1u);
		const uint32_t depth = 1;

		// Generate some random image data and upload as a buffer
		const size_t bufferSize = 4 * width * height;

		vks::Buffer imageBuffer;
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&imageBuffer,
			bufferSize));
		imageBuffer.map();

		// Fill buffer with random colors
		std::random_device rd;
		std::mt19937 rndEngine(rd());
		std::uniform_int_distribution<uint32_t> rndDist(0, 255);
		uint8_t* data = (uint8_t*)imageBuffer.mapped;
		uint8_t rndVal[4] = { 0, 0, 0, 0 };
		while (rndVal[0] + rndVal[1] + rndVal[2] < 10) {
			rndVal[0] = (uint8_t)rndDist(rndEngine);
			rndVal[1] = (uint8_t)rndDist(rndEngine);
			rndVal[2] = (uint8_t)rndDist(rndEngine);
		}
		rndVal[3] = 255;

		switch (mipLevel) {
		case 0:
			rndVal[0] = rndVal[1] = rndVal[2] = 255;
			break;
		case 1:
			rndVal[0] = rndVal[1] = rndVal[2] = 200;
			break;
		case 2:
			rndVal[0] = rndVal[1] = rndVal[2] = 150;
			break;
		}

		for (uint32_t y = 0; y < height; y++)
		{
			for (uint32_t x = 0; x < width; x++)
			{
				for (uint32_t c = 0; c < 4; c++, ++data)
				{
					*data = rndVal[c];
				}
			}
		}

		VkCommandBuffer copyCmd = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
		vks::tools::setImageLayout(copyCmd, texture.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
		VkBufferImageCopy region{};
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.layerCount = 1;
		region.imageSubresource.mipLevel = i;
		region.imageOffset = {};
		region.imageExtent = { width, height, depth };
		vkCmdCopyBufferToImage(copyCmd, imageBuffer.buffer, texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
		vks::tools::setImageLayout(copyCmd, texture.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		vulkanDevice->flushCommandBuffer(copyCmd, queue);

		imageBuffer.destroy();
	}
}

void VulkanExample::flushRandomPages()
{
	vkDeviceWaitIdle(device);

	std::default_random_engine rndEngine(std::random_device{}());
	std::uniform_real_distribution<float> rndDist(0.0f, 1.0f);

	std::vector<VirtualTexturePage> updatedPages;
	for (auto& page : texture.pages)
	{
		if (rndDist(rndEngine) < 0.5f) {
			continue;
		}
		page.release(device);
	}

	// Update sparse queue binding
	texture.updateSparseBindInfo();
	VkFenceCreateInfo fenceInfo = vks::initializers::fenceCreateInfo(VK_FLAGS_NONE);
	VkFence fence;
	VK_CHECK_RESULT(vkCreateFence(device, &fenceInfo, nullptr, &fence));
	vkQueueBindSparse(queue, 1, &texture.bindSparseInfo, fence);
	vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
}

void VulkanExample::OnUpdateUIOverlay(vks::UIOverlay* overlay)
{
	if (overlay->header("Settings")) {
		if (overlay->sliderFloat("LOD bias", &uboVS.lodBias, -(float)texture.mipLevels, (float)texture.mipLevels)) {
			updateUniformBuffers();
		}
		if (overlay->button("Fill random pages")) {
			fillRandomPages();
		}
		if (overlay->button("Flush random pages")) {
			flushRandomPages();
		}
		if (overlay->button("Fill mip tail")) {
			fillMipTail();
		}
	}
	if (overlay->header("Statistics")) {
		uint32_t respages = 0;
		std::for_each(texture.pages.begin(), texture.pages.end(), [&respages](VirtualTexturePage page) { respages += (page.resident()) ? 1 : 0; });
		overlay->text("Resident pages: %d of %d", respages, static_cast<uint32_t>(texture.pages.size()));
		overlay->text("Mip tail starts at: %d", texture.mipTailStart);
	}

}

VULKAN_EXAMPLE_MAIN()
