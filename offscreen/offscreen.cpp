/*
* Vulkan Example - Offscreen rendering using a separate framebuffer
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

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vulkan/vulkan.h>
#include "vulkanexamplebase.h"

#define VERTEX_BUFFER_BIND_ID 0


// Texture properties
#define TEX_DIM 512
#define TEX_FORMAT VK_FORMAT_R8G8B8A8_UNORM
#define TEX_FILTER VK_FILTER_LINEAR

// Offscreen frame buffer properties
#define FB_DIM TEX_DIM
#define FB_COLOR_FORMAT VK_FORMAT_R8G8B8A8_UNORM

// Vertex layout for this example
std::vector<vkMeshLoader::VertexLayout> vertexLayout =
{
	vkMeshLoader::VERTEX_LAYOUT_POSITION,
	vkMeshLoader::VERTEX_LAYOUT_UV,
	vkMeshLoader::VERTEX_LAYOUT_COLOR,
	vkMeshLoader::VERTEX_LAYOUT_NORMAL
};

class VulkanExample : public CBaseVulkanGame
{
public:
	virtual int32_t			init(CVulkanFramework* pFramework)
	{
		CBaseVulkanGame::init(pFramework);
		m_pFramework->zoom = -6.5f;
		m_pFramework->rotation = { -11.25f, 45.0f, 0.0f };
		m_pFramework->timerSpeed *= 0.25f;
		m_pFramework->title = "Vulkan Example - Offscreen rendering";
		// Values not set here are initialized in the base class constructor
		return 0;
	};
	
	bool debugDisplay = false;

	struct {
		vkTools::VulkanTexture colorMap;
	} textures;
	
	struct {
		vkMeshLoader::MeshBuffer example;
		vkMeshLoader::MeshBuffer quad;
		vkMeshLoader::MeshBuffer plane;
	} meshes;

	struct {
		VkPipelineVertexInputStateCreateInfo inputState;
		std::vector<VkVertexInputBindingDescription> bindingDescriptions;
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
	} vertices;

	struct {
		vkTools::UniformData vsShared;
		vkTools::UniformData vsMirror;
		vkTools::UniformData vsOffScreen;
		vkTools::UniformData vsDebugQuad;
	} uniformData;

	struct UBO {
		glm::mat4 projection;
		glm::mat4 model;
		glm::vec4 lightPos = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	};

	struct {
		UBO vsShared;
	} ubos;

	struct {
		VkPipeline debug;
		VkPipeline shaded;
		VkPipeline mirror;
	} pipelines;

	struct {
		VkPipelineLayout quad;
		VkPipelineLayout offscreen;
	} pipelineLayouts;

	struct {
		VkDescriptorSet offscreen;
		VkDescriptorSet mirror;
		VkDescriptorSet model;
		VkDescriptorSet debugQuad;
	} descriptorSets;

	VkDescriptorSetLayout descriptorSetLayout;

	// Framebuffer for offscreen rendering
	struct FrameBufferAttachment {
		VkImage image;
		VkDeviceMemory mem;
		VkImageView view;
	};
	struct FrameBuffer {
		int32_t width, height;
		VkFramebuffer frameBuffer;		
		FrameBufferAttachment color, depth;
		// Texture target for framebugger blut
		vkTools::VulkanTexture textureTarget;
	} offScreenFrameBuf;

	VkCommandBuffer offScreenCmdBuffer = VK_NULL_HANDLE;

	glm::vec3 meshPos = glm::vec3(0.0f, -1.5f, 0.0f);

	VulkanExample()
	{
	}

	virtual ~VulkanExample()
	{
		// Clean up used Vulkan resources 
		// Note : Inherited destructor cleans up resources stored in base class

		// Textures
		m_pFramework->textureLoader->destroyTexture(offScreenFrameBuf.textureTarget);
		m_pFramework->textureLoader->destroyTexture(textures.colorMap);

		// Frame buffer

		// Color attachment
		vkDestroyImageView(m_pFramework->device, offScreenFrameBuf.color.view, nullptr);
		vkDestroyImage(m_pFramework->device, offScreenFrameBuf.color.image, nullptr);
		vkFreeMemory(m_pFramework->device, offScreenFrameBuf.color.mem, nullptr);

		// Depth attachment
		vkDestroyImageView(m_pFramework->device, offScreenFrameBuf.depth.view, nullptr);
		vkDestroyImage(m_pFramework->device, offScreenFrameBuf.depth.image, nullptr);
		vkFreeMemory(m_pFramework->device, offScreenFrameBuf.depth.mem, nullptr);

		vkDestroyFramebuffer(m_pFramework->device, offScreenFrameBuf.frameBuffer, nullptr);

		vkDestroyPipeline(m_pFramework->device, pipelines.debug, nullptr);
		vkDestroyPipeline(m_pFramework->device, pipelines.shaded, nullptr);
		vkDestroyPipeline(m_pFramework->device, pipelines.mirror, nullptr);

		vkDestroyPipelineLayout(m_pFramework->device, pipelineLayouts.quad, nullptr);
		vkDestroyPipelineLayout(m_pFramework->device, pipelineLayouts.offscreen, nullptr);

		vkDestroyDescriptorSetLayout(m_pFramework->device, descriptorSetLayout, nullptr);

		// Meshes
		vkMeshLoader::freeMeshBufferResources(m_pFramework->device, &meshes.example);
		vkMeshLoader::freeMeshBufferResources(m_pFramework->device, &meshes.quad);
		vkMeshLoader::freeMeshBufferResources(m_pFramework->device, &meshes.plane);

		// Uniform buffers
		vkTools::destroyUniformData(m_pFramework->device, &uniformData.vsShared);
		vkTools::destroyUniformData(m_pFramework->device, &uniformData.vsMirror);
		vkTools::destroyUniformData(m_pFramework->device, &uniformData.vsOffScreen);
		vkTools::destroyUniformData(m_pFramework->device, &uniformData.vsDebugQuad);

		vkFreeCommandBuffers(m_pFramework->device, m_pFramework->cmdPool, 1, &offScreenCmdBuffer);
	}

	// Preapre an empty texture as the blit target from 
	// the offscreen framebuffer
	void prepareTextureTarget(uint32_t width, uint32_t height, VkFormat format)
	{
		m_pFramework->createSetupCommandBuffer();

		VkFormatProperties formatProperties;
		VkResult err;

		// Get device properites for the requested texture format
		vkGetPhysicalDeviceFormatProperties(m_pFramework->physicalDevice, format, &formatProperties);
		// Check if blit destination is supported for the requested format
		// Only try for optimal tiling, linear tiling usually won't support blit as destination anyway
		assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT);

		// Prepare blit target texture
		offScreenFrameBuf.textureTarget.width = width;
		offScreenFrameBuf.textureTarget.height = height;

		VkImageCreateInfo imageCreateInfo = vkTools::initializers::imageCreateInfo();
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.format = format;
		imageCreateInfo.extent = { width, height, 1 };
		imageCreateInfo.mipLevels = 1;
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		// Texture will be sampled in a shader and is also the blit destination
		imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;

		VkMemoryAllocateInfo memAllocInfo = vkTools::initializers::memoryAllocateInfo();
		VkMemoryRequirements memReqs;

		err = vkCreateImage(m_pFramework->device, &imageCreateInfo, nullptr, &offScreenFrameBuf.textureTarget.image);
		assert(!err);
		vkGetImageMemoryRequirements(m_pFramework->device, offScreenFrameBuf.textureTarget.image, &memReqs);
		memAllocInfo.allocationSize = memReqs.size;
		m_pFramework->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memAllocInfo.memoryTypeIndex);
		err = vkAllocateMemory(m_pFramework->device, &memAllocInfo, nullptr, &offScreenFrameBuf.textureTarget.deviceMemory);
		assert(!err);
		err = vkBindImageMemory(m_pFramework->device, offScreenFrameBuf.textureTarget.image, offScreenFrameBuf.textureTarget.deviceMemory, 0);
		assert(!err);

		// Image memory barrier
		// Set initial layout for the offscreen texture transfer destination
		// Will be transformed while updating the texture
		offScreenFrameBuf.textureTarget.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		vkTools::setImageLayout(
			m_pFramework->setupCmdBuffer, 
			offScreenFrameBuf.textureTarget.image,
			VK_IMAGE_ASPECT_COLOR_BIT, 
			VK_IMAGE_LAYOUT_PREINITIALIZED,
			offScreenFrameBuf.textureTarget.imageLayout);

		// Create sampler
		VkSamplerCreateInfo sampler = vkTools::initializers::samplerCreateInfo();
		sampler.magFilter = TEX_FILTER;
		sampler.minFilter = TEX_FILTER;
		sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler.addressModeV = sampler.addressModeU;
		sampler.addressModeW = sampler.addressModeU;
		sampler.mipLodBias = 0.0f;
		sampler.maxAnisotropy = 0;
		sampler.compareOp = VK_COMPARE_OP_NEVER;
		sampler.minLod = 0.0f;
		sampler.maxLod = 0.0f;
		sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		err = vkCreateSampler(m_pFramework->device, &sampler, nullptr, &offScreenFrameBuf.textureTarget.sampler);
		assert(!err);

		// Create image view
		VkImageViewCreateInfo view = {};
		view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view.pNext = NULL;
		view.image = VK_NULL_HANDLE;
		view.viewType = VK_IMAGE_VIEW_TYPE_2D;
		view.format = format;
		view.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		view.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		view.image = offScreenFrameBuf.textureTarget.image;
		err = vkCreateImageView(m_pFramework->device, &view, nullptr, &offScreenFrameBuf.textureTarget.view);
		assert(!err);

		m_pFramework->flushSetupCommandBuffer();
	}

	// Prepare a new framebuffer for offscreen rendering
	// The contents of this framebuffer are then
	// blitted to our render target
	void prepareOffscreenFramebuffer()
	{

		m_pFramework->createSetupCommandBuffer();

		offScreenFrameBuf.width = FB_DIM;
		offScreenFrameBuf.height = FB_DIM;

		VkFormat fbColorFormat = FB_COLOR_FORMAT;

		// Find a suitable depth format
		VkFormat fbDepthFormat;
		VkBool32 validDepthFormat = vkTools::getSupportedDepthFormat(m_pFramework->physicalDevice, &fbDepthFormat);
		assert(validDepthFormat);

		VkResult err;

		// Color attachment
		VkImageCreateInfo image = vkTools::initializers::imageCreateInfo();
		image.imageType = VK_IMAGE_TYPE_2D;
		image.format = fbColorFormat;
		image.extent.width = offScreenFrameBuf.width;
		image.extent.height = offScreenFrameBuf.height;
		image.mipLevels = 1;
		image.arrayLayers = 1;
		image.samples = VK_SAMPLE_COUNT_1_BIT;
		image.tiling = VK_IMAGE_TILING_OPTIMAL;
		// Image of the framebuffer is blit source
		image.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		image.flags = 0;

		VkMemoryAllocateInfo memAlloc = vkTools::initializers::memoryAllocateInfo();
		VkMemoryRequirements memReqs;

		VkImageViewCreateInfo colorImageView = vkTools::initializers::imageViewCreateInfo();
		colorImageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
		colorImageView.format = fbColorFormat;
		colorImageView.flags = 0;
		colorImageView.subresourceRange = {};
		colorImageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		colorImageView.subresourceRange.baseMipLevel = 0;
		colorImageView.subresourceRange.levelCount = 1;
		colorImageView.subresourceRange.baseArrayLayer = 0;
		colorImageView.subresourceRange.layerCount = 1;

		err = vkCreateImage(m_pFramework->device, &image, nullptr, &offScreenFrameBuf.color.image);
		assert(!err);
		vkGetImageMemoryRequirements(m_pFramework->device, offScreenFrameBuf.color.image, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		m_pFramework->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memAlloc.memoryTypeIndex);
		err = vkAllocateMemory(m_pFramework->device, &memAlloc, nullptr, &offScreenFrameBuf.color.mem);
		assert(!err);

		err = vkBindImageMemory(m_pFramework->device, offScreenFrameBuf.color.image, offScreenFrameBuf.color.mem, 0);
		assert(!err);

		vkTools::setImageLayout(
			m_pFramework->setupCmdBuffer,
			offScreenFrameBuf.color.image,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		colorImageView.image = offScreenFrameBuf.color.image;
		err = vkCreateImageView(m_pFramework->device, &colorImageView, nullptr, &offScreenFrameBuf.color.view);
		assert(!err);

		// Depth stencil attachment
		image.format = fbDepthFormat;
		image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

		VkImageViewCreateInfo depthStencilView = vkTools::initializers::imageViewCreateInfo();
		depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
		depthStencilView.format = fbDepthFormat;
		depthStencilView.flags = 0;
		depthStencilView.subresourceRange = {};
		depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		depthStencilView.subresourceRange.baseMipLevel = 0;
		depthStencilView.subresourceRange.levelCount = 1;
		depthStencilView.subresourceRange.baseArrayLayer = 0;
		depthStencilView.subresourceRange.layerCount = 1;

		err = vkCreateImage(m_pFramework->device, &image, nullptr, &offScreenFrameBuf.depth.image);
		assert(!err);
		vkGetImageMemoryRequirements(m_pFramework->device, offScreenFrameBuf.depth.image, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		m_pFramework->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memAlloc.memoryTypeIndex);
		err = vkAllocateMemory(m_pFramework->device, &memAlloc, nullptr, &offScreenFrameBuf.depth.mem);
		assert(!err);

		err = vkBindImageMemory(m_pFramework->device, offScreenFrameBuf.depth.image, offScreenFrameBuf.depth.mem, 0);
		assert(!err);

		vkTools::setImageLayout(
			m_pFramework->setupCmdBuffer,
			offScreenFrameBuf.depth.image,
			VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

		depthStencilView.image = offScreenFrameBuf.depth.image;
		err = vkCreateImageView(m_pFramework->device, &depthStencilView, nullptr, &offScreenFrameBuf.depth.view);
		assert(!err);

		m_pFramework->flushSetupCommandBuffer();

		VkImageView attachments[2];
		attachments[0] = offScreenFrameBuf.color.view;
		attachments[1] = offScreenFrameBuf.depth.view;

		VkFramebufferCreateInfo fbufCreateInfo = vkTools::initializers::framebufferCreateInfo();
		fbufCreateInfo.renderPass = m_pFramework->renderPass;
		fbufCreateInfo.attachmentCount = 2;
		fbufCreateInfo.pAttachments = attachments;
		fbufCreateInfo.width = offScreenFrameBuf.width;
		fbufCreateInfo.height = offScreenFrameBuf.height;
		fbufCreateInfo.layers = 1;

		err = vkCreateFramebuffer(m_pFramework->device, &fbufCreateInfo, nullptr, &offScreenFrameBuf.frameBuffer);
		assert(!err);
	}

	void createOffscreenCommandBuffer()
	{
		VkCommandBufferAllocateInfo cmd = vkTools::initializers::commandBufferAllocateInfo(
			m_pFramework->cmdPool,
			VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			1);
		VkResult vkRes = vkAllocateCommandBuffers(m_pFramework->device, &cmd, &offScreenCmdBuffer);
		assert(!vkRes);
	}

	// The command buffer to copy for rendering 
	// the offscreen scene and blitting it into
	// the texture target is only build once
	// and gets resubmitted 
	void buildOffscreenCommandBuffer()
	{
		VkResult err;

		VkCommandBufferBeginInfo cmdBufInfo = vkTools::initializers::commandBufferBeginInfo();

		VkClearValue clearValues[2];
		clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderPassBeginInfo = vkTools::initializers::renderPassBeginInfo();
		renderPassBeginInfo.renderPass = m_pFramework->renderPass;
		renderPassBeginInfo.framebuffer = offScreenFrameBuf.frameBuffer;
		renderPassBeginInfo.renderArea.extent.width = offScreenFrameBuf.width;
		renderPassBeginInfo.renderArea.extent.height = offScreenFrameBuf.height;
		renderPassBeginInfo.clearValueCount = 2;
		renderPassBeginInfo.pClearValues = clearValues;

		err = vkBeginCommandBuffer(offScreenCmdBuffer, &cmdBufInfo);
		assert(!err);

		vkCmdBeginRenderPass(offScreenCmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = vkTools::initializers::viewport(
			(float)offScreenFrameBuf.width,
			(float)offScreenFrameBuf.height,
			0.0f,
			1.0f);
		vkCmdSetViewport(offScreenCmdBuffer, 0, 1, &viewport);

		VkRect2D scissor = vkTools::initializers::rect2D(
			offScreenFrameBuf.width,
			offScreenFrameBuf.height,
			0,
			0);
		vkCmdSetScissor(offScreenCmdBuffer, 0, 1, &scissor);

		VkDeviceSize offsets[1] = { 0 };

		// Model
		vkCmdBindDescriptorSets(offScreenCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.offscreen, 0, 1, &descriptorSets.offscreen, 0, NULL);
		vkCmdBindPipeline(offScreenCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.shaded);
		vkCmdBindVertexBuffers(offScreenCmdBuffer, VERTEX_BUFFER_BIND_ID, 1, &meshes.example.vertices.buf, offsets);
		vkCmdBindIndexBuffer(offScreenCmdBuffer, meshes.example.indices.buf, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(offScreenCmdBuffer, meshes.example.indexCount, 1, 0, 0, 0);

		vkCmdEndRenderPass(offScreenCmdBuffer);

		// Make sure color writes to the framebuffer are finished before using it as transfer source
		vkTools::setImageLayout(
			offScreenCmdBuffer,
			offScreenFrameBuf.color.image,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

		// Transform texture target to transfer source
		vkTools::setImageLayout(
			offScreenCmdBuffer,
			offScreenFrameBuf.textureTarget.image,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		// Blit offscreen color buffer to our texture target
		VkImageBlit imgBlit;

		imgBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imgBlit.srcSubresource.mipLevel = 0;
		imgBlit.srcSubresource.baseArrayLayer = 0;
		imgBlit.srcSubresource.layerCount = 1;

		imgBlit.srcOffsets[0] = { 0, 0, 0 };
		imgBlit.srcOffsets[1].x = offScreenFrameBuf.width;
		imgBlit.srcOffsets[1].y = offScreenFrameBuf.height;
		imgBlit.srcOffsets[1].z = 1;

		imgBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imgBlit.dstSubresource.mipLevel = 0;
		imgBlit.dstSubresource.baseArrayLayer = 0;
		imgBlit.dstSubresource.layerCount = 1;

		imgBlit.dstOffsets[0] = { 0, 0, 0 };
		imgBlit.dstOffsets[1].x = offScreenFrameBuf.textureTarget.width;
		imgBlit.dstOffsets[1].y = offScreenFrameBuf.textureTarget.height;
		imgBlit.dstOffsets[1].z = 1;

		// Blit from framebuffer image to texture image
		// vkCmdBlitImage does scaling and (if necessary and possible) also does format conversions
		vkCmdBlitImage(
			offScreenCmdBuffer,
			offScreenFrameBuf.color.image,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			offScreenFrameBuf.textureTarget.image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&imgBlit,
			VK_FILTER_LINEAR
			);

		// Transform framebuffer color attachment back 
		vkTools::setImageLayout(
			offScreenCmdBuffer,
			offScreenFrameBuf.color.image,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		// Transform texture target back to shader read
		// Makes sure that writes to the textuer are finished before
		// it's accessed in the shader
		vkTools::setImageLayout(
			offScreenCmdBuffer,
			offScreenFrameBuf.textureTarget.image,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		err = vkEndCommandBuffer(offScreenCmdBuffer);
		assert(!err);
	}

	void buildCommandBuffers()
	{
		VkCommandBufferBeginInfo cmdBufInfo = vkTools::initializers::commandBufferBeginInfo();

		VkClearValue clearValues[2];
		clearValues[0].color = m_pFramework->defaultClearColor;
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderPassBeginInfo = vkTools::initializers::renderPassBeginInfo();
		renderPassBeginInfo.renderPass = m_pFramework->renderPass;
		renderPassBeginInfo.renderArea.offset.x = 0;
		renderPassBeginInfo.renderArea.offset.y = 0;
		renderPassBeginInfo.renderArea.extent.width  = m_pFramework->ScreenRect.Width;
		renderPassBeginInfo.renderArea.extent.height = m_pFramework->ScreenRect.Height;
		renderPassBeginInfo.clearValueCount = 2;
		renderPassBeginInfo.pClearValues = clearValues;

		VkResult err;

		for (int32_t i = 0; i < m_pFramework->drawCmdBuffers.size(); ++i)
		{
			// Set target frame buffer
			renderPassBeginInfo.framebuffer = m_pFramework->frameBuffers[i];

			err = vkBeginCommandBuffer(m_pFramework->drawCmdBuffers[i], &cmdBufInfo);
			assert(!err);

			vkCmdBeginRenderPass(m_pFramework->drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport viewport = vkTools::initializers::viewport(
				(float)m_pFramework->ScreenRect.Width,
				(float)m_pFramework->ScreenRect.Height,
				0.0f,
				1.0f);
			vkCmdSetViewport(m_pFramework->drawCmdBuffers[i], 0, 1, &viewport);

			VkRect2D scissor = vkTools::initializers::rect2D(
				m_pFramework->ScreenRect.Width,
				m_pFramework->ScreenRect.Height,
				0,
				0);
			vkCmdSetScissor(m_pFramework->drawCmdBuffers[i], 0, 1, &scissor);

			VkDeviceSize offsets[1] = { 0 };

			if (debugDisplay)
			{
				vkCmdBindDescriptorSets(m_pFramework->drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.quad, 0, 1, &descriptorSets.debugQuad, 0, NULL);
				vkCmdBindPipeline(m_pFramework->drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.debug);
				vkCmdBindVertexBuffers(m_pFramework->drawCmdBuffers[i], VERTEX_BUFFER_BIND_ID, 1, &meshes.quad.vertices.buf, offsets);
				vkCmdBindIndexBuffer(m_pFramework->drawCmdBuffers[i], meshes.quad.indices.buf, 0, VK_INDEX_TYPE_UINT32);
				vkCmdDrawIndexed(m_pFramework->drawCmdBuffers[i], meshes.quad.indexCount, 1, 0, 0, 0);
			}

			// Scene
			vkCmdBindPipeline(m_pFramework->drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.debug);

			// Reflection plane
			vkCmdBindDescriptorSets(m_pFramework->drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.quad, 0, 1, &descriptorSets.mirror, 0, NULL);
			vkCmdBindPipeline(m_pFramework->drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.mirror);

			vkCmdBindVertexBuffers(m_pFramework->drawCmdBuffers[i], VERTEX_BUFFER_BIND_ID, 1, &meshes.plane.vertices.buf, offsets);
			vkCmdBindIndexBuffer(m_pFramework->drawCmdBuffers[i], meshes.plane.indices.buf, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(m_pFramework->drawCmdBuffers[i], meshes.plane.indexCount, 1, 0, 0, 0);

			// Model
			vkCmdBindDescriptorSets(m_pFramework->drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.quad, 0, 1, &descriptorSets.model, 0, NULL);
			vkCmdBindPipeline(m_pFramework->drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.shaded);

			vkCmdBindVertexBuffers(m_pFramework->drawCmdBuffers[i], VERTEX_BUFFER_BIND_ID, 1, &meshes.example.vertices.buf, offsets);
			vkCmdBindIndexBuffer(m_pFramework->drawCmdBuffers[i], meshes.example.indices.buf, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(m_pFramework->drawCmdBuffers[i], meshes.example.indexCount, 1, 0, 0, 0);

			vkCmdEndRenderPass(m_pFramework->drawCmdBuffers[i]);

			err = vkEndCommandBuffer(m_pFramework->drawCmdBuffers[i]);
			assert(!err);
		}
	}

	void draw()
	{
		VkResult err;

		// Get next image in the swap chain (back/front buffer)
		err = m_pFramework->swapChain.acquireNextImage(m_pFramework->semaphores.presentComplete, &m_pFramework->currentBuffer);
		assert(!err);

		m_pFramework->submitPostPresentBarrier(m_pFramework->swapChain.buffers[m_pFramework->currentBuffer].image);

		// Gather command buffers to be sumitted to the queue
		std::vector<VkCommandBuffer> submitCmdBuffers = {
			offScreenCmdBuffer,
			m_pFramework->drawCmdBuffers[m_pFramework->currentBuffer],
		};
		m_pFramework->submitInfo.commandBufferCount = (uint32_t)submitCmdBuffers.size();
		m_pFramework->submitInfo.pCommandBuffers = submitCmdBuffers.data();

		// Submit to queue
		err = vkQueueSubmit(m_pFramework->queue, 1, &m_pFramework->submitInfo, VK_NULL_HANDLE);
		assert(!err);

		m_pFramework->submitPrePresentBarrier(m_pFramework->swapChain.buffers[m_pFramework->currentBuffer].image);

		err = m_pFramework->swapChain.queuePresent(m_pFramework->queue, m_pFramework->currentBuffer, m_pFramework->semaphores.renderComplete);
		assert(!err);

		err = vkQueueWaitIdle(m_pFramework->queue);
		assert(!err);
	}

	void loadMeshes()
	{
		m_pFramework-> loadMesh(m_pFramework->getAssetPath() + "models/plane.obj", &meshes.plane, vertexLayout, 0.4f);
		m_pFramework-> loadMesh(m_pFramework->getAssetPath() + "models/chinesedragon.dae", &meshes.example, vertexLayout, 0.3f);
	}

	void loadTextures()
	{
		m_pFramework->textureLoader->loadTexture(
			m_pFramework->getAssetPath() + "textures/darkmetal_bc3.ktx",
			VK_FORMAT_BC3_UNORM_BLOCK,
			&textures.colorMap);
	}

	void generateQuad()
	{
		// Setup vertices for a single uv-mapped quad
		struct Vertex {
			float pos[3];
			float uv[2];
			float col[3];
			float normal[3];
		};

#define QUAD_COLOR_NORMAL { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }
		std::vector<Vertex> vertexBuffer =
		{
			{ { 1.0f, 1.0f, 0.0f },{ 1.0f, 1.0f }, QUAD_COLOR_NORMAL },
			{ { 0.0f, 1.0f, 0.0f },{ 0.0f, 1.0f }, QUAD_COLOR_NORMAL },
			{ { 0.0f, 0.0f, 0.0f },{ 0.0f, 0.0f }, QUAD_COLOR_NORMAL },
			{ { 1.0f, 0.0f, 0.0f },{ 1.0f, 0.0f }, QUAD_COLOR_NORMAL }
		};
#undef QUAD_COLOR_NORMAL

		m_pFramework->createBuffer(
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			vertexBuffer.size() * sizeof(Vertex),
			vertexBuffer.data(),
			&meshes.quad.vertices.buf,
			&meshes.quad.vertices.mem);

		// Setup indices
		std::vector<uint32_t> indexBuffer = { 0,1,2, 2,3,0 };
		meshes.quad.indexCount = (uint32_t)indexBuffer.size();

		m_pFramework->createBuffer(
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			indexBuffer.size() * sizeof(uint32_t),
			indexBuffer.data(),
			&meshes.quad.indices.buf,
			&meshes.quad.indices.mem);
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
		// Location 1 : Texture coordinates
		vertices.attributeDescriptions[1] =
			vkTools::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				1,
				VK_FORMAT_R32G32_SFLOAT,
				sizeof(float) * 3);
		// Location 2 : Color
		vertices.attributeDescriptions[2] =
			vkTools::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				2,
				VK_FORMAT_R32G32B32_SFLOAT,
				sizeof(float) * 5);
		// Location 3 : Normal
		vertices.attributeDescriptions[3] =
			vkTools::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				3,
				VK_FORMAT_R32G32B32_SFLOAT,
				sizeof(float) * 8);

		vertices.inputState = vkTools::initializers::pipelineVertexInputStateCreateInfo();
		vertices.inputState.vertexBindingDescriptionCount = (uint32_t)vertices.bindingDescriptions.size();
		vertices.inputState.pVertexBindingDescriptions = vertices.bindingDescriptions.data();
		vertices.inputState.vertexAttributeDescriptionCount = (uint32_t)vertices.attributeDescriptions.size();
		vertices.inputState.pVertexAttributeDescriptions = vertices.attributeDescriptions.data();
	}

	void setupDescriptorPool()
	{
		std::vector<VkDescriptorPoolSize> poolSizes =
		{
			vkTools::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 6),
			vkTools::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 8)
		};

		VkDescriptorPoolCreateInfo descriptorPoolInfo =
			vkTools::initializers::descriptorPoolCreateInfo(
				(uint32_t)poolSizes.size(),
				poolSizes.data(),
				5);

		VkResult vkRes = vkCreateDescriptorPool(m_pFramework->device, &descriptorPoolInfo, nullptr, &m_pFramework->descriptorPool);
		assert(!vkRes);
	}

	void setupDescriptorSetLayout()
	{
		// Textured quad pipeline layout
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
				1),
			// Binding 2 : Fragment shader image sampler
			vkTools::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_SHADER_STAGE_FRAGMENT_BIT,
				2)
		};

		VkDescriptorSetLayoutCreateInfo descriptorLayout =
			vkTools::initializers::descriptorSetLayoutCreateInfo(
				setLayoutBindings.data(),
				(uint32_t)setLayoutBindings.size());

		VkResult err = vkCreateDescriptorSetLayout(m_pFramework->device, &descriptorLayout, nullptr, &descriptorSetLayout);
		assert(!err);

		VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo =
			vkTools::initializers::pipelineLayoutCreateInfo(
				&descriptorSetLayout,
				1);

		err = vkCreatePipelineLayout(m_pFramework->device, &pPipelineLayoutCreateInfo, nullptr, &pipelineLayouts.quad);
		assert(!err);

		// Offscreen pipeline layout
		err = vkCreatePipelineLayout(m_pFramework->device, &pPipelineLayoutCreateInfo, nullptr, &pipelineLayouts.offscreen);
		assert(!err);
	}

	void setupDescriptorSet()
	{
		// Mirror plane descriptor set
		VkDescriptorSetAllocateInfo allocInfo =
			vkTools::initializers::descriptorSetAllocateInfo(
				m_pFramework->descriptorPool,
				&descriptorSetLayout,
				1);

		VkResult vkRes = vkAllocateDescriptorSets(m_pFramework->device, &allocInfo, &descriptorSets.mirror);
		assert(!vkRes);

		// Image descriptor for the offscreen mirror texture
		VkDescriptorImageInfo texDescriptorMirror =
			vkTools::initializers::descriptorImageInfo(
				offScreenFrameBuf.textureTarget.sampler,
				offScreenFrameBuf.textureTarget.view,
				VK_IMAGE_LAYOUT_GENERAL);

		// Image descriptor for the color map
		VkDescriptorImageInfo texDescriptorColorMap =
			vkTools::initializers::descriptorImageInfo(
				textures.colorMap.sampler,
				textures.colorMap.view,
				VK_IMAGE_LAYOUT_GENERAL);

		std::vector<VkWriteDescriptorSet> writeDescriptorSets =
		{
			// Binding 0 : Vertex shader uniform buffer
			vkTools::initializers::writeDescriptorSet(
				descriptorSets.mirror,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				0,
				&uniformData.vsMirror.descriptor),
			// Binding 1 : Fragment shader texture sampler
			vkTools::initializers::writeDescriptorSet(
				descriptorSets.mirror,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				1,
				&texDescriptorMirror),
			// Binding 2 : Fragment shader texture sampler
			vkTools::initializers::writeDescriptorSet(
				descriptorSets.mirror,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				2,
				&texDescriptorColorMap)
		};

		vkUpdateDescriptorSets(m_pFramework->device, (uint32_t)writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);

		// Model
		// No texture
		vkRes = vkAllocateDescriptorSets(m_pFramework->device, &allocInfo, &descriptorSets.model);
		assert(!vkRes);

		std::vector<VkWriteDescriptorSet> modelWriteDescriptorSets =
		{
			// Binding 0 : Vertex shader uniform buffer
			vkTools::initializers::writeDescriptorSet(
				descriptorSets.model,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				0,
				&uniformData.vsShared.descriptor)
		};
		vkUpdateDescriptorSets(m_pFramework->device, (uint32_t)modelWriteDescriptorSets.size(), modelWriteDescriptorSets.data(), 0, NULL);

		// Offscreen
		vkRes = vkAllocateDescriptorSets(m_pFramework->device, &allocInfo, &descriptorSets.offscreen);
		assert(!vkRes);

		std::vector<VkWriteDescriptorSet> offScreenWriteDescriptorSets =
		{
			// Binding 0 : Vertex shader uniform buffer
			vkTools::initializers::writeDescriptorSet(
			descriptorSets.offscreen,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				0,
				&uniformData.vsOffScreen.descriptor)
		};
		vkUpdateDescriptorSets(m_pFramework->device, (uint32_t)offScreenWriteDescriptorSets.size(), offScreenWriteDescriptorSets.data(), 0, NULL);

		// Debug quad
		vkRes = vkAllocateDescriptorSets(m_pFramework->device, &allocInfo, &descriptorSets.debugQuad);
		assert(!vkRes);

		std::vector<VkWriteDescriptorSet> debugQuadWriteDescriptorSets =
		{
			// Binding 0 : Vertex shader uniform buffer
			vkTools::initializers::writeDescriptorSet(
				descriptorSets.debugQuad,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				0,
				&uniformData.vsDebugQuad.descriptor),
			// Binding 1 : Fragment shader texture sampler
			vkTools::initializers::writeDescriptorSet(
				descriptorSets.debugQuad,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				1,
				&texDescriptorMirror)
		};
		vkUpdateDescriptorSets(m_pFramework->device, (uint32_t)debugQuadWriteDescriptorSets.size(), debugQuadWriteDescriptorSets.data(), 0, NULL);
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
				(uint32_t)dynamicStateEnables.size(),
				0);

		// Solid rendering pipeline
		// Load shaders
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

		shaderStages[0] = m_pFramework->loadShader(m_pFramework->getAssetPath() + "shaders/offscreen/quad.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = m_pFramework->loadShader(m_pFramework->getAssetPath() + "shaders/offscreen/quad.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

		VkGraphicsPipelineCreateInfo pipelineCreateInfo =
			vkTools::initializers::pipelineCreateInfo(
				pipelineLayouts.quad,
				m_pFramework->renderPass,
				0);

		pipelineCreateInfo.pVertexInputState = &vertices.inputState;
		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
		pipelineCreateInfo.pRasterizationState = &rasterizationState;
		pipelineCreateInfo.pColorBlendState = &colorBlendState;
		pipelineCreateInfo.pMultisampleState = &multisampleState;
		pipelineCreateInfo.pViewportState = &viewportState;
		pipelineCreateInfo.pDepthStencilState = &depthStencilState;
		pipelineCreateInfo.pDynamicState = &dynamicState;
		pipelineCreateInfo.stageCount = (uint32_t)shaderStages.size();
		pipelineCreateInfo.pStages = shaderStages.data();

		VkResult err = vkCreateGraphicsPipelines(m_pFramework->device, m_pFramework->pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.debug);
		assert(!err);

		// Mirror
		shaderStages[0] = m_pFramework->loadShader(m_pFramework->getAssetPath() + "shaders/offscreen/mirror.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = m_pFramework->loadShader(m_pFramework->getAssetPath() + "shaders/offscreen/mirror.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

		err = vkCreateGraphicsPipelines(m_pFramework->device, m_pFramework->pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.mirror);
		assert(!err);

		// Solid shading pipeline
		shaderStages[0] = m_pFramework->loadShader(m_pFramework->getAssetPath() + "shaders/offscreen/offscreen.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = m_pFramework->loadShader(m_pFramework->getAssetPath() + "shaders/offscreen/offscreen.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

		pipelineCreateInfo.layout = pipelineLayouts.offscreen;

		err = vkCreateGraphicsPipelines(m_pFramework->device, m_pFramework->pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.shaded);
		assert(!err);
	}

	// Prepare and initialize uniform buffer containing shader uniforms
	void prepareUniformBuffers()
	{
		// Mesh vertex shader uniform buffer block
		m_pFramework->createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			sizeof(ubos.vsShared),
			nullptr,
			&uniformData.vsShared.buffer,
			&uniformData.vsShared.memory,
			&uniformData.vsShared.descriptor);

		// Mirror plane vertex shader uniform buffer block
		m_pFramework->createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			sizeof(ubos.vsShared),
			nullptr,
			&uniformData.vsMirror.buffer,
			&uniformData.vsMirror.memory,
			&uniformData.vsMirror.descriptor);

		// Offscreen vertex shader uniform buffer block 
		m_pFramework->createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			sizeof(ubos.vsShared),
			nullptr,
			&uniformData.vsOffScreen.buffer,
			&uniformData.vsOffScreen.memory,
			&uniformData.vsOffScreen.descriptor);

		// Debug quad vertex shader uniform buffer block 
		m_pFramework->createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			sizeof(ubos.vsShared),
			nullptr,
			&uniformData.vsDebugQuad.buffer,
			&uniformData.vsDebugQuad.memory,
			&uniformData.vsDebugQuad.descriptor);

		updateUniformBuffers();
		updateUniformBufferOffscreen();
	}

	void updateUniformBuffers()
	{
		// Mesh
		ubos.vsShared.projection = glm::perspective(deg_to_rad(60.0f), (float)m_pFramework->ScreenRect.Width / (float)m_pFramework->ScreenRect.Height, 0.1f, 256.0f);
		glm::mat4 viewMatrix = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, m_pFramework->zoom));

		ubos.vsShared.model = viewMatrix * glm::translate(glm::mat4(), glm::vec3(0, 0, 0));
		ubos.vsShared.model = glm::rotate(ubos.vsShared.model, glm::radians(m_pFramework->rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		ubos.vsShared.model = glm::rotate(ubos.vsShared.model, glm::radians(m_pFramework->rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		ubos.vsShared.model = glm::rotate(ubos.vsShared.model, glm::radians(m_pFramework->rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

		ubos.vsShared.model = glm::translate(ubos.vsShared.model, meshPos);

		uint8_t *pData;
		VkResult err = vkMapMemory(m_pFramework->device, uniformData.vsShared.memory, 0, sizeof(ubos.vsShared), 0, (void **)&pData);
		assert(!err);
		memcpy(pData, &ubos.vsShared, sizeof(ubos.vsShared));
		vkUnmapMemory(m_pFramework->device, uniformData.vsShared.memory);

		// Mirror
		ubos.vsShared.model = viewMatrix * glm::translate(glm::mat4(), glm::vec3(0, 0, 0));
		ubos.vsShared.model = glm::rotate(ubos.vsShared.model, glm::radians(m_pFramework->rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		ubos.vsShared.model = glm::rotate(ubos.vsShared.model, glm::radians(m_pFramework->rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		ubos.vsShared.model = glm::rotate(ubos.vsShared.model, glm::radians(m_pFramework->rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

		err = vkMapMemory(m_pFramework->device, uniformData.vsMirror.memory, 0, sizeof(ubos.vsShared), 0, (void **)&pData);
		assert(!err);
		memcpy(pData, &ubos.vsShared, sizeof(ubos.vsShared));
		vkUnmapMemory(m_pFramework->device, uniformData.vsMirror.memory);

		// Debug quad
		ubos.vsShared.projection = glm::ortho(0.0f, 4.0f, 0.0f, 4.0f*(float)m_pFramework->ScreenRect.Height / (float)m_pFramework->ScreenRect.Width, -1.0f, 1.0f);
		ubos.vsShared.model = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, 0.0f));

		err = vkMapMemory(m_pFramework->device, uniformData.vsDebugQuad.memory, 0, sizeof(ubos.vsShared), 0, (void **)&pData);
		assert(!err);
		memcpy(pData, &ubos.vsShared, sizeof(ubos.vsShared));
		vkUnmapMemory(m_pFramework->device, uniformData.vsDebugQuad.memory);
	}

	void updateUniformBufferOffscreen()
	{
		ubos.vsShared.projection = glm::perspective(deg_to_rad(60.0f), (float)m_pFramework->ScreenRect.Width / (float)m_pFramework->ScreenRect.Height, 0.1f, 256.0f);
		glm::mat4 viewMatrix = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, m_pFramework->zoom));

		ubos.vsShared.model = viewMatrix * glm::translate(glm::mat4(), glm::vec3(0, 0, 0));
		ubos.vsShared.model = glm::rotate(ubos.vsShared.model, glm::radians(m_pFramework->rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		ubos.vsShared.model = glm::rotate(ubos.vsShared.model, glm::radians(m_pFramework->rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		ubos.vsShared.model = glm::rotate(ubos.vsShared.model, glm::radians(m_pFramework->rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

		ubos.vsShared.model = glm::scale(ubos.vsShared.model, glm::vec3(1.0f, -1.0f, 1.0f));
		ubos.vsShared.model = glm::translate(ubos.vsShared.model, meshPos);

		uint8_t *pData;
		VkResult err = vkMapMemory(m_pFramework->device, uniformData.vsOffScreen.memory, 0, sizeof(ubos.vsShared), 0, (void **)&pData);
		assert(!err);
		memcpy(pData, &ubos.vsShared, sizeof(ubos.vsShared));
		vkUnmapMemory(m_pFramework->device, uniformData.vsOffScreen.memory);
	}

	virtual int32_t	prepare()
	{
		CBaseVulkanGame::prepare();
		loadTextures();
		generateQuad();
		loadMeshes();
		setupVertexDescriptions();
		prepareUniformBuffers();
		prepareTextureTarget(TEX_DIM, TEX_DIM, TEX_FORMAT);
		setupDescriptorSetLayout();
		preparePipelines();
		setupDescriptorPool();
		setupDescriptorSet();
		createOffscreenCommandBuffer();
		prepareOffscreenFramebuffer();
		buildCommandBuffers();
		buildOffscreenCommandBuffer(); 
		m_pFramework->prepared = true;
		return 0;
	}

	virtual int32_t render()
	{
		if (!m_pFramework->prepared)
			return 1;
		vkDeviceWaitIdle(m_pFramework->device);
		draw();
		vkDeviceWaitIdle(m_pFramework->device);
		if (!m_pFramework->paused)
		{
			updateUniformBuffers();
			updateUniformBufferOffscreen();
		}
		return 0;
	}

	virtual void viewChanged()
	{
		updateUniformBuffers();
		updateUniformBufferOffscreen();
	}

	virtual void	keyPressed(uint32_t keyCode)
	{
	}

};

DEFINE_VULKAN_GAME_CREATE_AND_RELEASE_FUNCTIONS()