/*
* UI overlay class using ImGui
*
* Copyright (C) 2017 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <vector>
#include <sstream>
#include <iomanip>

#include <vulkan/vulkan.h>
#include "VulkanTools.h"
#include "VulkanDebug.h"
#include "VulkanBuffer.hpp"
#include "VulkanDevice.hpp"

#include "../external/imgui/imgui.h"

#if defined(__ANDROID__)
#include "VulkanAndroid.h"
#endif

namespace vks 
{
	struct UIOverlayCreateInfo 
	{
		vks::VulkanDevice *device;
		VkQueue copyQueue;
		VkRenderPass renderPass;
		std::vector<VkFramebuffer> framebuffers;
		VkFormat colorformat;
		VkFormat depthformat;
		uint32_t width;
		uint32_t height;
		std::vector<VkPipelineShaderStageCreateInfo> shaders;
		VkSampleCountFlagBits rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		uint32_t subpassCount = 1;
		std::vector<VkClearValue> clearValues = {};
		uint32_t attachmentCount = 1;
	};

	class UIOverlay 
	{
	private:
		vks::Buffer vertexBuffer;
		vks::Buffer indexBuffer;
		int32_t vertexCount = 0;
		int32_t indexCount = 0;

		VkDescriptorPool descriptorPool;
		VkDescriptorSetLayout descriptorSetLayout;
		VkDescriptorSet descriptorSet;
		VkPipelineLayout pipelineLayout;
		VkPipelineCache pipelineCache;
		VkPipeline pipeline;
		VkRenderPass renderPass;
		VkCommandPool commandPool;
		VkFence fence;

		VkDeviceMemory fontMemory = VK_NULL_HANDLE;
		VkImage fontImage = VK_NULL_HANDLE;
		VkImageView fontView = VK_NULL_HANDLE;
		VkSampler sampler;

		struct PushConstBlock {
			glm::vec2 scale;
			glm::vec2 translate;
		} pushConstBlock;

		UIOverlayCreateInfo createInfo = {};

		void prepareResources();
		void preparePipeline();
		void prepareRenderPass();
		void updateCommandBuffers();
	public:
		bool visible = true;
		float scale = 1.0f;

		std::vector<VkCommandBuffer> cmdBuffers;

		UIOverlay(vks::UIOverlayCreateInfo createInfo);
		~UIOverlay();

		void update();
		void resize(uint32_t width, uint32_t height, std::vector<VkFramebuffer> framebuffers);

		void submit(VkQueue queue, uint32_t bufferindex, VkSubmitInfo submitInfo);

		bool header(const char* caption);
		bool checkBox(const char* caption, bool* value);
		bool checkBox(const char* caption, int32_t* value);
		bool inputFloat(const char* caption, float* value, float step, uint32_t precision);
		bool sliderFloat(const char* caption, float* value, float min, float max);
		bool sliderInt(const char* caption, int32_t* value, int32_t min, int32_t max);
		bool comboBox(const char* caption, int32_t* itemindex, std::vector<std::string> items);
		bool button(const char* caption);
		void text(const char* formatstr, ...);
	};
}