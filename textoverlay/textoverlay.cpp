/*
* Vulkan Example - Text overlay rendering on-top of an existing scene using a separate render pass
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <vector>
#include <sstream>
#include <iomanip>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include <vulkan/vulkan.h>
#include "vulkanexamplebase.h"
#include "../external/stb/stb_font_consolas_24_latin1.inl"

#define VERTEX_BUFFER_BIND_ID 0
#define ENABLE_VALIDATION false

// Vertex layout for this example
std::vector<vkMeshLoader::VertexLayout> vertexLayout =
{
	vkMeshLoader::VERTEX_LAYOUT_POSITION,
	vkMeshLoader::VERTEX_LAYOUT_NORMAL,
	vkMeshLoader::VERTEX_LAYOUT_UV,
	vkMeshLoader::VERTEX_LAYOUT_COLOR,
};


// Defines for the STB font used
// STB font files can be found at http://nothings.org/stb/font/
#define STB_FONT_NAME stb_font_consolas_24_latin1
#define STB_FONT_WIDTH STB_FONT_consolas_24_latin1_BITMAP_WIDTH
#define STB_FONT_HEIGHT STB_FONT_consolas_24_latin1_BITMAP_HEIGHT 
#define STB_FIRST_CHAR STB_FONT_consolas_24_latin1_FIRST_CHAR
#define STB_NUM_CHARS STB_FONT_consolas_24_latin1_NUM_CHARS

// Max. number of chars the text overlay buffer can hold
#define MAX_CHAR_COUNT 1024

// Mostly self-contained text overlay class
// todo : comment
class VulkanTextOverlay
{
private:
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
	VkFormat colorFormat;
	VkFormat depthFormat;
	VkExtent2D windowSize;

	VkSampler sampler;
	VkImage image;
	VkImageView view;
	VkBuffer buffer;
	//		VkDeviceMemory memory;
	VkDeviceMemory imageMemory;
	VkDescriptorPool descriptorPool;
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSet descriptorSet;
	VkPipelineLayout pipelineLayout;
	VkPipelineCache pipelineCache;
	VkPipeline pipeline;
	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> cmdBuffers;
	std::vector<VkFramebuffer> frameBuffers;

	VkRenderPass renderPass;

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

	// todo : duplicate code, android
	VkPipelineShaderStageCreateInfo loadShader(std::string fileName, VkShaderStageFlagBits stage)
	{
		VkPipelineShaderStageCreateInfo shaderStage = {};
		shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStage.stage = stage;
#if defined(__ANDROID__)
		shaderStage.module = vkTools::loadShader(androidApp->activity->assetManager, fileName.c_str(), device, stage);
#else
		shaderStage.module = vkTools::loadShader(fileName.c_str(), device, stage);
#endif
		shaderStage.pName = "main";
		assert(shaderStage.module != NULL);
		//shaderModules.push_back(shaderStage.module);
		return shaderStage;
	}

public:

	// todo : move to private!
	stb_fontchar stbFontData[STB_NUM_CHARS];
	uint32_t numLetters;
	VkDeviceMemory memory;

	// Pointer to mapped vertex buffer
	glm::vec4 *mapped = nullptr;

	VulkanTextOverlay::VulkanTextOverlay(
		VkPhysicalDevice physicalDevice,
		VkDevice device,
		std::vector<VkFramebuffer> framebuffers,
		VkFormat colorformat,
		VkFormat depthformat,
		VkExtent2D windowsize)
	{
		this->physicalDevice = physicalDevice;
		this->device = device;
		this->colorFormat = colorformat;
		this->depthFormat = depthformat;
		this->windowSize = windowsize;
		this->frameBuffers = framebuffers;

		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &deviceMemoryProperties);
		cmdBuffers.resize(framebuffers.size());
		prepareResources();
		prepareRenderPass();
		preparePipeline();
	}

	// Prepare all vulkan resources required to render the font
	// The text overlay uses separate resources for descriptors (pool, sets, layouts), pipelines and command buffers
	void prepareResources()
	{
		static unsigned char font24pixels[STB_FONT_HEIGHT][STB_FONT_WIDTH];
		STB_FONT_NAME(stbFontData, font24pixels, STB_FONT_HEIGHT);

		// Vertex buffer

		VkDeviceSize bufferSize = MAX_CHAR_COUNT * sizeof(glm::vec4);

		VkBufferCreateInfo bufferInfo = vkTools::initializers::bufferCreateInfo(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, bufferSize);
		vkTools::checkResult(vkCreateBuffer(device, &bufferInfo, nullptr, &buffer));

		VkMemoryRequirements memReqs;
		VkMemoryAllocateInfo allocInfo = vkTools::initializers::memoryAllocateInfo();

		vkGetBufferMemoryRequirements(device, buffer, &memReqs);
		allocInfo.allocationSize = memReqs.size;
		getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &allocInfo.memoryTypeIndex);

		vkTools::checkResult(vkAllocateMemory(device, &allocInfo, nullptr, &memory));
		vkTools::checkResult(vkBindBufferMemory(device, buffer, memory, 0));

		VkImageCreateInfo imageInfo = vkTools::initializers::imageCreateInfo();
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.format = VK_FORMAT_R8_UNORM;
		imageInfo.extent.width = STB_FONT_WIDTH;
		imageInfo.extent.height = STB_FONT_HEIGHT;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.tiling = VK_IMAGE_TILING_LINEAR;
		imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;

		vkTools::checkResult(vkCreateImage(device, &imageInfo, nullptr, &image));

		allocInfo.allocationSize = STB_FONT_WIDTH * STB_NUM_CHARS;

		vkTools::checkResult(vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory));
		vkTools::checkResult(vkBindImageMemory(device, image, imageMemory, 0));

		// todo : staging

		VkImageViewCreateInfo imageViewInfo = vkTools::initializers::imageViewCreateInfo();
		imageViewInfo.image = image;
		imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewInfo.format = imageInfo.format;
		imageViewInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B,	VK_COMPONENT_SWIZZLE_A };
		imageViewInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

		vkTools::checkResult(vkCreateImageView(device, &imageViewInfo, nullptr, &view));

		uint8_t *data;
		vkTools::checkResult(vkMapMemory(device, imageMemory, 0, allocInfo.allocationSize, 0, (void **)&data));
		memcpy(data, &font24pixels[0][0], STB_FONT_WIDTH * STB_FONT_HEIGHT);
		vkUnmapMemory(device, imageMemory);

		// Sampler
		VkSamplerCreateInfo samplerInfo = vkTools::initializers::samplerCreateInfo();
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 1.0f;
		samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		vkTools::checkResult(vkCreateSampler(device, &samplerInfo, nullptr, &sampler));

		// Descriptor
		// Font uses a separate descriptor pool
		std::array<VkDescriptorPoolSize, 1> poolSizes;
		poolSizes[0] = vkTools::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1);

		VkDescriptorPoolCreateInfo descriptorPoolInfo =
			vkTools::initializers::descriptorPoolCreateInfo(
				poolSizes.size(),
				poolSizes.data(),
				1);

		vkTools::checkResult(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));

		// Descriptor set layout
		std::array<VkDescriptorSetLayoutBinding, 1> setLayoutBindings;
		setLayoutBindings[0] = vkTools::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0);

		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo =
			vkTools::initializers::descriptorSetLayoutCreateInfo(
				setLayoutBindings.data(),
				setLayoutBindings.size());

		vkTools::checkResult(vkCreateDescriptorSetLayout(device, &descriptorSetLayoutInfo, nullptr, &descriptorSetLayout));

		// Pipeline layout
		VkPipelineLayoutCreateInfo pipelineLayoutInfo =
			vkTools::initializers::pipelineLayoutCreateInfo(
				&descriptorSetLayout,
				1);

		vkTools::checkResult(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout));

		// Descriptor set
		VkDescriptorSetAllocateInfo descriptorSetAllocInfo =
			vkTools::initializers::descriptorSetAllocateInfo(
				descriptorPool,
				&descriptorSetLayout,
				1);

		vkTools::checkResult(vkAllocateDescriptorSets(device, &descriptorSetAllocInfo, &descriptorSet));

		VkDescriptorImageInfo texDescriptor =
			vkTools::initializers::descriptorImageInfo(
				sampler,
				view,
				VK_IMAGE_LAYOUT_GENERAL);

		std::array<VkWriteDescriptorSet, 1> writeDescriptorSets;
		writeDescriptorSets[0] = vkTools::initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &texDescriptor);
		vkUpdateDescriptorSets(device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);

		// Command buffer

		// Pool
		VkCommandPoolCreateInfo cmdPoolInfo = {};
		cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolInfo.queueFamilyIndex = 0; // todo : pass from example base / swap chain
		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		vkTools::checkResult(vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &commandPool));

		VkCommandBufferAllocateInfo cmdBufAllocateInfo =
			vkTools::initializers::commandBufferAllocateInfo(
				commandPool,
				VK_COMMAND_BUFFER_LEVEL_PRIMARY,
				(uint32_t)cmdBuffers.size());

		vkTools::checkResult(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, cmdBuffers.data()));

		// Pipeline cache
		VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
		pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		vkTools::checkResult(vkCreatePipelineCache(device, &pipelineCacheCreateInfo, nullptr, &pipelineCache));
	}

	// Prepare a separate pipeline for the font rendering decoupled from the main application
	void preparePipeline()
	{
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
			vkTools::initializers::pipelineInputAssemblyStateCreateInfo(
				VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
				0,
				VK_FALSE);

		VkPipelineRasterizationStateCreateInfo rasterizationState =
			vkTools::initializers::pipelineRasterizationStateCreateInfo(
				VK_POLYGON_MODE_FILL,
				VK_CULL_MODE_NONE,
				VK_FRONT_FACE_CLOCKWISE,
				0);

		// Enable blending
		VkPipelineColorBlendAttachmentState blendAttachmentState =
			vkTools::initializers::pipelineColorBlendAttachmentState(0xf, VK_TRUE);

		blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
		blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
		blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
		blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

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
				dynamicStateEnables.size(),
				0);

		std::array<VkVertexInputBindingDescription, 2> vertexBindings = {};
		vertexBindings[0] = vkTools::initializers::vertexInputBindingDescription(0, sizeof(glm::vec4), VK_VERTEX_INPUT_RATE_VERTEX);
		vertexBindings[1] = vkTools::initializers::vertexInputBindingDescription(1, sizeof(glm::vec4), VK_VERTEX_INPUT_RATE_VERTEX);

		std::array<VkVertexInputAttributeDescription, 2> vertexAttribs = {};
		vertexAttribs[0] = vkTools::initializers::vertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32_SFLOAT, 0);
		vertexAttribs[1] = vkTools::initializers::vertexInputAttributeDescription(1, 1, VK_FORMAT_R32G32_SFLOAT, sizeof(glm::vec2));

		VkPipelineVertexInputStateCreateInfo inputState = vkTools::initializers::pipelineVertexInputStateCreateInfo();
		inputState.vertexBindingDescriptionCount = vertexBindings.size();
		inputState.pVertexBindingDescriptions = vertexBindings.data();
		inputState.vertexAttributeDescriptionCount = vertexAttribs.size();
		inputState.pVertexAttributeDescriptions = vertexAttribs.data();

		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

		//shaderStages[0] = loadShader(getAssetPath() + "shaders/base/font.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		//shaderStages[1] = loadShader(getAssetPath() + "shaders/base/font.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

		// todo : pass asset path
		shaderStages[0] = loadShader("../data/shaders/base/font.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader("../data/shaders/base/font.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

		VkGraphicsPipelineCreateInfo pipelineCreateInfo =
			vkTools::initializers::pipelineCreateInfo(
				pipelineLayout,
				renderPass,
				0);

		pipelineCreateInfo.pVertexInputState = &inputState;
		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
		pipelineCreateInfo.pRasterizationState = &rasterizationState;
		pipelineCreateInfo.pColorBlendState = &colorBlendState;
		pipelineCreateInfo.pMultisampleState = &multisampleState;
		pipelineCreateInfo.pViewportState = &viewportState;
		pipelineCreateInfo.pDepthStencilState = &depthStencilState;
		pipelineCreateInfo.pDynamicState = &dynamicState;
		pipelineCreateInfo.stageCount = shaderStages.size();
		pipelineCreateInfo.pStages = shaderStages.data();

		vkTools::checkResult(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipeline));
	}

	// Prepare a separate render pass for rendering the text as an overlay
	void prepareRenderPass()
	{
		VkAttachmentDescription attachments[2] = {};

		// Color attachment
		attachments[0].format = colorFormat;
		attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
		// Don't clear the framebuffer (like the renderpass from the example does)
		attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		// Depth attachment
		attachments[1].format = depthFormat;
		attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorReference = {};
		colorReference.attachment = 0;
		colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthReference = {};
		depthReference.attachment = 1;
		depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.flags = 0;
		subpass.inputAttachmentCount = 0;
		subpass.pInputAttachments = NULL;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorReference;
		subpass.pResolveAttachments = NULL;
		subpass.pDepthStencilAttachment = &depthReference;
		subpass.preserveAttachmentCount = 0;
		subpass.pPreserveAttachments = NULL;

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.pNext = NULL;
		renderPassInfo.attachmentCount = 2;
		renderPassInfo.pAttachments = attachments;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 0;
		renderPassInfo.pDependencies = NULL;

		vkTools::checkResult(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));
	}
	
	// Map buffer 
	void beginTextUpdate()
	{
		vkTools::checkResult(vkMapMemory(device, memory, 0, VK_WHOLE_SIZE, 0, (void **)&mapped));
		numLetters = 0;
	}

	// Add text to the current buffer
	// todo : drop shadow? color attribute?
	void addText(std::string text, float x, float y)
	{
		assert(mapped != nullptr);

#define CHAR_WIDTH 2.0f * 0.75f / (float)windowSize.width - 1.0f  
#define CHAR_HEIGHT 2.0f * 0.75f / (float)windowSize.height - 1.0f

		// Generate a uv mapped quad per char in the new text
		for (auto letter : text)
		{
			stb_fontchar *charData = &stbFontData[(uint32_t)letter - STB_FIRST_CHAR];

			mapped->x = (x + charData->x0) * CHAR_WIDTH;
			mapped->y = (y + charData->y0) * CHAR_HEIGHT;
			mapped->z = charData->s0;
			mapped->w = charData->t0;
			mapped++;

			mapped->x = (x + charData->x1) * CHAR_WIDTH;
			mapped->y = (y + charData->y0) * CHAR_HEIGHT;
			mapped->z = charData->s1;
			mapped->w = charData->t0;
			mapped++;

			mapped->x = (x + charData->x0) * CHAR_WIDTH;
			mapped->y = (y + charData->y1) * CHAR_HEIGHT;
			mapped->z = charData->s0;
			mapped->w = charData->t1;
			mapped++;

			mapped->x = (x + charData->x1) * CHAR_WIDTH;
			mapped->y = (y + charData->y1) * CHAR_HEIGHT;
			mapped->z = charData->s1;
			mapped->w = charData->t1;
			mapped++;

			x += charData->advance_int;

			numLetters++;
		}
	}

	// Unmap buffer and update command buffers
	void endTextUpdate()
	{
		vkUnmapMemory(device, memory);
		mapped = nullptr;
		updateCommandBuffers();
	}

	// Needs to be called by the application
	void updateCommandBuffers()
	{
		VkCommandBufferBeginInfo cmdBufInfo = vkTools::initializers::commandBufferBeginInfo();

		VkClearValue clearValues[1];
		clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };

		VkRenderPassBeginInfo renderPassBeginInfo = vkTools::initializers::renderPassBeginInfo();
		renderPassBeginInfo.renderPass = renderPass;
		renderPassBeginInfo.renderArea.extent = windowSize;
		renderPassBeginInfo.clearValueCount = 1;
		renderPassBeginInfo.pClearValues = clearValues;

		for (int32_t i = 0; i < cmdBuffers.size(); ++i)
		{
			renderPassBeginInfo.framebuffer = frameBuffers[i];

			vkTools::checkResult(vkBeginCommandBuffer(cmdBuffers[i], &cmdBufInfo));

			vkCmdBeginRenderPass(cmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport viewport = vkTools::initializers::viewport((float)windowSize.width, (float)windowSize.height, 0.0f, 1.0f);
			vkCmdSetViewport(cmdBuffers[i], 0, 1, &viewport);

			VkRect2D scissor = vkTools::initializers::rect2D(windowSize.width, windowSize.height, 0, 0);
			vkCmdSetScissor(cmdBuffers[i], 0, 1, &scissor);

			vkCmdBindPipeline(cmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
			vkCmdBindDescriptorSets(cmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);

			VkDeviceSize offsets = 0;
			vkCmdBindVertexBuffers(cmdBuffers[i], 0, 1, &buffer, &offsets);
			vkCmdBindVertexBuffers(cmdBuffers[i], 1, 1, &buffer, &offsets);
			for (uint32_t j = 0; j < numLetters; j++)
			{
				vkCmdDraw(cmdBuffers[i], 4, 1, j * 4, 0);
			}

			vkCmdEndRenderPass(cmdBuffers[i]);

			vkTools::checkResult(vkEndCommandBuffer(cmdBuffers[i]));
		}
	}

	// Submit the text command buffers to a queue
	// Does a queue wait idle
	void submit(VkQueue queue, uint32_t bufferindex)
	{
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cmdBuffers[bufferindex];

		vkTools::checkResult(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
		vkTools::checkResult(vkQueueWaitIdle(queue));
	}

};

class VulkanExample : public VulkanExampleBase
{
public:
	VulkanTextOverlay *textOverlay = nullptr;

	struct {
		vkTools::VulkanTexture colorMap;
	} textures;

	struct {
		VkPipelineVertexInputStateCreateInfo inputState;
		std::vector<VkVertexInputBindingDescription> bindingDescriptions;
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
	} vertices;

	struct {
		vkMeshLoader::MeshBuffer example;
	} meshes;

	struct {
		vkTools::UniformData vsScene;
	} uniformData;

	struct {
		glm::mat4 projection;
		glm::mat4 model;
		glm::vec4 lightPos = glm::vec4(5.0f, 5.0f, 5.0f, 1.0f);
	} uboVS;

	struct {
		VkPipeline solid;
	} pipelines;

	VkPipelineLayout pipelineLayout;
	VkDescriptorSet descriptorSet;
	VkDescriptorSetLayout descriptorSetLayout;

	VulkanExample() : VulkanExampleBase(ENABLE_VALIDATION)
	{
		zoom = -7.5f;
		zoomSpeed = 2.5f;
		rotation = { -5.0f, -35.0f, 0.0f };
		//cameraPos = glm::vec3(4.5f, 0.33f, 0.0f);
		title = "Vulkan Example - Text overlay";
	}

	~VulkanExample()
	{
		// Clean up used Vulkan resources 
		// Note : Inherited destructor cleans up resources stored in base class
		vkDestroyPipeline(device, pipelines.solid, nullptr);

		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

		vkMeshLoader::freeMeshBufferResources(device, &meshes.example);

		textureLoader->destroyTexture(textures.colorMap);

		vkTools::destroyUniformData(device, &uniformData.vsScene);

		delete(textOverlay);
	}

	void buildCommandBuffers()
	{
		VkCommandBufferBeginInfo cmdBufInfo = vkTools::initializers::commandBufferBeginInfo();

		VkClearValue clearValues[3];

		// Clear to a white background for higher contrast
		clearValues[0].color = { { 0.0f, 0.0f, 0.2f, 1.0f } };
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderPassBeginInfo = vkTools::initializers::renderPassBeginInfo();
		renderPassBeginInfo.renderPass = renderPass;
		renderPassBeginInfo.renderArea.extent.width = width;
		renderPassBeginInfo.renderArea.extent.height = height;
		renderPassBeginInfo.clearValueCount = 3;
		renderPassBeginInfo.pClearValues = clearValues;

		for (int32_t i = 0; i < drawCmdBuffers.size(); ++i)
		{
			// Set target frame buffer
			renderPassBeginInfo.framebuffer = frameBuffers[i];

			vkTools::checkResult(vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo));

			vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport viewport = vkTools::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
			vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

			VkRect2D scissor = vkTools::initializers::rect2D(width, height, 0, 0);
			vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

			vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);
			vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.solid);

			VkDeviceSize offsets[1] = { 0 };
			vkCmdBindVertexBuffers(drawCmdBuffers[i], VERTEX_BUFFER_BIND_ID, 1, &meshes.example.vertices.buf, offsets);
			vkCmdBindIndexBuffer(drawCmdBuffers[i], meshes.example.indices.buf, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(drawCmdBuffers[i], meshes.example.indexCount, 1, 0, 0, 0);

			vkCmdEndRenderPass(drawCmdBuffers[i]);

			vkTools::checkResult(vkEndCommandBuffer(drawCmdBuffers[i]));
		}
	}

	// Update the text buffer displayed by the text overlay
	void updateTextOverlay(void)
	{
		textOverlay->beginTextUpdate();

		textOverlay->addText(title, 5.0f, 5.0f);

		std::stringstream ss;
		ss << std::fixed << std::setprecision(2) << (frameTimer * 1000.0f) << "ms (" << lastFPS << " fps)";

		textOverlay->addText(ss.str(), 5.0f, 30.0f);

		textOverlay->addText(deviceProperties.deviceName, 5.0f, 55.0f);

		// Put some text on top of object
		// Project object position into screen space
		glm::vec3 projected = glm::project(cameraPos, uboVS.model, uboVS.projection, glm::vec4(0, 0, (float)width, (float)height));
		textOverlay->addText("Look ma I'm a teapot!", projected.x, projected.y);

		textOverlay->endTextUpdate();
	}

	void draw()
	{
		// Get next image in the swap chain (back/front buffer)
		vkTools::checkResult(swapChain.acquireNextImage(semaphores.presentComplete, &currentBuffer));

		submitPostPresentBarrier(swapChain.buffers[currentBuffer].image);

		// Command buffer to be sumitted to the queue
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];

		// Submit to queue
		vkTools::checkResult(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

		// Submit text overlay to queue
		textOverlay->submit(queue, currentBuffer);

		submitPrePresentBarrier(swapChain.buffers[currentBuffer].image);

		vkTools::checkResult(swapChain.queuePresent(queue, currentBuffer, semaphores.renderComplete));

		vkTools::checkResult(vkQueueWaitIdle(queue));
	}

	void loadTextures()
	{
		textureLoader->loadTexture(
			getAssetPath() + "models/voyager/voyager.ktx",
			VK_FORMAT_BC3_UNORM_BLOCK,
			&textures.colorMap);
	}

	void loadMeshes()
	{
//		loadMesh(getAssetPath() + "models/lunarlander/lunarlander.dae", &meshes.example, vertexLayout, 5.0f);
		loadMesh(getAssetPath() + "models/teapot.3ds", &meshes.example, vertexLayout, 0.15f);
	}

	void setupVertexDescriptions()
	{
		// Binding description
		vertices.bindingDescriptions.resize(1);
		vertices.bindingDescriptions[0] =
			vkTools::initializers::vertexInputBindingDescription(
				VERTEX_BUFFER_BIND_ID,
				vkMeshLoader::vertexSize(vertexLayout),
				VK_VERTEX_INPUT_RATE_VERTEX);

		// Attribute descriptions
		vertices.attributeDescriptions.resize(4);
		// Location 0 : Position
		vertices.attributeDescriptions[0] =
			vkTools::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				0,
				VK_FORMAT_R32G32B32_SFLOAT,
				0);
		// Location 1 : Normal
		vertices.attributeDescriptions[1] =
			vkTools::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				1,
				VK_FORMAT_R32G32B32_SFLOAT,
				sizeof(float) * 3);
		// Location 2 : Texture coordinates
		vertices.attributeDescriptions[2] =
			vkTools::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				2,
				VK_FORMAT_R32G32_SFLOAT,
				sizeof(float) * 6);
		// Location 3 : Color
		vertices.attributeDescriptions[3] =
			vkTools::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				3,
				VK_FORMAT_R32G32B32_SFLOAT,
				sizeof(float) * 8);

		vertices.inputState = vkTools::initializers::pipelineVertexInputStateCreateInfo();
		vertices.inputState.vertexBindingDescriptionCount = vertices.bindingDescriptions.size();
		vertices.inputState.pVertexBindingDescriptions = vertices.bindingDescriptions.data();
		vertices.inputState.vertexAttributeDescriptionCount = vertices.attributeDescriptions.size();
		vertices.inputState.pVertexAttributeDescriptions = vertices.attributeDescriptions.data();
	}

	void setupDescriptorPool()
	{
		// Example uses one ubo and one combined image sampler
		std::vector<VkDescriptorPoolSize> poolSizes =
		{
			vkTools::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1),
			vkTools::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1),
		};

		VkDescriptorPoolCreateInfo descriptorPoolInfo =
			vkTools::initializers::descriptorPoolCreateInfo(
				poolSizes.size(),
				poolSizes.data(),
				2);

		vkTools::checkResult(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));
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
			// Binding 1 : Fragment shader combined sampler
			vkTools::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_SHADER_STAGE_FRAGMENT_BIT,
				1),
		};

		VkDescriptorSetLayoutCreateInfo descriptorLayout =
			vkTools::initializers::descriptorSetLayoutCreateInfo(
				setLayoutBindings.data(),
				setLayoutBindings.size());

		vkTools::checkResult(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayout));

		VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo =
			vkTools::initializers::pipelineLayoutCreateInfo(
				&descriptorSetLayout,
				1);

		vkTools::checkResult(vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &pipelineLayout));
	}

	void setupDescriptorSet()
	{
		VkDescriptorSetAllocateInfo allocInfo =
			vkTools::initializers::descriptorSetAllocateInfo(
				descriptorPool,
				&descriptorSetLayout,
				1);

		vkTools::checkResult(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));

		VkDescriptorImageInfo texDescriptor =
			vkTools::initializers::descriptorImageInfo(
				textures.colorMap.sampler,
				textures.colorMap.view,
				VK_IMAGE_LAYOUT_GENERAL);

		std::vector<VkWriteDescriptorSet> writeDescriptorSets =
		{
			// Binding 0 : Vertex shader uniform buffer
			vkTools::initializers::writeDescriptorSet(
			descriptorSet,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				0,
				&uniformData.vsScene.descriptor),
			// Binding 1 : Color map 
			vkTools::initializers::writeDescriptorSet(
				descriptorSet,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				1,
				&texDescriptor)
		};

		vkUpdateDescriptorSets(device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);
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
				VK_FRONT_FACE_CLOCKWISE,
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
				dynamicStateEnables.size(),
				0);

		// Solid rendering pipeline
		// Load shaders
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

		shaderStages[0] = loadShader(getAssetPath() + "shaders/textoverlay/mesh.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getAssetPath() + "shaders/textoverlay/mesh.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

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
		pipelineCreateInfo.stageCount = shaderStages.size();
		pipelineCreateInfo.pStages = shaderStages.data();

		vkTools::checkResult(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.solid));
	}

	// Prepare and initialize uniform buffer containing shader uniforms
	void prepareUniformBuffers()
	{
		// Vertex shader uniform buffer block
		createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			sizeof(uboVS),
			&uboVS,
			&uniformData.vsScene.buffer,
			&uniformData.vsScene.memory,
			&uniformData.vsScene.descriptor);

		updateUniformBuffers();
	}

	void updateUniformBuffers()
	{
		// Vertex shader
		glm::mat4 viewMatrix = glm::mat4();
		uboVS.projection = glm::perspective(glm::radians(60.0f), (float)width / (float)height, 0.1f, 256.0f);
		viewMatrix = glm::translate(viewMatrix, glm::vec3(0.0f, 0.0f, zoom));

		float offset = 0.5f;
		int uboIndex = 1;
		uboVS.model = glm::mat4();
		uboVS.model = viewMatrix * glm::translate(uboVS.model, cameraPos);
		uboVS.model = glm::rotate(uboVS.model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		uboVS.model = glm::rotate(uboVS.model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		uboVS.model = glm::rotate(uboVS.model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

		uint8_t *pData;
		vkTools::checkResult(vkMapMemory(device, uniformData.vsScene.memory, 0, sizeof(uboVS), 0, (void **)&pData));
		memcpy(pData, &uboVS, sizeof(uboVS));
		vkUnmapMemory(device, uniformData.vsScene.memory);
	}

	void prepareTextOverlay()
	{
		VkExtent2D windowSize = { width, height };
		textOverlay = new VulkanTextOverlay(
			physicalDevice,
			device,
			frameBuffers,
			colorformat,
			depthFormat,
			windowSize
			);
		updateTextOverlay();
	}

	void prepare()
	{
		VulkanExampleBase::prepare();
		loadTextures();
		loadMeshes();
		setupVertexDescriptions();
		prepareUniformBuffers();
		setupDescriptorSetLayout();
		preparePipelines();
		setupDescriptorPool();
		setupDescriptorSet();
		buildCommandBuffers();
		prepareTextOverlay();
		prepared = true;
	}

	virtual void render()
	{
		if (!prepared)
			return;
		draw();
		// todo 
		if (frameCounter == 0)
		{
			updateTextOverlay();
		}
	}

	virtual void viewChanged()
	{
		vkDeviceWaitIdle(device);
		updateUniformBuffers();
		updateTextOverlay();
	}
};

VulkanExample *vulkanExample;

#if defined(_WIN32)
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (vulkanExample != NULL)
	{
		vulkanExample->handleMessages(hWnd, uMsg, wParam, lParam);
	}
	return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}
#elif defined(__linux__) && !defined(__ANDROID__)
static void handleEvent(const xcb_generic_event_t *event)
{
	if (vulkanExample != NULL)
	{
		vulkanExample->handleEvent(event);
	}
}
#endif

// Main entry point
#if defined(_WIN32)
// Windows entry point
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
#elif defined(__ANDROID__)
// Android entry point
void android_main(android_app* state)
#elif defined(__linux__)
// Linux entry point
int main(const int argc, const char *argv[])
#endif
{
#if defined(__ANDROID__)
	// Removing this may cause the compiler to omit the main entry point 
	// which would make the application crash at start
	app_dummy();
#endif
	vulkanExample = new VulkanExample();
#if defined(_WIN32)
	vulkanExample->setupWindow(hInstance, WndProc);
#elif defined(__ANDROID__)
	// Attach vulkan example to global android application state
	state->userData = vulkanExample;
	state->onAppCmd = VulkanExample::handleAppCommand;
	state->onInputEvent = VulkanExample::handleAppInput;
	vulkanExample->androidApp = state;
#elif defined(__linux__)
	vulkanExample->setupWindow();
#endif
#if !defined(__ANDROID__)
	vulkanExample->initSwapchain();
	vulkanExample->prepare();
#endif
	vulkanExample->renderLoop();
	delete(vulkanExample);
#if !defined(__ANDROID__)
	return 0;
#endif
}