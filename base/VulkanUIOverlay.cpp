
/*
* UI overlay class using ImGui
*
* Copyright (C) 2017-2025 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "VulkanUIOverlay.h"

namespace vks 
{
	UIOverlay::UIOverlay()
	{
#if defined(__ANDROID__)		
		if (vks::android::screenDensity >= ACONFIGURATION_DENSITY_XXHIGH) {
			scale = 3.5f;
		}
		else if (vks::android::screenDensity >= ACONFIGURATION_DENSITY_XHIGH) {
			scale = 2.5f;
		}
		else if (vks::android::screenDensity >= ACONFIGURATION_DENSITY_HIGH) {
			scale = 2.0f;
		};
#endif
		// Init ImGui
		ImGui::CreateContext();
		// Color scheme
		ImGuiStyle& style = ImGui::GetStyle();
		style.Colors[ImGuiCol_TitleBg] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
		style.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
		style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.0f, 0.0f, 0.0f, 0.1f);
		style.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
		style.Colors[ImGuiCol_Header] = ImVec4(0.8f, 0.0f, 0.0f, 0.4f);
		style.Colors[ImGuiCol_HeaderActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
		style.Colors[ImGuiCol_HeaderHovered] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
		style.Colors[ImGuiCol_FrameBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.8f);
		style.Colors[ImGuiCol_CheckMark] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
		style.Colors[ImGuiCol_SliderGrab] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
		style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
		style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(1.0f, 1.0f, 1.0f, 0.1f);
		style.Colors[ImGuiCol_FrameBgActive] = ImVec4(1.0f, 1.0f, 1.0f, 0.2f);
		style.Colors[ImGuiCol_Button] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
		style.Colors[ImGuiCol_ButtonHovered] = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
		style.Colors[ImGuiCol_ButtonActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
		// Dimensions
		ImGuiIO& io = ImGui::GetIO();
		io.FontGlobalScale = scale;
	}

	UIOverlay::~UIOverlay()	{
		if (ImGui::GetCurrentContext()) {
			ImGui::DestroyContext();
		}
	}

	/** Prepare all vulkan resources required to render the UI overlay */
	void UIOverlay::prepareResources()
	{
		assert(maxConcurrentFrames > 0);

		ImGuiIO& io = ImGui::GetIO();

		// Create font texture
		unsigned char* fontData;
		int texWidth, texHeight;
#if defined(__ANDROID__)
		float scale = (float)vks::android::screenDensity / (float)ACONFIGURATION_DENSITY_MEDIUM;
		AAsset* asset = AAssetManager_open(androidApp->activity->assetManager, "Roboto-Medium.ttf", AASSET_MODE_STREAMING);
		if (asset) {
			size_t size = AAsset_getLength(asset);
			assert(size > 0);
			char *fontAsset = new char[size];
			AAsset_read(asset, fontAsset, size);
			AAsset_close(asset);
			io.Fonts->AddFontFromMemoryTTF(fontAsset, size, 14.0f * scale);
			delete[] fontAsset;
		}
#else
		const std::string filename = getAssetPath() + "Roboto-Medium.ttf";
		io.Fonts->AddFontFromFileTTF(filename.c_str(), 16.0f * scale);
#endif
		io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);
		VkDeviceSize uploadSize = texWidth*texHeight * 4 * sizeof(char);

		// Set ImGui style scale factor to handle retina and other HiDPI displays (same as font scaling above)
		ImGuiStyle& style = ImGui::GetStyle();
		style.ScaleAllSizes(scale);

		// Create target image for copy
		VkImageCreateInfo imageInfo{
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = VK_FORMAT_R8G8B8A8_UNORM,
			.extent = {.width = (uint32_t)texWidth, .height = (uint32_t)texHeight, .depth = 1 },
			.mipLevels = 1,
			.arrayLayers = 1,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = VK_IMAGE_TILING_OPTIMAL,
			.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
		};
		VK_CHECK_RESULT(vkCreateImage(device->logicalDevice, &imageInfo, nullptr, &fontImage));
		VkMemoryRequirements memReqs;
		vkGetImageMemoryRequirements(device->logicalDevice, fontImage, &memReqs);
		VkMemoryAllocateInfo memAllocInfo{
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = memReqs.size,
			.memoryTypeIndex = device->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
		};
		VK_CHECK_RESULT(vkAllocateMemory(device->logicalDevice, &memAllocInfo, nullptr, &fontMemory));
		VK_CHECK_RESULT(vkBindImageMemory(device->logicalDevice, fontImage, fontMemory, 0));

		// Image view
		VkImageViewCreateInfo viewInfo{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = fontImage,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = VK_FORMAT_R8G8B8A8_UNORM,
			.subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 }
		};
		VK_CHECK_RESULT(vkCreateImageView(device->logicalDevice, &viewInfo, nullptr, &fontView));

		// Staging buffers for font data upload
		vks::Buffer stagingBuffer;
		VK_CHECK_RESULT(device->createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, uploadSize));
		stagingBuffer.map();
		memcpy(stagingBuffer.mapped, fontData, uploadSize);

		// Copy buffer data to font image
		VkCommandBuffer copyCmd = device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
		// Prepare for transfer
		vks::tools::setImageLayout(
			copyCmd,
			fontImage,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_PIPELINE_STAGE_HOST_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT);
		// Copy
		VkBufferImageCopy bufferCopyRegion{
			.imageSubresource = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .layerCount = 1 },
			.imageExtent = {.width = (uint32_t)texWidth, .height = (uint32_t)texHeight, .depth = 1 }
		};
		vkCmdCopyBufferToImage(copyCmd, stagingBuffer.buffer, fontImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferCopyRegion);
		// Prepare for shader read
		vks::tools::setImageLayout(
			copyCmd,
			fontImage,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		device->flushCommandBuffer(copyCmd, queue, true);

		stagingBuffer.destroy();

		// Font texture Sampler
		VkSamplerCreateInfo samplerInfo{
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.magFilter = VK_FILTER_LINEAR,
			.minFilter = VK_FILTER_LINEAR,
			.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
			.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.maxAnisotropy = 1.0f,
			.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
		};
		VK_CHECK_RESULT(vkCreateSampler(device->logicalDevice, &samplerInfo, nullptr, &sampler));

		// Descriptor pool
		VkDescriptorPoolSize poolSize{ .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = 1 };
		VkDescriptorPoolCreateInfo descriptorPoolInfo{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, .maxSets = 2, .poolSizeCount = 1, .pPoolSizes = &poolSize };
		VK_CHECK_RESULT(vkCreateDescriptorPool(device->logicalDevice, &descriptorPoolInfo, nullptr, &descriptorPool));

		// Descriptor set layout
		VkDescriptorSetLayoutBinding setLayoutBinding = vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0);
		VkDescriptorSetLayoutCreateInfo descriptorLayout{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, .bindingCount = 1, .pBindings = &setLayoutBinding };
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device->logicalDevice, &descriptorLayout, nullptr, &descriptorSetLayout));

		// Descriptor set
		VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device->logicalDevice, &allocInfo, &descriptorSet));
		VkDescriptorImageInfo fontDescriptor{ .sampler = sampler, .imageView = fontView, .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
		VkWriteDescriptorSet writeDescriptorSets = vks::initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &fontDescriptor);
		vkUpdateDescriptorSets(device->logicalDevice, 1, &writeDescriptorSets, 0, nullptr);

		// Buffers per max. frames-in-flight
		buffers.resize(maxConcurrentFrames);
	}

	/** Prepare a separate pipeline for the UI overlay rendering decoupled from the main application */
	void UIOverlay::preparePipeline(const VkPipelineCache pipelineCache, const VkRenderPass renderPass, const VkFormat colorFormat, const VkFormat depthFormat)
	{
		// Pipeline layout
		// Push constants for UI rendering parameters
		VkPushConstantRange pushConstantRange{ .stageFlags = VK_SHADER_STAGE_VERTEX_BIT, .size = sizeof(PushConstBlock) };
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = 1,
			.pSetLayouts = &descriptorSetLayout,
			.pushConstantRangeCount = 1,
			.pPushConstantRanges = &pushConstantRange
		};
		VK_CHECK_RESULT(vkCreatePipelineLayout(device->logicalDevice, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

		// Setup graphics pipeline for UI rendering
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
		VkPipelineRasterizationStateCreateInfo rasterizationState = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);

		// Enable blending
		VkPipelineColorBlendAttachmentState blendAttachmentState{
			.blendEnable = VK_TRUE,
			.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
			.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			.colorBlendOp = VK_BLEND_OP_ADD,
			.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
			.alphaBlendOp = VK_BLEND_OP_ADD,
			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
		};

		// Vertex bindings an attributes based on ImGui vertex definition
		std::vector<VkVertexInputBindingDescription> vertexInputBindings = {
			{ .binding = 0, .stride = sizeof(ImDrawVert), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX }
		};
		std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = {
			{ .location = 0, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(ImDrawVert, pos) },
			{ .location = 1, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(ImDrawVert, uv) },
			{ .location = 2, .binding = 0, .format = VK_FORMAT_R8G8B8A8_UNORM, .offset = offsetof(ImDrawVert, col) },
		};
		VkPipelineVertexInputStateCreateInfo vertexInputState = vks::initializers::pipelineVertexInputStateCreateInfo();
		vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindings.size());
		vertexInputState.pVertexBindingDescriptions = vertexInputBindings.data();
		vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
		vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();

		VkPipelineColorBlendStateCreateInfo colorBlendState = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
		VkPipelineDepthStencilStateCreateInfo depthStencilState = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_FALSE, VK_FALSE, VK_COMPARE_OP_ALWAYS);
		VkPipelineViewportStateCreateInfo viewportState = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
		VkPipelineMultisampleStateCreateInfo multisampleState = vks::initializers::pipelineMultisampleStateCreateInfo(rasterizationSamples);
		std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicState = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
		VkGraphicsPipelineCreateInfo pipelineCreateInfo{
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.stageCount = static_cast<uint32_t>(shaders.size()),
			.pStages = shaders.data(),
			.pVertexInputState = &vertexInputState,
			.pInputAssemblyState = &inputAssemblyState,
			.pViewportState = &viewportState,
			.pRasterizationState = &rasterizationState,
			.pMultisampleState = &multisampleState,
			.pDepthStencilState = &depthStencilState,
			.pColorBlendState = &colorBlendState,
			.pDynamicState = &dynamicState,
			.layout = pipelineLayout,
			.renderPass = renderPass,
			.subpass = subpass,
		};
#if defined(VK_KHR_dynamic_rendering)
		// If we are using dynamic rendering (renderPass is null), we must define color, depth and stencil attachments at pipeline create time
		VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo{};
		if (renderPass == VK_NULL_HANDLE) {
			pipelineRenderingCreateInfo = {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
				.colorAttachmentCount = 1,
				.pColorAttachmentFormats = &colorFormat,
				.depthAttachmentFormat = depthFormat,
				.stencilAttachmentFormat = depthFormat
			};
			pipelineCreateInfo.pNext = &pipelineRenderingCreateInfo;
		}
#endif
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device->logicalDevice, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipeline));
	}

	/** Update vertex and index buffer containing the imGui elements when required */
	void UIOverlay::update(uint32_t currentBuffer)
	{	
		ImDrawData* imDrawData = ImGui::GetDrawData();

		if (!imDrawData) {
			return;
		}

		// Note: Alignment is done inside buffer creation
		VkDeviceSize vertexBufferSize = imDrawData->TotalVtxCount * sizeof(ImDrawVert);
		VkDeviceSize indexBufferSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);
		
		// Update buffers only if vertex or index count has been changed compared to current buffer size
		if ((vertexBufferSize == 0) || (indexBufferSize == 0)) {
			return;
		}

		// Create buffers with multiple of a chunk size to minimize the need to recreate them
		const VkDeviceSize chunkSize = 16384;
		vertexBufferSize = ((vertexBufferSize + chunkSize - 1) / chunkSize) * chunkSize;
		indexBufferSize = ((indexBufferSize + chunkSize - 1) / chunkSize) * chunkSize;

		// Recreate vertex buffer only if necessary
		if ((buffers[currentBuffer].vertexBuffer.buffer == VK_NULL_HANDLE) || (buffers[currentBuffer].vertexBuffer.size < vertexBufferSize)) {
			buffers[currentBuffer].vertexBuffer.unmap();
			buffers[currentBuffer].vertexBuffer.destroy();
			VK_CHECK_RESULT(device->createBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &buffers[currentBuffer].vertexBuffer, vertexBufferSize));
			buffers[currentBuffer].vertexCount = imDrawData->TotalVtxCount;
			buffers[currentBuffer].vertexBuffer.map();
		}

		// Recreate index buffer only if necessary
		if ((buffers[currentBuffer].indexBuffer.buffer == VK_NULL_HANDLE) || (buffers[currentBuffer].indexBuffer.size < indexBufferSize)) {
			buffers[currentBuffer].indexBuffer.unmap();
			buffers[currentBuffer].indexBuffer.destroy();
			VK_CHECK_RESULT(device->createBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &buffers[currentBuffer].indexBuffer, indexBufferSize));
			buffers[currentBuffer].indexCount = imDrawData->TotalIdxCount;
			buffers[currentBuffer].indexBuffer.map();
		}

		// Upload data
		ImDrawVert* vtxDst = (ImDrawVert*)buffers[currentBuffer].vertexBuffer.mapped;
		ImDrawIdx* idxDst = (ImDrawIdx*)buffers[currentBuffer].indexBuffer.mapped;

		for (int n = 0; n < imDrawData->CmdListsCount; n++) {
			const ImDrawList* cmd_list = imDrawData->CmdLists[n];
			memcpy(vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
			memcpy(idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
			vtxDst += cmd_list->VtxBuffer.Size;
			idxDst += cmd_list->IdxBuffer.Size;
		}

		// Flush to make writes visible to GPU
		buffers[currentBuffer].vertexBuffer.flush();
		buffers[currentBuffer].indexBuffer.flush();
	}

	void UIOverlay::draw(const VkCommandBuffer commandBuffer, uint32_t currentBuffer)
	{
		ImDrawData* imDrawData = ImGui::GetDrawData();
		int32_t vertexOffset = 0;
		int32_t indexOffset = 0;

		if ((!imDrawData) || (imDrawData->CmdListsCount == 0)) {
			return;
		}

		if (buffers[currentBuffer].vertexBuffer.buffer == VK_NULL_HANDLE || buffers[currentBuffer].indexBuffer.buffer == VK_NULL_HANDLE) {
			return;
		}

		ImGuiIO& io = ImGui::GetIO();
		
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);

		pushConstBlock.scale = glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
		pushConstBlock.translate = glm::vec2(-1.0f);
		vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstBlock), &pushConstBlock);

		assert(buffers[currentBuffer].vertexBuffer.buffer != VK_NULL_HANDLE && buffers[currentBuffer].indexBuffer.buffer != VK_NULL_HANDLE);

		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &buffers[currentBuffer].vertexBuffer.buffer, offsets);
		vkCmdBindIndexBuffer(commandBuffer, buffers[currentBuffer].indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);

		for (int32_t i = 0; i < imDrawData->CmdListsCount; i++) {
			const ImDrawList* cmd_list = imDrawData->CmdLists[i];
			for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++) {
				const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[j];
				VkRect2D scissorRect{
					.offset = {.x = std::max((int32_t)(pcmd->ClipRect.x), 0), .y = std::max((int32_t)(pcmd->ClipRect.y), 0) },
					.extent = {.width = (uint32_t)(pcmd->ClipRect.z - pcmd->ClipRect.x), .height = (uint32_t)(pcmd->ClipRect.w - pcmd->ClipRect.y) }
				};
				vkCmdSetScissor(commandBuffer, 0, 1, &scissorRect);
				vkCmdDrawIndexed(commandBuffer, pcmd->ElemCount, 1, indexOffset, vertexOffset, 0);
				indexOffset += pcmd->ElemCount;
			}
#if (defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_METAL_EXT)) && TARGET_OS_SIMULATOR
			// Apple Device Simulator does not support vkCmdDrawIndexed() with vertexOffset > 0, so rebind vertex buffer instead
			offsets[0] += cmd_list->VtxBuffer.Size * sizeof(ImDrawVert);
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &buffers[currentBuffer].vertexBuffer.buffer, offsets);
#else
			vertexOffset += cmd_list->VtxBuffer.Size;
#endif
		}
	}

	void UIOverlay::resize(uint32_t width, uint32_t height)
	{
		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize = ImVec2((float)(width), (float)(height));
	}

	void UIOverlay::freeResources()
	{
		for (auto& buffer : buffers) {
			buffer.vertexBuffer.destroy();
			buffer.indexBuffer.destroy();
		}
		vkDestroyImageView(device->logicalDevice, fontView, nullptr);
		vkDestroyImage(device->logicalDevice, fontImage, nullptr);
		vkFreeMemory(device->logicalDevice, fontMemory, nullptr);
		vkDestroySampler(device->logicalDevice, sampler, nullptr);
		vkDestroyDescriptorSetLayout(device->logicalDevice, descriptorSetLayout, nullptr);
		vkDestroyDescriptorPool(device->logicalDevice, descriptorPool, nullptr);
		vkDestroyPipelineLayout(device->logicalDevice, pipelineLayout, nullptr);
		vkDestroyPipeline(device->logicalDevice, pipeline, nullptr);
	}

	bool UIOverlay::header(const char *caption)
	{
		return ImGui::CollapsingHeader(caption, ImGuiTreeNodeFlags_DefaultOpen);
	}

	bool UIOverlay::checkBox(const char *caption, bool *value)
	{
		return ImGui::Checkbox(caption, value);
	}

	bool UIOverlay::checkBox(const char *caption, int32_t *value)
	{
		bool val = (*value == 1);
		bool res = ImGui::Checkbox(caption, &val);
		*value = val;
		return res;
	}

	bool UIOverlay::radioButton(const char* caption, bool value)
	{
		return ImGui::RadioButton(caption, value);
	}

	bool UIOverlay::inputFloat(const char *caption, float *value, float step, uint32_t precision)
	{
		return ImGui::InputFloat(caption, value, step, step * 10.0f, precision);
	}

	bool UIOverlay::sliderFloat(const char* caption, float* value, float min, float max)
	{
		return ImGui::SliderFloat(caption, value, min, max);
	}

	bool UIOverlay::sliderInt(const char* caption, int32_t* value, int32_t min, int32_t max)
	{
		return ImGui::SliderInt(caption, value, min, max);
	}

	bool UIOverlay::comboBox(const char *caption, int32_t *itemindex, std::vector<std::string> items)
	{
		if (items.empty()) {
			return false;
		}
		std::vector<const char*> charitems;
		charitems.reserve(items.size());
		for (size_t i = 0; i < items.size(); i++) {
			charitems.push_back(items[i].c_str());
		}
		uint32_t itemCount = static_cast<uint32_t>(charitems.size());
		return ImGui::Combo(caption, itemindex, &charitems[0], itemCount, itemCount);
	}

	bool UIOverlay::button(const char *caption)
	{
		return ImGui::Button(caption);
	}

	bool UIOverlay::colorPicker(const char* caption, float* color) {
		return ImGui::ColorEdit4(caption, color, ImGuiColorEditFlags_NoInputs);
	}

	void UIOverlay::text(const char *formatstr, ...)
	{
		va_list args;
		va_start(args, formatstr);
		ImGui::TextV(formatstr, args);
		va_end(args);
	}
}
