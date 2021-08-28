/*
* Vulkan Example - Variable rate shading
*
* Copyright (C) 2020 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "variablerateshading.h"

VulkanExample::VulkanExample() : VulkanExampleBase(ENABLE_VALIDATION)
{
	title = "Variable rate shading";
	apiVersion = VK_API_VERSION_1_1;
	camera.type = Camera::CameraType::firstperson;
	camera.flipY = true;
	camera.setPosition(glm::vec3(0.0f, 1.0f, 0.0f));
	camera.setRotation(glm::vec3(0.0f, -90.0f, 0.0f));
	camera.setPerspective(60.0f, (float)width / (float)height, 0.1f, 256.0f);
	camera.setRotationSpeed(0.25f);
	enabledInstanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	enabledDeviceExtensions.push_back(VK_NV_SHADING_RATE_IMAGE_EXTENSION_NAME);
}

VulkanExample::~VulkanExample()
{
	vkDestroyPipeline(device, basePipelines.masked, nullptr);
	vkDestroyPipeline(device, basePipelines.opaque, nullptr);
	vkDestroyPipeline(device, shadingRatePipelines.masked, nullptr);
	vkDestroyPipeline(device, shadingRatePipelines.opaque, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
	vkDestroyImageView(device, shadingRateImage.view, nullptr);
	vkDestroyImage(device, shadingRateImage.image, nullptr);
	vkFreeMemory(device, shadingRateImage.memory, nullptr);
	shaderData.buffer.destroy();
}

void VulkanExample::getEnabledFeatures()
{
	enabledFeatures.samplerAnisotropy = deviceFeatures.samplerAnisotropy;
	// POI
	enabledPhysicalDeviceShadingRateImageFeaturesNV = {};
	enabledPhysicalDeviceShadingRateImageFeaturesNV.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADING_RATE_IMAGE_FEATURES_NV;
	enabledPhysicalDeviceShadingRateImageFeaturesNV.shadingRateImage = VK_TRUE;
	deviceCreatepNextChain = &enabledPhysicalDeviceShadingRateImageFeaturesNV;
}

/*
	If the window has been resized, we need to recreate the shading rate image
*/
void VulkanExample::handleResize()
{
	// Delete allocated resources
	vkDestroyImageView(device, shadingRateImage.view, nullptr);
	vkDestroyImage(device, shadingRateImage.image, nullptr);
	vkFreeMemory(device, shadingRateImage.memory, nullptr);
	// Recreate image
	prepareShadingRateImage();
}

void VulkanExample::buildCommandBuffers()
{
	if (resized)
	{
		handleResize();
	}

	VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

	VkClearValue clearValues[2];
	clearValues[0].color = defaultClearColor;
	clearValues[0].color = { { 0.25f, 0.25f, 0.25f, 1.0f } };;
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
	renderPassBeginInfo.renderPass = renderPass;
	renderPassBeginInfo.renderArea.offset.x = 0;
	renderPassBeginInfo.renderArea.offset.y = 0;
	renderPassBeginInfo.renderArea.extent.width = width;
	renderPassBeginInfo.renderArea.extent.height = height;
	renderPassBeginInfo.clearValueCount = 2;
	renderPassBeginInfo.pClearValues = clearValues;

	const VkViewport viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
	const VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);

	for (int32_t i = 0; i < drawCmdBuffers.size(); ++i)
	{
		renderPassBeginInfo.framebuffer = frameBuffers[i];
		VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo));
		vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);
		vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);
		vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

		// POI: Bind the image that contains the shading rate patterns
		if (enableShadingRate) {
			vkCmdBindShadingRateImageNV(drawCmdBuffers[i], shadingRateImage.view, VK_IMAGE_LAYOUT_SHADING_RATE_OPTIMAL_NV);
		};

		// Render the scene
		Pipelines& pipelines = enableShadingRate ? shadingRatePipelines : basePipelines;
		vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.opaque);
		scene.draw(drawCmdBuffers[i], vkglTF::RenderFlags::BindImages | vkglTF::RenderFlags::RenderOpaqueNodes, pipelineLayout);
		vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.masked);
		scene.draw(drawCmdBuffers[i], vkglTF::RenderFlags::BindImages | vkglTF::RenderFlags::RenderAlphaMaskedNodes, pipelineLayout);

		drawUI(drawCmdBuffers[i]);
		vkCmdEndRenderPass(drawCmdBuffers[i]);
		VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
	}
}

void VulkanExample::loadAssets()
{
	vkglTF::descriptorBindingFlags = vkglTF::DescriptorBindingFlags::ImageBaseColor | vkglTF::DescriptorBindingFlags::ImageNormalMap;
	scene.loadFromFile(getAssetPath() + "models/sponza/sponza.gltf", vulkanDevice, queue, vkglTF::FileLoadingFlags::PreTransformVertices);
}

void VulkanExample::setupDescriptors()
{
	// Pool
	const std::vector<VkDescriptorPoolSize> poolSizes = {
		vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1),
	};
	VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, 1);
	VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));

	// Descriptor set layout
	const std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
		vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0),
	};
	VkDescriptorSetLayoutCreateInfo descriptorLayout = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayout));

	// Pipeline layout
	const std::vector<VkDescriptorSetLayout> setLayouts = {
		descriptorSetLayout,
		vkglTF::descriptorSetLayoutImage,
	};
	VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(setLayouts.data(), 2);
	VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &pipelineLayout));

	// Descriptor set
	VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);
	VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));
	std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
		vks::initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &shaderData.buffer.descriptor),
	};
	vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
}

// [POI]
void VulkanExample::prepareShadingRateImage()
{
	// Shading rate image size depends on shading rate texel size
	// For each texel in the target image, there is a corresponding shading texel size width x height block in the shading rate image
	VkExtent3D imageExtent{};
	imageExtent.width = static_cast<uint32_t>(ceil(width / (float)physicalDeviceShadingRateImagePropertiesNV.shadingRateTexelSize.width));
	imageExtent.height = static_cast<uint32_t>(ceil(height / (float)physicalDeviceShadingRateImagePropertiesNV.shadingRateTexelSize.height));
	imageExtent.depth = 1;

	VkImageCreateInfo imageCI{};
	imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCI.imageType = VK_IMAGE_TYPE_2D;
	imageCI.format = VK_FORMAT_R8_UINT;
	imageCI.extent = imageExtent;
	imageCI.mipLevels = 1;
	imageCI.arrayLayers = 1;
	imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCI.usage = VK_IMAGE_USAGE_SHADING_RATE_IMAGE_BIT_NV | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	VK_CHECK_RESULT(vkCreateImage(device, &imageCI, nullptr, &shadingRateImage.image));
	VkMemoryRequirements memReqs{};
	vkGetImageMemoryRequirements(device, shadingRateImage.image, &memReqs);
	
	VkDeviceSize bufferSize = imageExtent.width * imageExtent.height * sizeof(uint8_t);

	VkMemoryAllocateInfo memAllloc{};
	memAllloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAllloc.allocationSize = memReqs.size;
	memAllloc.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK_RESULT(vkAllocateMemory(device, &memAllloc, nullptr, &shadingRateImage.memory));
	VK_CHECK_RESULT(vkBindImageMemory(device, shadingRateImage.image, shadingRateImage.memory, 0));

	VkImageViewCreateInfo imageViewCI{};
	imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCI.image = shadingRateImage.image;
	imageViewCI.format = VK_FORMAT_R8_UINT;
	imageViewCI.subresourceRange.baseMipLevel = 0;
	imageViewCI.subresourceRange.levelCount = 1;
	imageViewCI.subresourceRange.baseArrayLayer = 0;
	imageViewCI.subresourceRange.layerCount = 1;
	imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	VK_CHECK_RESULT(vkCreateImageView(device, &imageViewCI, nullptr, &shadingRateImage.view));

	// Populate with lowest possible shading rate pattern
	uint8_t val = VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_4X4_PIXELS_NV;
	uint8_t* shadingRatePatternData = new uint8_t[bufferSize];
	memset(shadingRatePatternData, val, bufferSize);

	// Create a circular pattern with decreasing sampling rates outwards (max. range, pattern)
	std::map<float, VkShadingRatePaletteEntryNV> patternLookup = {
		{ 8.0f, VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_PIXEL_NV },
		{ 12.0f, VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_2X1_PIXELS_NV },
		{ 16.0f, VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_1X2_PIXELS_NV },
		{ 18.0f, VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_2X2_PIXELS_NV },
		{ 20.0f, VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_4X2_PIXELS_NV },
		{ 24.0f, VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_2X4_PIXELS_NV }
	};

	uint8_t* ptrData = shadingRatePatternData;
	for (uint32_t y = 0; y < imageExtent.height; y++) {
		for (uint32_t x = 0; x < imageExtent.width; x++) {
			const float deltaX = (float)imageExtent.width / 2.0f - (float)x;
			const float deltaY = ((float)imageExtent.height / 2.0f - (float)y) * ((float)width / (float)height);
			const float dist = std::sqrt(deltaX * deltaX + deltaY * deltaY);
			for (auto pattern : patternLookup) {
				if (dist < pattern.first) {
					*ptrData = pattern.second;
					break;
				}
			}
			ptrData++;
		}
	}

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingMemory;

	VkBufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = bufferSize;
	bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VK_CHECK_RESULT(vkCreateBuffer(device, &bufferCreateInfo, nullptr, &stagingBuffer));
	VkMemoryAllocateInfo memAllocInfo{};
	memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memReqs = {};
	vkGetBufferMemoryRequirements(device, stagingBuffer, &memReqs);
	memAllocInfo.allocationSize = memReqs.size;
	memAllocInfo.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	VK_CHECK_RESULT(vkAllocateMemory(device, &memAllocInfo, nullptr, &stagingMemory));
	VK_CHECK_RESULT(vkBindBufferMemory(device, stagingBuffer, stagingMemory, 0));

	uint8_t* mapped;
	VK_CHECK_RESULT(vkMapMemory(device, stagingMemory, 0, memReqs.size, 0, (void**)&mapped));
	memcpy(mapped, shadingRatePatternData, bufferSize);
	vkUnmapMemory(device, stagingMemory);

	delete[] shadingRatePatternData;

	// Upload
	VkCommandBuffer copyCmd = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.levelCount = 1;
	subresourceRange.layerCount = 1;
	{
		VkImageMemoryBarrier imageMemoryBarrier{};
		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageMemoryBarrier.srcAccessMask = 0;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.image = shadingRateImage.image;
		imageMemoryBarrier.subresourceRange = subresourceRange;
		vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
	}
	VkBufferImageCopy bufferCopyRegion{};
	bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	bufferCopyRegion.imageSubresource.layerCount = 1;
	bufferCopyRegion.imageExtent.width = imageExtent.width;
	bufferCopyRegion.imageExtent.height = imageExtent.height;
	bufferCopyRegion.imageExtent.depth = 1;
	vkCmdCopyBufferToImage(copyCmd, stagingBuffer, shadingRateImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferCopyRegion);
	{
		VkImageMemoryBarrier imageMemoryBarrier{};
		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADING_RATE_OPTIMAL_NV;
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = 0;
		imageMemoryBarrier.image = shadingRateImage.image;
		imageMemoryBarrier.subresourceRange = subresourceRange;
		vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
	}
	vulkanDevice->flushCommandBuffer(copyCmd, queue, true);

	vkFreeMemory(device, stagingMemory, nullptr);
	vkDestroyBuffer(device, stagingBuffer, nullptr);
}

void VulkanExample::preparePipelines()
{
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
	VkPipelineRasterizationStateCreateInfo rasterizationStateCI = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
	VkPipelineColorBlendAttachmentState blendAttachmentStateCI = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
	VkPipelineColorBlendStateCreateInfo colorBlendStateCI = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentStateCI);
	VkPipelineDepthStencilStateCreateInfo depthStencilStateCI = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
	VkPipelineViewportStateCreateInfo viewportStateCI = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
	VkPipelineMultisampleStateCreateInfo multisampleStateCI = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
	const std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicStateCI = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables.data(), static_cast<uint32_t>(dynamicStateEnables.size()), 0);
	std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

	VkGraphicsPipelineCreateInfo pipelineCI = vks::initializers::pipelineCreateInfo(pipelineLayout, renderPass, 0);
	pipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
	pipelineCI.pRasterizationState = &rasterizationStateCI;
	pipelineCI.pColorBlendState = &colorBlendStateCI;
	pipelineCI.pMultisampleState = &multisampleStateCI;
	pipelineCI.pViewportState = &viewportStateCI;
	pipelineCI.pDepthStencilState = &depthStencilStateCI;
	pipelineCI.pDynamicState = &dynamicStateCI;
	pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
	pipelineCI.pStages = shaderStages.data();
	pipelineCI.pVertexInputState = vkglTF::Vertex::getPipelineVertexInputState({ vkglTF::VertexComponent::Position, vkglTF::VertexComponent::Normal, vkglTF::VertexComponent::UV, vkglTF::VertexComponent::Color, vkglTF::VertexComponent::Tangent });

	shaderStages[0] = loadShader(getShadersPath() + "variablerateshading/scene.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = loadShader(getShadersPath() + "variablerateshading/scene.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

	// Properties for alpha masked materials will be passed via specialization constants
	struct SpecializationData {
		bool alphaMask;
		float alphaMaskCutoff;
	} specializationData;
	specializationData.alphaMask = false;
	specializationData.alphaMaskCutoff = 0.5f;
	const std::vector<VkSpecializationMapEntry> specializationMapEntries = {
		vks::initializers::specializationMapEntry(0, offsetof(SpecializationData, alphaMask), sizeof(SpecializationData::alphaMask)),
		vks::initializers::specializationMapEntry(1, offsetof(SpecializationData, alphaMaskCutoff), sizeof(SpecializationData::alphaMaskCutoff)),
	};
	VkSpecializationInfo specializationInfo = vks::initializers::specializationInfo(specializationMapEntries, sizeof(specializationData), &specializationData);
	shaderStages[1].pSpecializationInfo = &specializationInfo;

	// Create pipeline without shading rate 
	VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &basePipelines.opaque));
	specializationData.alphaMask = true;
	rasterizationStateCI.cullMode = VK_CULL_MODE_NONE;
	VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &basePipelines.masked));
	rasterizationStateCI.cullMode = VK_CULL_MODE_BACK_BIT;
	specializationData.alphaMask = false;

	// Create pipeline with shading rate enabled
	// [POI] Possible per-Viewport shading rate palette entries
	const std::vector<VkShadingRatePaletteEntryNV> shadingRatePaletteEntries = {
		VK_SHADING_RATE_PALETTE_ENTRY_NO_INVOCATIONS_NV,
		VK_SHADING_RATE_PALETTE_ENTRY_16_INVOCATIONS_PER_PIXEL_NV,
		VK_SHADING_RATE_PALETTE_ENTRY_8_INVOCATIONS_PER_PIXEL_NV,
		VK_SHADING_RATE_PALETTE_ENTRY_4_INVOCATIONS_PER_PIXEL_NV,
		VK_SHADING_RATE_PALETTE_ENTRY_2_INVOCATIONS_PER_PIXEL_NV,
		VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_PIXEL_NV,
		VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_2X1_PIXELS_NV,
		VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_1X2_PIXELS_NV,
		VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_2X2_PIXELS_NV,
		VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_4X2_PIXELS_NV,
		VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_2X4_PIXELS_NV,
		VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_4X4_PIXELS_NV,
	};
	VkShadingRatePaletteNV shadingRatePalette{};
	shadingRatePalette.shadingRatePaletteEntryCount = static_cast<uint32_t>(shadingRatePaletteEntries.size());
	shadingRatePalette.pShadingRatePaletteEntries = shadingRatePaletteEntries.data();
	VkPipelineViewportShadingRateImageStateCreateInfoNV pipelineViewportShadingRateImageStateCI{};
	pipelineViewportShadingRateImageStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_SHADING_RATE_IMAGE_STATE_CREATE_INFO_NV;
	pipelineViewportShadingRateImageStateCI.shadingRateImageEnable = VK_TRUE;
	pipelineViewportShadingRateImageStateCI.viewportCount = 1;
	pipelineViewportShadingRateImageStateCI.pShadingRatePalettes = &shadingRatePalette;
	viewportStateCI.pNext = &pipelineViewportShadingRateImageStateCI;
	VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &shadingRatePipelines.opaque));
	specializationData.alphaMask = true;
	rasterizationStateCI.cullMode = VK_CULL_MODE_NONE;
	VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &shadingRatePipelines.masked));
}

void VulkanExample::prepareUniformBuffers()
{
	VK_CHECK_RESULT(vulkanDevice->createBuffer(
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&shaderData.buffer,
		sizeof(shaderData.values)));
	VK_CHECK_RESULT(shaderData.buffer.map());
	updateUniformBuffers();
}

void VulkanExample::updateUniformBuffers()
{
	shaderData.values.projection = camera.matrices.perspective;
	shaderData.values.view = camera.matrices.view;
	shaderData.values.viewPos = camera.viewPos;
	shaderData.values.colorShadingRate = colorShadingRate;
	memcpy(shaderData.buffer.mapped, &shaderData.values, sizeof(shaderData.values));
}

void VulkanExample::prepare()
{
	VulkanExampleBase::prepare();
	loadAssets();
	
	// [POI]
	vkCmdBindShadingRateImageNV = reinterpret_cast<PFN_vkCmdBindShadingRateImageNV>(vkGetDeviceProcAddr(device, "vkCmdBindShadingRateImageNV"));
	physicalDeviceShadingRateImagePropertiesNV.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADING_RATE_IMAGE_PROPERTIES_NV;
	VkPhysicalDeviceProperties2 deviceProperties2{};
	deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	deviceProperties2.pNext = &physicalDeviceShadingRateImagePropertiesNV;
	vkGetPhysicalDeviceProperties2(physicalDevice, &deviceProperties2);

	prepareShadingRateImage();
	prepareUniformBuffers();
	setupDescriptors();
	preparePipelines();
	buildCommandBuffers();
	prepared = true;
}

void VulkanExample::render()
{
	renderFrame();
	if (camera.updated) {
		updateUniformBuffers();
	}
}

void VulkanExample::OnUpdateUIOverlay(vks::UIOverlay* overlay)
{
	if (overlay->checkBox("Enable shading rate", &enableShadingRate)) {
		buildCommandBuffers();
	}
	if (overlay->checkBox("Color shading rates", &colorShadingRate)) {
		updateUniformBuffers();
	}
}

VULKAN_EXAMPLE_MAIN()
