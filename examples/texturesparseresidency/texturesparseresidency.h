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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <vector>
#include <algorithm>
#include <random>
#include <chrono>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vulkan/vulkan.h>
#include "vulkanexamplebase.h"
#include "VulkanDevice.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanModel.hpp"

#define ENABLE_VALIDATION false

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

	VirtualTexturePage();
	bool resident();
	void allocate(VkDevice device, uint32_t memoryTypeIndex);
	void release(VkDevice device);
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
	uint32_t mipTailStart;												// First mip level in mip tail
	VkSparseImageMemoryRequirements sparseImageMemoryRequirements;		// @todo: Comment
	uint32_t memoryTypeIndex;											// @todo: Comment

	// @todo: comment
	struct MipTailInfo {
		bool singleMipTail;
		bool alingedMipSize;
	} mipTailInfo;

	VirtualTexturePage *addPage(VkOffset3D offset, VkExtent3D extent, const VkDeviceSize size, const uint32_t mipLevel, uint32_t layer);
	void updateSparseBindInfo();
	// @todo: replace with dtor?
	void destroy();
};

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

	vks::VertexLayout vertexLayout = vks::VertexLayout({
		vks::VERTEX_COMPONENT_POSITION,
		vks::VERTEX_COMPONENT_NORMAL,
		vks::VERTEX_COMPONENT_UV,
	});
	vks::Model plane;

	struct UboVS {
		glm::mat4 projection;
		glm::mat4 model;
		glm::vec4 viewPos;
		float lodBias = 0.0f;
	} uboVS;
	vks::Buffer uniformBufferVS;

	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;
	VkDescriptorSet descriptorSet;
	VkDescriptorSetLayout descriptorSetLayout;

	//todo: comment
	VkSemaphore bindSparseSemaphore = VK_NULL_HANDLE;

	VulkanExample();
	~VulkanExample();
	virtual void getEnabledFeatures();
	glm::uvec3 alignedDivision(const VkExtent3D& extent, const VkExtent3D& granularity);
	void prepareSparseTexture(uint32_t width, uint32_t height, uint32_t layerCount, VkFormat format);
	// @todo: move to dtor of texture
	void destroyTextureImage(SparseTexture texture);
	void buildCommandBuffers();
	void draw();
	void loadAssets();
	void setupDescriptorPool();
	void setupDescriptorSetLayout();
	void setupDescriptorSet();
	void preparePipelines();
	void prepareUniformBuffers();
	void updateUniformBuffers();
	void prepare();
	virtual void render();
	void uploadContent(VirtualTexturePage page, VkImage image);
	void fillRandomPages();
	void fillMipTail();
	void flushRandomPages();
	virtual void OnUpdateUIOverlay(vks::UIOverlay* overlay);
};
