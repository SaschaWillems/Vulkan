/*
* Assorted commonly used Vulkan helper functions
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#include "vulkan/vulkan.h"

#include <math.h>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <fstream>
#include <assert.h>
#include <stdio.h>
#include <vector>
#include <iostream>
#include <stdexcept>
#if defined(_WIN32)
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#elif defined(__ANDROID__)
#include "vulkanandroid.h"
#include <android/asset_manager.h>
#endif

// Custom define for better code readability
#define VK_FLAGS_NONE 0
// Default fence timeout in nanoseconds
#define DEFAULT_FENCE_TIMEOUT 100000000000

// Macro to check and display Vulkan return results
#define VK_CHECK_RESULT(f)																				\
{																										\
	VkResult res = (f);																					\
	if (res != VK_SUCCESS)																				\
	{																									\
		std::cout << "Fatal : VkResult is \"" << vkTools::errorString(res) << "\" in " << __FILE__ << " at line " << __LINE__ << std::endl; \
		assert(res == VK_SUCCESS);																		\
	}																									\
}																										\

namespace vkTools
{
	// Check if extension is globally available
	VkBool32 checkGlobalExtensionPresent(const char* extensionName);
	// Check if extension is present on the given device
	VkBool32 checkDeviceExtensionPresent(VkPhysicalDevice physicalDevice, const char* extensionName);
	// Return string representation of a vulkan error string
	std::string errorString(VkResult errorCode);

	// Selected a suitable supported depth format starting with 32 bit down to 16 bit
	// Returns false if none of the depth formats in the list is supported by the device
	VkBool32 getSupportedDepthFormat(VkPhysicalDevice physicalDevice, VkFormat *depthFormat);

	// Put an image memory barrier for setting an image layout on the sub resource into the given command buffer
	void setImageLayout(
		VkCommandBuffer cmdbuffer,
		VkImage image,
		VkImageAspectFlags aspectMask,
		VkImageLayout oldImageLayout,
		VkImageLayout newImageLayout,
		VkImageSubresourceRange subresourceRange,
		VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	// Uses a fixed sub resource layout with first mip level and layer
	void setImageLayout(
		VkCommandBuffer cmdbuffer, 
		VkImage image, 
		VkImageAspectFlags aspectMask, 
		VkImageLayout oldImageLayout, 
		VkImageLayout newImageLayout,
		VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

	// Display error message and exit on fatal error
	void exitFatal(std::string message, std::string caption);

	// Load a SPIR-V shader (binary) 
#if defined(__ANDROID__)
	VkShaderModule loadShader(AAssetManager* assetManager, const char *fileName, VkDevice device, VkShaderStageFlagBits stage);
#else
	VkShaderModule loadShader(const char *fileName, VkDevice device, VkShaderStageFlagBits stage);
#endif

	// Load a GLSL shader (text)
	// Note: GLSL support requires vendor-specific extensions to be enabled and is not a core-feature of Vulkan
	VkShaderModule loadShaderGLSL(const char *fileName, VkDevice device, VkShaderStageFlagBits stage);

	// Contains often used vulkan object initializers
	// Save lot of VK_STRUCTURE_TYPE assignments
	// Some initializers are parameterized for convenience
	namespace initializers
	{
		VkMemoryAllocateInfo memoryAllocateInfo();

		VkMappedMemoryRange mappedMemoryRange();

		VkCommandBufferAllocateInfo commandBufferAllocateInfo(
			VkCommandPool commandPool,
			VkCommandBufferLevel level,
			uint32_t bufferCount);

		VkCommandPoolCreateInfo commandPoolCreateInfo();
		VkCommandBufferBeginInfo commandBufferBeginInfo();
		VkCommandBufferInheritanceInfo commandBufferInheritanceInfo();

		VkRenderPassBeginInfo renderPassBeginInfo();
		VkRenderPassCreateInfo renderPassCreateInfo();

		VkImageMemoryBarrier imageMemoryBarrier();
		VkBufferMemoryBarrier bufferMemoryBarrier();
		VkMemoryBarrier memoryBarrier();

		VkImageCreateInfo imageCreateInfo();
		VkSamplerCreateInfo samplerCreateInfo();
		VkImageViewCreateInfo imageViewCreateInfo();

		VkFramebufferCreateInfo framebufferCreateInfo();

		VkSemaphoreCreateInfo semaphoreCreateInfo();
		VkFenceCreateInfo fenceCreateInfo(VkFenceCreateFlags flags = VK_FLAGS_NONE);
		VkEventCreateInfo eventCreateInfo();

		VkSubmitInfo submitInfo();

		VkViewport viewport(
			float width, 
			float height, 
			float minDepth, 
			float maxDepth);

		VkRect2D rect2D(
			int32_t width,
			int32_t height,
			int32_t offsetX,
			int32_t offsetY);

		VkBufferCreateInfo bufferCreateInfo();

		VkBufferCreateInfo bufferCreateInfo(
			VkBufferUsageFlags usage, 
			VkDeviceSize size);

		VkDescriptorPoolCreateInfo descriptorPoolCreateInfo(
			uint32_t poolSizeCount,
			VkDescriptorPoolSize* pPoolSizes,
			uint32_t maxSets);

		VkDescriptorPoolSize descriptorPoolSize(
			VkDescriptorType type,
			uint32_t descriptorCount);

		VkDescriptorSetLayoutBinding descriptorSetLayoutBinding(
			VkDescriptorType type, 
			VkShaderStageFlags stageFlags, 
			uint32_t binding,
			uint32_t count = 1);

		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo(
			const VkDescriptorSetLayoutBinding* pBindings,
			uint32_t bindingCount);

		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo(
			const VkDescriptorSetLayout* pSetLayouts,
			uint32_t setLayoutCount);
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo(
			uint32_t setLayoutCount = 1);

		VkDescriptorSetAllocateInfo descriptorSetAllocateInfo(
			VkDescriptorPool descriptorPool,
			const VkDescriptorSetLayout* pSetLayouts,
			uint32_t descriptorSetCount);

		VkDescriptorImageInfo descriptorImageInfo(
			VkSampler sampler,
			VkImageView imageView,
			VkImageLayout imageLayout);

		VkWriteDescriptorSet writeDescriptorSet(
			VkDescriptorSet dstSet, 
			VkDescriptorType type, 
			uint32_t binding, 
			VkDescriptorBufferInfo* bufferInfo);

		VkWriteDescriptorSet writeDescriptorSet(
			VkDescriptorSet dstSet, 
			VkDescriptorType type, 
			uint32_t binding, 
			VkDescriptorImageInfo* imageInfo);

		VkVertexInputBindingDescription vertexInputBindingDescription(
			uint32_t binding, 
			uint32_t stride, 
			VkVertexInputRate inputRate);

		VkVertexInputAttributeDescription vertexInputAttributeDescription(
			uint32_t binding,
			uint32_t location,
			VkFormat format,
			uint32_t offset);

		VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo();

		VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo(
			VkPrimitiveTopology topology,
			VkPipelineInputAssemblyStateCreateFlags flags,
			VkBool32 primitiveRestartEnable);

		VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo(
			VkPolygonMode polygonMode,
			VkCullModeFlags cullMode,
			VkFrontFace frontFace,
			VkPipelineRasterizationStateCreateFlags flags);

		VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState(
			VkColorComponentFlags colorWriteMask,
			VkBool32 blendEnable);

		VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo(
			uint32_t attachmentCount,
			const VkPipelineColorBlendAttachmentState* pAttachments);

		VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo(
			VkBool32 depthTestEnable,
			VkBool32 depthWriteEnable,
			VkCompareOp depthCompareOp);

		VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo(
			uint32_t viewportCount,
			uint32_t scissorCount,
			VkPipelineViewportStateCreateFlags flags);

		VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo(
			VkSampleCountFlagBits rasterizationSamples,
			VkPipelineMultisampleStateCreateFlags flags);

		VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo(
			const VkDynamicState *pDynamicStates,
			uint32_t dynamicStateCount,
			VkPipelineDynamicStateCreateFlags flags);

		VkPipelineTessellationStateCreateInfo pipelineTessellationStateCreateInfo(
			uint32_t patchControlPoints);

		VkGraphicsPipelineCreateInfo pipelineCreateInfo(
			VkPipelineLayout layout,
			VkRenderPass renderPass,
			VkPipelineCreateFlags flags);

		VkComputePipelineCreateInfo computePipelineCreateInfo(
			VkPipelineLayout layout,
			VkPipelineCreateFlags flags);

		VkPushConstantRange pushConstantRange(
			VkShaderStageFlags stageFlags,
			uint32_t size,
			uint32_t offset);

		VkBindSparseInfo bindSparseInfo();

		/** @brief Initialize a map entry for a shader specialization constant */
		VkSpecializationMapEntry specializationMapEntry(uint32_t constantID, uint32_t offset, size_t size);

		/** @biref Initialize a specialization constant info structure to pass to a shader stage */
		VkSpecializationInfo specializationInfo(uint32_t mapEntryCount, const VkSpecializationMapEntry* mapEntries, size_t dataSize, const void* data);
	}

}
