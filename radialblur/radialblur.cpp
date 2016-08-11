/*
* Vulkan Example - Fullscreen radial blur (Single pass offscreen effect)
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
#define ENABLE_VALIDATION false

// Texture properties
#define TEX_DIM 128
#define TEX_FORMAT VK_FORMAT_R8G8B8A8_UNORM
#define TEX_FILTER VK_FILTER_LINEAR;

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

class VulkanExample : public VulkanExampleBase
{
public:
	bool blur = true;
	bool displayTexture = false;

	struct {
		vkMeshLoader::MeshBuffer example;
		vkMeshLoader::MeshBuffer quad;
	} meshes;

	struct {
		VkPipelineVertexInputStateCreateInfo inputState;
		std::vector<VkVertexInputBindingDescription> bindingDescriptions;
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
	} vertices;

	struct {
		vkTools::UniformData vsScene;
		vkTools::UniformData vsQuad;
		vkTools::UniformData fsQuad;
	} uniformData;

	struct {
		glm::mat4 projection;
		glm::mat4 model;
	} uboVS;

	struct {
		glm::mat4 projection;
		glm::mat4 model;
	} uboQuadVS;

	struct {
		int32_t texWidth = TEX_DIM;
		int32_t texHeight = TEX_DIM;
		float radialBlurScale = 0.25f;
		float radialBlurStrength = 0.75f;
		glm::vec2 radialOrigin = glm::vec2(0.5f, 0.5f);
	} uboQuadFS;

	struct {
		VkPipeline radialBlur;
		VkPipeline colorPass;
		VkPipeline phongPass;
		VkPipeline fullScreenOnly;
	} pipelines;

	struct {
		VkPipelineLayout radialBlur;
		VkPipelineLayout scene;
	} pipelineLayouts;

	struct {
		VkDescriptorSet scene;
		VkDescriptorSet quad;
	} descriptorSets;

	// Descriptor set layout is shared amongst
	// all descriptor sets
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
		VkSampler colorSampler;
	} offScreenFrameBuf;

	VkCommandBuffer offScreenCmdBuffer = VK_NULL_HANDLE;

	// Semaphore used to synchronize between offscreen and final scene rendering
	VkSemaphore offscreenSemaphore = VK_NULL_HANDLE;

	VulkanExample() : VulkanExampleBase(ENABLE_VALIDATION)
	{
		zoom = -12.0f;
		rotation = { -16.25f, -28.75f, 0.0f };
		timerSpeed *= 0.5f;
		enableTextOverlay = true;
		title = "Vulkan Example - Radial blur";
	}

	~VulkanExample()
	{
		// Clean up used Vulkan resources 
		// Note : Inherited destructor cleans up resources stored in base class

		// Frame buffer
		vkDestroySampler(device, offScreenFrameBuf.colorSampler, nullptr);

		// Color attachment
		vkDestroyImageView(device, offScreenFrameBuf.color.view, nullptr);
		vkDestroyImage(device, offScreenFrameBuf.color.image, nullptr);
		vkFreeMemory(device, offScreenFrameBuf.color.mem, nullptr);

		// Depth attachment
		vkDestroyImageView(device, offScreenFrameBuf.depth.view, nullptr);
		vkDestroyImage(device, offScreenFrameBuf.depth.image, nullptr);
		vkFreeMemory(device, offScreenFrameBuf.depth.mem, nullptr);

		vkDestroyFramebuffer(device, offScreenFrameBuf.frameBuffer, nullptr);

		vkDestroyPipeline(device, pipelines.radialBlur, nullptr);
		vkDestroyPipeline(device, pipelines.phongPass, nullptr);
		vkDestroyPipeline(device, pipelines.colorPass, nullptr);
		vkDestroyPipeline(device, pipelines.fullScreenOnly, nullptr);

		vkDestroyPipelineLayout(device, pipelineLayouts.radialBlur, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayouts.scene, nullptr);

		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

		// Meshes
		vkMeshLoader::freeMeshBufferResources(device, &meshes.example);
		vkMeshLoader::freeMeshBufferResources(device, &meshes.quad);

		// Uniform buffers
		vkTools::destroyUniformData(device, &uniformData.vsScene);
		vkTools::destroyUniformData(device, &uniformData.vsQuad);
		vkTools::destroyUniformData(device, &uniformData.fsQuad);

		vkFreeCommandBuffers(device, cmdPool, 1, &offScreenCmdBuffer);
		vkDestroySemaphore(device, offscreenSemaphore, nullptr);
	}

	// Preapre an empty texture as the blit target from 
	// the offscreen framebuffer
	void prepareTextureTarget(vkTools::VulkanTexture *tex, uint32_t width, uint32_t height, VkFormat format)
	{
		VkCommandBuffer cmdBuffer = VulkanExampleBase::createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

		VkFormatProperties formatProperties;

		// Get device properites for the requested texture format
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);
		// Check if blit destination is supported for the requested format
		// Only try for optimal tiling, linear tiling usually won't support blit as destination anyway
		assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT);

		// Prepare blit target texture
		tex->width = width;
		tex->height = height;

		VkImageCreateInfo imageCreateInfo = vkTools::initializers::imageCreateInfo();
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.format = format;
		imageCreateInfo.extent = { width, height, 1 };
		imageCreateInfo.mipLevels = 1;
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		// Texture will be sampled in a shader and is also the blit destination
		imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		VkMemoryAllocateInfo memAllocInfo = vkTools::initializers::memoryAllocateInfo();
		VkMemoryRequirements memReqs;

		VK_CHECK_RESULT(vkCreateImage(device, &imageCreateInfo, nullptr, &tex->image));
		vkGetImageMemoryRequirements(device, tex->image, &memReqs);
		memAllocInfo.allocationSize = memReqs.size;
		memAllocInfo.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device, &memAllocInfo, nullptr, &(tex->deviceMemory)));
		VK_CHECK_RESULT(vkBindImageMemory(device, tex->image, tex->deviceMemory, 0));

		// Transform image layout to transfer destination
		tex->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		vkTools::setImageLayout(
			cmdBuffer,
			tex->image,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			tex->imageLayout);

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
		VK_CHECK_RESULT(vkCreateSampler(device, &sampler, nullptr, &tex->sampler));

		// Create image view
		VkImageViewCreateInfo view = vkTools::initializers::imageViewCreateInfo();
		view.image = VK_NULL_HANDLE;
		view.viewType = VK_IMAGE_VIEW_TYPE_2D;
		view.format = format;
		view.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		view.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		view.image = tex->image;
		VK_CHECK_RESULT(vkCreateImageView(device, &view, nullptr, &tex->view));

		VulkanExampleBase::flushCommandBuffer(cmdBuffer, queue, true);
	}

	// Setup the offscreen framebuffer for rendering the blurred scene
	// The color attachment of this framebuffer will then be used
	// to sample frame in the fragment shader of the final pass
	void prepareOffscreenFramebuffer()
	{
		offScreenFrameBuf.width = FB_DIM;
		offScreenFrameBuf.height = FB_DIM;

		VkFormat fbColorFormat = FB_COLOR_FORMAT;

		// Find a suitable depth format
		VkFormat fbDepthFormat;
		VkBool32 validDepthFormat = vkTools::getSupportedDepthFormat(physicalDevice, &fbDepthFormat);
		assert(validDepthFormat);

		// Color attachment
		VkImageCreateInfo image = vkTools::initializers::imageCreateInfo();
		image.imageType = VK_IMAGE_TYPE_2D;
		image.format = fbColorFormat;
		image.extent.width = offScreenFrameBuf.width;
		image.extent.height = offScreenFrameBuf.height;
		image.extent.depth = 1;
		image.mipLevels = 1;
		image.arrayLayers = 1;
		image.samples = VK_SAMPLE_COUNT_1_BIT;
		image.tiling = VK_IMAGE_TILING_OPTIMAL;
		// We will sample directly from the color attachment
		image.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		VkMemoryAllocateInfo memAlloc = vkTools::initializers::memoryAllocateInfo();
		VkMemoryRequirements memReqs;

		VkImageViewCreateInfo colorImageView = vkTools::initializers::imageViewCreateInfo();
		colorImageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
		colorImageView.format = fbColorFormat;
		colorImageView.subresourceRange = {};
		colorImageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		colorImageView.subresourceRange.baseMipLevel = 0;
		colorImageView.subresourceRange.levelCount = 1;
		colorImageView.subresourceRange.baseArrayLayer = 0;
		colorImageView.subresourceRange.layerCount = 1;

		VK_CHECK_RESULT(vkCreateImage(device, &image, nullptr, &offScreenFrameBuf.color.image));
		vkGetImageMemoryRequirements(device, offScreenFrameBuf.color.image, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		memAlloc.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &offScreenFrameBuf.color.mem));
		VK_CHECK_RESULT(vkBindImageMemory(device, offScreenFrameBuf.color.image, offScreenFrameBuf.color.mem, 0));

		// Get a primary command buffer for submitting image layout transitions for the framebuffer attachments
		VkCommandBuffer layoutCmd = VulkanExampleBase::createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

		// Set the initial layout to shader read instead of attachment 
		// This is done as the render loop does the actualy image layout transitions
		vkTools::setImageLayout(
			layoutCmd,
			offScreenFrameBuf.color.image,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		colorImageView.image = offScreenFrameBuf.color.image;
		VK_CHECK_RESULT(vkCreateImageView(device, &colorImageView, nullptr, &offScreenFrameBuf.color.view));

		// Create sampler to sample from to collor attachment 
		// Used to sample in the fragment shader for final rendering
		VkSamplerCreateInfo sampler = vkTools::initializers::samplerCreateInfo();
		sampler.magFilter = VK_FILTER_LINEAR;
		sampler.minFilter = VK_FILTER_LINEAR;
		sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler.addressModeV = sampler.addressModeU;
		sampler.addressModeW = sampler.addressModeU;
		sampler.mipLodBias = 0.0f;
		sampler.maxAnisotropy = 0;
		sampler.minLod = 0.0f;
		sampler.maxLod = 1.0f;
		sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		VK_CHECK_RESULT(vkCreateSampler(device, &sampler, nullptr, &offScreenFrameBuf.colorSampler));

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

		VK_CHECK_RESULT(vkCreateImage(device, &image, nullptr, &offScreenFrameBuf.depth.image));
		vkGetImageMemoryRequirements(device, offScreenFrameBuf.depth.image, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		memAlloc.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &offScreenFrameBuf.depth.mem));
		VK_CHECK_RESULT(vkBindImageMemory(device, offScreenFrameBuf.depth.image, offScreenFrameBuf.depth.mem, 0));

		vkTools::setImageLayout(
			layoutCmd,
			offScreenFrameBuf.depth.image, 
			VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 
			VK_IMAGE_LAYOUT_UNDEFINED, 
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

		// Submit the command buffer to apply the image memory barrier
		VulkanExampleBase::flushCommandBuffer(layoutCmd, queue, true);

		depthStencilView.image = offScreenFrameBuf.depth.image;
		VK_CHECK_RESULT(vkCreateImageView(device, &depthStencilView, nullptr, &offScreenFrameBuf.depth.view));

		VkImageView attachments[2];
		attachments[0] = offScreenFrameBuf.color.view;
		attachments[1] = offScreenFrameBuf.depth.view;

		VkFramebufferCreateInfo fbufCreateInfo = vkTools::initializers::framebufferCreateInfo();
		fbufCreateInfo.renderPass = renderPass;
		fbufCreateInfo.attachmentCount = 2;
		fbufCreateInfo.pAttachments = attachments;
		fbufCreateInfo.width = offScreenFrameBuf.width;
		fbufCreateInfo.height = offScreenFrameBuf.height;
		fbufCreateInfo.layers = 1;
		VK_CHECK_RESULT(vkCreateFramebuffer(device, &fbufCreateInfo, nullptr, &offScreenFrameBuf.frameBuffer));
	}

	// Sets up the command buffer that renders the scene to the offscreen frame buffer
	void buildOffscreenCommandBuffer()
	{
		if (offScreenCmdBuffer == VK_NULL_HANDLE)
		{
			offScreenCmdBuffer = VulkanExampleBase::createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, false);
		}

		// Create a semaphore used to synchronize offscreen rendering and usage
		VkSemaphoreCreateInfo semaphoreCreateInfo = vkTools::initializers::semaphoreCreateInfo();
		VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &offscreenSemaphore));

		VkCommandBufferBeginInfo cmdBufInfo = vkTools::initializers::commandBufferBeginInfo();

		VkClearValue clearValues[2];
		clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderPassBeginInfo = vkTools::initializers::renderPassBeginInfo();
		renderPassBeginInfo.renderPass = renderPass;
		renderPassBeginInfo.framebuffer = offScreenFrameBuf.frameBuffer;
		renderPassBeginInfo.renderArea.extent.width = offScreenFrameBuf.width;
		renderPassBeginInfo.renderArea.extent.height = offScreenFrameBuf.height;
		renderPassBeginInfo.clearValueCount = 2;
		renderPassBeginInfo.pClearValues = clearValues;

		VK_CHECK_RESULT(vkBeginCommandBuffer(offScreenCmdBuffer, &cmdBufInfo));

		// Change back layout of the color attachment after sampling in the fragment shader
		vkTools::setImageLayout(
			offScreenCmdBuffer,
			offScreenFrameBuf.color.image,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		VkViewport viewport = vkTools::initializers::viewport((float)offScreenFrameBuf.width, (float)offScreenFrameBuf.height, 0.0f, 1.0f);
		vkCmdSetViewport(offScreenCmdBuffer, 0, 1, &viewport);

		VkRect2D scissor = vkTools::initializers::rect2D(offScreenFrameBuf.width, offScreenFrameBuf.height, 0, 0);
		vkCmdSetScissor(offScreenCmdBuffer, 0, 1, &scissor);

		vkCmdBeginRenderPass(offScreenCmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindDescriptorSets(offScreenCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.scene, 0, 1, &descriptorSets.scene, 0, NULL);
		vkCmdBindPipeline(offScreenCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.colorPass);

		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(offScreenCmdBuffer, VERTEX_BUFFER_BIND_ID, 1, &meshes.example.vertices.buf, offsets);
		vkCmdBindIndexBuffer(offScreenCmdBuffer, meshes.example.indices.buf, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(offScreenCmdBuffer, meshes.example.indexCount, 1, 0, 0, 0);

		vkCmdEndRenderPass(offScreenCmdBuffer);

		// Change layout of the color attachment for sampling in the fragment shader
		vkTools::setImageLayout(
			offScreenCmdBuffer,
			offScreenFrameBuf.color.image,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		VK_CHECK_RESULT(vkEndCommandBuffer(offScreenCmdBuffer));
	}

	void reBuildCommandBuffers()
	{
		if (!checkCommandBuffers())
		{
			destroyCommandBuffers();
			createCommandBuffers();
		}
		buildCommandBuffers();
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

			VkDeviceSize offsets[1] = { 0 };

			// 3D scene
			vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.scene, 0, 1, &descriptorSets.scene, 0, NULL);
			vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.phongPass);

			vkCmdBindVertexBuffers(drawCmdBuffers[i], VERTEX_BUFFER_BIND_ID, 1, &meshes.example.vertices.buf, offsets);
			vkCmdBindIndexBuffer(drawCmdBuffers[i], meshes.example.indices.buf, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(drawCmdBuffers[i], meshes.example.indexCount, 1, 0, 0, 0);

			// Fullscreen quad with radial blur
			if (blur)
			{
				vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.radialBlur, 0, 1, &descriptorSets.quad, 0, NULL);
				vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, (displayTexture) ? pipelines.fullScreenOnly : pipelines.radialBlur);
				vkCmdBindVertexBuffers(drawCmdBuffers[i], VERTEX_BUFFER_BIND_ID, 1, &meshes.quad.vertices.buf, offsets);
				vkCmdBindIndexBuffer(drawCmdBuffers[i], meshes.quad.indices.buf, 0, VK_INDEX_TYPE_UINT32);
				vkCmdDrawIndexed(drawCmdBuffers[i], meshes.quad.indexCount, 1, 0, 0, 0);
			}

			vkCmdEndRenderPass(drawCmdBuffers[i]);

			VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
		}
	}

	void loadMeshes()
	{
		loadMesh(getAssetPath() + "models/glowsphere.dae", &meshes.example, vertexLayout, 0.05f);
	}

	// Setup vertices for a single uv-mapped quad
	void generateQuad()
	{
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

		createBuffer(
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			vertexBuffer.size() * sizeof(Vertex),
			vertexBuffer.data(),
			&meshes.quad.vertices.buf,
			&meshes.quad.vertices.mem);

		// Setup indices
		std::vector<uint32_t> indexBuffer = { 0,1,2, 2,3,0 };
		meshes.quad.indexCount = indexBuffer.size();

		createBuffer(
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
		vertices.inputState.vertexBindingDescriptionCount = vertices.bindingDescriptions.size();
		vertices.inputState.pVertexBindingDescriptions = vertices.bindingDescriptions.data();
		vertices.inputState.vertexAttributeDescriptionCount = vertices.attributeDescriptions.size();
		vertices.inputState.pVertexAttributeDescriptions = vertices.attributeDescriptions.data();
	}

	void setupDescriptorPool()
	{
		// Example uses three ubos and one image sampler
		std::vector<VkDescriptorPoolSize> poolSizes =
		{
			vkTools::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4),
			vkTools::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2)
		};

		VkDescriptorPoolCreateInfo descriptorPoolInfo =
			vkTools::initializers::descriptorPoolCreateInfo(
				poolSizes.size(),
				poolSizes.data(),
				2);

		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));
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
			// Binding 2 : Fragment shader uniform buffer
			vkTools::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				VK_SHADER_STAGE_FRAGMENT_BIT,
				2)
		};

		VkDescriptorSetLayoutCreateInfo descriptorLayout =
			vkTools::initializers::descriptorSetLayoutCreateInfo(
				setLayoutBindings.data(),
				setLayoutBindings.size());

		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayout));

		VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo =
			vkTools::initializers::pipelineLayoutCreateInfo(
				&descriptorSetLayout,
				1);

		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &pipelineLayouts.radialBlur));

		// Offscreen pipeline layout
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &pipelineLayouts.scene));
	}

	void setupDescriptorSet()
	{
		// Textured quad descriptor set
		VkDescriptorSetAllocateInfo allocInfo =
			vkTools::initializers::descriptorSetAllocateInfo(
				descriptorPool,
				&descriptorSetLayout,
				1);

		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.quad));

		// Image descriptor for the offscreen color attachment image
		VkDescriptorImageInfo texDescriptor =
			vkTools::initializers::descriptorImageInfo(
				offScreenFrameBuf.colorSampler,
				offScreenFrameBuf.color.view,
				VK_IMAGE_LAYOUT_GENERAL);

		std::vector<VkWriteDescriptorSet> writeDescriptorSets =
		{
			// Binding 0 : Vertex shader uniform buffer
			vkTools::initializers::writeDescriptorSet(
				descriptorSets.quad,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				0,
				&uniformData.vsScene.descriptor),
			// Binding 1 : Fragment shader texture sampler
			vkTools::initializers::writeDescriptorSet(
				descriptorSets.quad,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				1,
				&texDescriptor),
			// Binding 2 : Fragment shader uniform buffer
			vkTools::initializers::writeDescriptorSet(
				descriptorSets.quad,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				2,
				&uniformData.fsQuad.descriptor)
		};

		vkUpdateDescriptorSets(device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);

		// Offscreen 3D scene descriptor set
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.scene));

		std::vector<VkWriteDescriptorSet> offScreenWriteDescriptorSets =
		{
			// Binding 0 : Vertex shader uniform buffer
			vkTools::initializers::writeDescriptorSet(
				descriptorSets.scene,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				0,
				&uniformData.vsQuad.descriptor)
		};
		vkUpdateDescriptorSets(device, offScreenWriteDescriptorSets.size(), offScreenWriteDescriptorSets.data(), 0, NULL);
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
				dynamicStateEnables.size(),
				0);

		// Radial blur pipeline
		// Load shaders
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

		shaderStages[0] = loadShader(getAssetPath() + "shaders/radialblur/radialblur.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getAssetPath() + "shaders/radialblur/radialblur.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

		VkGraphicsPipelineCreateInfo pipelineCreateInfo =
			vkTools::initializers::pipelineCreateInfo(
				pipelineLayouts.radialBlur,
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

		// Additive blending
		blendAttachmentState.colorWriteMask = 0xF;
		blendAttachmentState.blendEnable = VK_TRUE;
		blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
		blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
		blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
		blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;

		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.radialBlur));

		// No blending (for debug display)
		blendAttachmentState.blendEnable = VK_FALSE;
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.fullScreenOnly));

		// Phong pass
		shaderStages[0] = loadShader(getAssetPath() + "shaders/radialblur/phongpass.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getAssetPath() + "shaders/radialblur/phongpass.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

		pipelineCreateInfo.layout = pipelineLayouts.scene;
		blendAttachmentState.blendEnable = VK_FALSE;
		depthStencilState.depthWriteEnable = VK_TRUE;

		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.phongPass));

		// Color only pass (offscreen blur base)
		shaderStages[0] = loadShader(getAssetPath() + "shaders/radialblur/colorpass.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getAssetPath() + "shaders/radialblur/colorpass.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.colorPass));
	}

	// Prepare and initialize uniform buffer containing shader uniforms
	void prepareUniformBuffers()
	{
		// Phong and color pass vertex shader uniform buffer
		createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			sizeof(uboVS),
			&uboVS,
			&uniformData.vsScene.buffer,
			&uniformData.vsScene.memory,
			&uniformData.vsScene.descriptor);

		// Fullscreen quad vertex shader uniform buffer
		createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			sizeof(uboVS),
			&uboVS,
			&uniformData.vsQuad.buffer,
			&uniformData.vsQuad.memory,
			&uniformData.vsQuad.descriptor);

		// Fullscreen quad fragment shader uniform buffer
		createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			sizeof(uboQuadFS),
			&uboQuadFS,
			&uniformData.fsQuad.buffer,
			&uniformData.fsQuad.memory,
			&uniformData.fsQuad.descriptor);

		updateUniformBuffersScene();
		updateUniformBuffersScreen();
	}

	// Update uniform buffers for rendering the 3D scene
	void updateUniformBuffersScene()
	{
		uboQuadVS.projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 1.0f, 256.0f);
		glm::mat4 viewMatrix = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, zoom));

		uboQuadVS.model = glm::mat4();
		uboQuadVS.model = viewMatrix * glm::translate(uboQuadVS.model, cameraPos);
		uboQuadVS.model = glm::rotate(uboQuadVS.model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		uboQuadVS.model = glm::rotate(uboQuadVS.model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		uboQuadVS.model = glm::rotate(uboQuadVS.model, glm::radians(timer * 360.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		uboQuadVS.model = glm::rotate(uboQuadVS.model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

		uint8_t *pData;
		VK_CHECK_RESULT(vkMapMemory(device, uniformData.vsQuad.memory, 0, sizeof(uboQuadVS), 0, (void **)&pData));
		memcpy(pData, &uboQuadVS, sizeof(uboQuadVS));
		vkUnmapMemory(device, uniformData.vsQuad.memory);
	}

	// Update uniform buffers for the fullscreen quad
	void updateUniformBuffersScreen() 
	{
		// Vertex shader
		uboVS.projection = glm::ortho(0.0f, 1.0f, 0.0f, 1.0f, -1.0f, 1.0f);
		uboVS.model = glm::mat4();

		uint8_t *pData;
		VK_CHECK_RESULT(vkMapMemory(device, uniformData.vsScene.memory, 0, sizeof(uboVS), 0, (void **)&pData));
		memcpy(pData, &uboVS, sizeof(uboVS));
		vkUnmapMemory(device, uniformData.vsScene.memory);

		// Fragment shader
		VK_CHECK_RESULT(vkMapMemory(device, uniformData.fsQuad.memory, 0, sizeof(uboQuadFS), 0, (void **)&pData));
		memcpy(pData, &uboQuadFS, sizeof(uboQuadFS));
		vkUnmapMemory(device, uniformData.fsQuad.memory);
	}

	void draw()
	{
		VulkanExampleBase::prepareFrame();

		// The scene render command buffer has to wait for the offscreen
		// rendering to be finished before we can use the framebuffer 
		// color image for sampling during final rendering
		// To ensure this we use a dedicated offscreen synchronization
		// semaphore that will be signaled when offscreen renderin
		// has been finished
		// This is necessary as an implementation may start both
		// command buffers at the same time, there is no guarantee
		// that command buffers will be executed in the order they
		// have been submitted by the application

		// Offscreen rendering

		// Wait for swap chain presentation to finish
		submitInfo.pWaitSemaphores = &semaphores.presentComplete;
		// Signal ready with offscreen semaphore
		submitInfo.pSignalSemaphores = &offscreenSemaphore;

		// Submit work
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &offScreenCmdBuffer;
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

		// Scene rendering

		// Wait for offscreen semaphore
		submitInfo.pWaitSemaphores = &offscreenSemaphore;
		// Signal ready with render complete semaphpre
		submitInfo.pSignalSemaphores = &semaphores.renderComplete;

		// Submit work
		submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

		VulkanExampleBase::submitFrame();
	}

	void prepare()
	{
		VulkanExampleBase::prepare();
		generateQuad();
		loadMeshes();
		prepareOffscreenFramebuffer();
		setupVertexDescriptions();
		prepareUniformBuffers();
		setupDescriptorSetLayout();
		preparePipelines();
		setupDescriptorPool();
		setupDescriptorSet();
		buildCommandBuffers();
		buildOffscreenCommandBuffer();
		prepared = true;
	}

	virtual void render()
	{
		if (!prepared)
			return;
		draw();
		if (!paused)
		{
			updateUniformBuffersScene();
		}
	}

	virtual void viewChanged()
	{
		updateUniformBuffersScene();
		updateUniformBuffersScreen();
	}

	virtual void keyPressed(uint32_t keyCode)
	{
		switch (keyCode)
		{
		case KEY_B:
		case GAMEPAD_BUTTON_A:
			toggleBlur();
			break;
		case KEY_T:
		case GAMEPAD_BUTTON_X:
			toggleTextureDisplay();
			break;
		}
	}

	virtual void getOverlayText(VulkanTextOverlay *textOverlay)
	{
#if defined(__ANDROID__)
		textOverlay->addText("Press \"Button A\" to toggle blur", 5.0f, 85.0f, VulkanTextOverlay::alignLeft);
		textOverlay->addText("Press \"Button X\" to display offscreen texture", 5.0f, 105.0f, VulkanTextOverlay::alignLeft);
#else
		textOverlay->addText("Press \"B\" to toggle blur", 5.0f, 85.0f, VulkanTextOverlay::alignLeft);
		textOverlay->addText("Press \"T\" to display offscreen texture", 5.0f, 105.0f, VulkanTextOverlay::alignLeft);
#endif
	}

	void toggleBlur()
	{
		blur = !blur;
		updateUniformBuffersScene();
		reBuildCommandBuffers();
	}

	void toggleTextureDisplay()
	{
		displayTexture = !displayTexture;
		reBuildCommandBuffers();
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
