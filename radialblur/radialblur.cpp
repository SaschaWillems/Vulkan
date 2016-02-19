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
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vulkan/vulkan.h>
#include "vulkanexamplebase.h"

#define VERTEX_BUFFER_BIND_ID 0
//#define USE_GLSL
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
		// Texture target for framebuffer blit
		vkTools::VulkanTexture textureTarget;
	} offScreenFrameBuf;

	VkCommandBuffer offScreenCmdBuffer = VK_NULL_HANDLE;

	VulkanExample() : VulkanExampleBase(ENABLE_VALIDATION)
	{
		zoom = -12.0f;
		rotation = { -16.25f, -28.75f, 0.0f };
		timerSpeed *= 0.5f;
		title = "Vulkan Example - Radial blur";
	}

	~VulkanExample()
	{
		// Clean up used Vulkan resources 
		// Note : Inherited destructor cleans up resources stored in base class

		// Texture target
		textureLoader->destroyTexture(offScreenFrameBuf.textureTarget);

		// Frame buffer

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
	}

	// Preapre an empty texture as the blit target from 
	// the offscreen framebuffer
	void prepareTextureTarget(vkTools::VulkanTexture *tex, uint32_t width, uint32_t height, VkFormat format)
	{
		VkFormatProperties formatProperties;
		VkResult err;

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
		// Texture will be sampled in a shader and is also the blit destination
		imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		VkMemoryAllocateInfo memAllocInfo = vkTools::initializers::memoryAllocateInfo();
		VkMemoryRequirements memReqs;

		err = vkCreateImage(device, &imageCreateInfo, nullptr, &tex->image);
		assert(!err);
		vkGetImageMemoryRequirements(device, tex->image, &memReqs);
		memAllocInfo.allocationSize = memReqs.size;
		getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memAllocInfo.memoryTypeIndex);
		err = vkAllocateMemory(device, &memAllocInfo, nullptr, &(tex->deviceMemory));
		assert(!err);
		err = vkBindImageMemory(device, tex->image, tex->deviceMemory, 0);
		assert(!err);

		// Transform image layout to transer destination
		tex->imageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		vkTools::setImageLayout(
			setupCmdBuffer,
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
		err = vkCreateSampler(device, &sampler, nullptr, &tex->sampler);
		assert(!err);

		// Create image view
		VkImageViewCreateInfo view = vkTools::initializers::imageViewCreateInfo();
		view.image = VK_NULL_HANDLE;
		view.viewType = VK_IMAGE_VIEW_TYPE_2D;
		view.format = format;
		view.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		view.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		view.image = tex->image;
		err = vkCreateImageView(device, &view, nullptr, &tex->view);
		assert(!err);
	}

	// Prepare a new framebuffer for offscreen rendering
	// The contents of this framebuffer are then
	// blitted to our render target
	void prepareOffscreenFramebuffer()
	{
		createSetupCommandBuffer();

		offScreenFrameBuf.width = FB_DIM;
		offScreenFrameBuf.height = FB_DIM;

		VkFormat fbColorFormat = FB_COLOR_FORMAT;

		// Find a suitable depth format
		VkFormat fbDepthFormat;
		VkBool32 validDepthFormat = vkTools::getSupportedDepthFormat(physicalDevice, &fbDepthFormat);
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

		err = vkCreateImage(device, &image, nullptr, &offScreenFrameBuf.color.image);
		assert(!err);
		vkGetImageMemoryRequirements(device, offScreenFrameBuf.color.image, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memAlloc.memoryTypeIndex);
		err = vkAllocateMemory(device, &memAlloc, nullptr, &offScreenFrameBuf.color.mem);
		assert(!err);

		err = vkBindImageMemory(device, offScreenFrameBuf.color.image, offScreenFrameBuf.color.mem, 0);
		assert(!err);

		vkTools::setImageLayout(
			setupCmdBuffer, 
			offScreenFrameBuf.color.image, 
			VK_IMAGE_ASPECT_COLOR_BIT, 
			VK_IMAGE_LAYOUT_UNDEFINED, 
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		colorImageView.image = offScreenFrameBuf.color.image;
		err = vkCreateImageView(device, &colorImageView, nullptr, &offScreenFrameBuf.color.view);
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

		err = vkCreateImage(device, &image, nullptr, &offScreenFrameBuf.depth.image);
		assert(!err);
		vkGetImageMemoryRequirements(device, offScreenFrameBuf.depth.image, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memAlloc.memoryTypeIndex);
		err = vkAllocateMemory(device, &memAlloc, nullptr, &offScreenFrameBuf.depth.mem);
		assert(!err);

		err = vkBindImageMemory(device, offScreenFrameBuf.depth.image, offScreenFrameBuf.depth.mem, 0);
		assert(!err);

		vkTools::setImageLayout(
			setupCmdBuffer, 
			offScreenFrameBuf.depth.image, 
			VK_IMAGE_ASPECT_DEPTH_BIT, 
			VK_IMAGE_LAYOUT_UNDEFINED, 
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

		depthStencilView.image = offScreenFrameBuf.depth.image;
		err = vkCreateImageView(device, &depthStencilView, nullptr, &offScreenFrameBuf.depth.view);
		assert(!err);

		flushSetupCommandBuffer();

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

		err = vkCreateFramebuffer(device, &fbufCreateInfo, nullptr, &offScreenFrameBuf.frameBuffer);
		assert(!err);
	}

	void createOffscreenCommandBuffer()
	{
		VkCommandBufferAllocateInfo cmd = vkTools::initializers::commandBufferAllocateInfo(
			cmdPool,
			VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			1);
		VkResult vkRes = vkAllocateCommandBuffers(device, &cmd, &offScreenCmdBuffer);
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
		renderPassBeginInfo.renderPass = renderPass;
		renderPassBeginInfo.framebuffer = offScreenFrameBuf.frameBuffer;
		renderPassBeginInfo.renderArea.extent.width = offScreenFrameBuf.width;
		renderPassBeginInfo.renderArea.extent.height = offScreenFrameBuf.height;
		renderPassBeginInfo.clearValueCount = 2;
		renderPassBeginInfo.pClearValues = clearValues;

		err = vkBeginCommandBuffer(offScreenCmdBuffer, &cmdBufInfo);
		assert(!err);

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

		vkCmdBeginRenderPass(offScreenCmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindDescriptorSets(offScreenCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.scene, 0, 1, &descriptorSets.scene, 0, NULL);
		vkCmdBindPipeline(offScreenCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.colorPass);

		VkDeviceSize offsets[1] = { 0 };
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

		// Transform texture target to transfer destination
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
		// Makes sure that writes to the texture are finished before
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

		VkResult err;

		for (int32_t i = 0; i < drawCmdBuffers.size(); ++i)
		{
			// Set target frame buffer
			renderPassBeginInfo.framebuffer = frameBuffers[i];

			err = vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo);
			assert(!err);

			vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport viewport = vkTools::initializers::viewport(
				(float)width,
				(float)height,
				0.0f,
				1.0f);
			vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

			VkRect2D scissor = vkTools::initializers::rect2D(
				width,
				height,
				0,
				0);
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

			VkImageMemoryBarrier prePresentBarrier = vkTools::prePresentBarrier(swapChain.buffers[i].image);
			VkImageMemoryBarrier *pMemoryBarrier = &prePresentBarrier;
			vkCmdPipelineBarrier(
				drawCmdBuffers[i],
				VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_FLAGS_NONE,
				0, nullptr,
				0, nullptr,
				1, &prePresentBarrier);

			err = vkEndCommandBuffer(drawCmdBuffers[i]);
			assert(!err);
		}
	}

	void draw()
	{
		VkResult err;
		VkSemaphore presentCompleteSemaphore;
		VkSemaphoreCreateInfo presentCompleteSemaphoreCreateInfo =
			vkTools::initializers::semaphoreCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);

		err = vkCreateSemaphore(device, &presentCompleteSemaphoreCreateInfo, nullptr, &presentCompleteSemaphore);
		assert(!err);

		// Get next image in the swap chain (back/front buffer)
		err = swapChain.acquireNextImage(presentCompleteSemaphore, &currentBuffer);
		assert(!err);

		// Gather command buffers to be sumitted to the queue
		std::vector<VkCommandBuffer> submitCmdBuffers = {
			offScreenCmdBuffer,
			drawCmdBuffers[currentBuffer]
		};

		VkSubmitInfo submitInfo = vkTools::initializers::submitInfo();
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &presentCompleteSemaphore;
		submitInfo.commandBufferCount = submitCmdBuffers.size();
		submitInfo.pCommandBuffers = submitCmdBuffers.data();

		// Submit draw command buffer
		err = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
		assert(!err);

		err = swapChain.queuePresent(queue, currentBuffer);
		assert(!err);

		vkDestroySemaphore(device, presentCompleteSemaphore, nullptr);

		submitPostPresentBarrier(swapChain.buffers[currentBuffer].image);

		err = vkQueueWaitIdle(queue);
		assert(err == VK_SUCCESS);
	}

	void loadMeshes()
	{
		loadMesh("./../data/models/glowsphere.X", &meshes.example, vertexLayout, 0.05f);
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

		VkResult vkRes = vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool);
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

		VkResult err = vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayout);
		assert(!err);

		VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo =
			vkTools::initializers::pipelineLayoutCreateInfo(
				&descriptorSetLayout,
				1);

		err = vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &pipelineLayouts.radialBlur);
		assert(!err);

		// Offscreen pipeline layout
		err = vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &pipelineLayouts.scene);
		assert(!err);
	}

	void setupDescriptorSet()
	{
		// Textured quad descriptor set
		VkDescriptorSetAllocateInfo allocInfo =
			vkTools::initializers::descriptorSetAllocateInfo(
				descriptorPool,
				&descriptorSetLayout,
				1);

		VkResult vkRes = vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.quad);
		assert(!vkRes);

		// Image descriptor for the color map texture
		VkDescriptorImageInfo texDescriptor =
			vkTools::initializers::descriptorImageInfo(
				offScreenFrameBuf.textureTarget.sampler,
				offScreenFrameBuf.textureTarget.view,
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
		vkRes = vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.scene);
		assert(!vkRes);

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

#ifdef USE_GLSL
		shaderStages[0] = loadShaderGLSL("./../data/shaders/radialblur/radialblur.vert", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShaderGLSL("./../data/shaders/radialblur/radialblur.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
#else
		shaderStages[0] = loadShader("./../data/shaders/radialblur/radialblur.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader("./../data/shaders/radialblur/radialblur.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
#endif

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

		VkResult err = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.radialBlur);
		assert(!err);

		// No blending (for debug display)
		blendAttachmentState.blendEnable = VK_FALSE;
		err = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.fullScreenOnly);
		assert(!err);

		// Phong pass
#ifdef USE_GLSL
		shaderStages[0] = loadShaderGLSL("./../data/shaders/radialblur/phongpass.vert", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShaderGLSL("./../data/shaders/radialblur/phongpass.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
#else
		shaderStages[0] = loadShader("./../data/shaders/radialblur/phongpass.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader("./../data/shaders/radialblur/phongpass.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
#endif

		pipelineCreateInfo.layout = pipelineLayouts.scene;
		blendAttachmentState.blendEnable = VK_FALSE;
		depthStencilState.depthWriteEnable = VK_TRUE;

		err = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.phongPass);
		assert(!err);

		// Color only pass (offscreen blur base)
#ifdef USE_GLSL
		shaderStages[0] = loadShaderGLSL("./../data/shaders/radialblur/colorpass.vert", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShaderGLSL("./../data/shaders/radialblur/colorpass.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
#else
		shaderStages[0] = loadShader("./../data/shaders/radialblur/colorpass.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader("./../data/shaders/radialblur/colorpass.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
#endif

		err = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.colorPass);
		assert(!err);

	}

	// Prepare and initialize uniform buffer containing shader uniforms
	void prepareUniformBuffers()
	{
		VkResult err;

		// Phong and color pass vertex shader uniform buffer
		createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			sizeof(uboVS),
			&uboVS,
			&uniformData.vsScene.buffer,
			&uniformData.vsScene.memory,
			&uniformData.vsScene.descriptor);

		// Fullscreen quad vertex shader uniform buffer
		createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			sizeof(uboVS),
			&uboVS,
			&uniformData.vsQuad.buffer,
			&uniformData.vsQuad.memory,
			&uniformData.vsQuad.descriptor);

		// Fullscreen quad fragment shader uniform buffer
		createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
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
		uboQuadVS.projection = glm::perspective(deg_to_rad(45.0f), (float)width / (float)height, 1.0f, 256.0f);
		glm::mat4 viewMatrix = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, zoom));

		uboQuadVS.model = glm::mat4();
		uboQuadVS.model = viewMatrix * glm::translate(uboQuadVS.model, glm::vec3(0, 0, 0));
		uboQuadVS.model = glm::rotate(uboQuadVS.model, deg_to_rad(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		uboQuadVS.model = glm::rotate(uboQuadVS.model, deg_to_rad(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		uboQuadVS.model = glm::rotate(uboQuadVS.model, deg_to_rad(timer * 360.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		uboQuadVS.model = glm::rotate(uboQuadVS.model, deg_to_rad(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

		uint8_t *pData;
		VkResult err = vkMapMemory(device, uniformData.vsQuad.memory, 0, sizeof(uboQuadVS), 0, (void **)&pData);
		assert(!err);
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
		VkResult err = vkMapMemory(device, uniformData.vsScene.memory, 0, sizeof(uboVS), 0, (void **)&pData);
		assert(!err);
		memcpy(pData, &uboVS, sizeof(uboVS));
		vkUnmapMemory(device, uniformData.vsScene.memory);

		// Fragment shader
		err = vkMapMemory(device, uniformData.fsQuad.memory, 0, sizeof(uboQuadFS), 0, (void **)&pData);
		assert(!err);
		memcpy(pData, &uboQuadFS, sizeof(uboQuadFS));
		vkUnmapMemory(device, uniformData.fsQuad.memory);
	}

	void prepare()
	{
		VulkanExampleBase::prepare();
		generateQuad();
		loadMeshes();
		setupVertexDescriptions();
		prepareUniformBuffers();
		prepareTextureTarget(&offScreenFrameBuf.textureTarget, TEX_DIM, TEX_DIM, TEX_FORMAT);
		setupDescriptorSetLayout();
		preparePipelines();
		setupDescriptorPool();
		setupDescriptorSet();
		createOffscreenCommandBuffer();
		prepareOffscreenFramebuffer();
		buildCommandBuffers();
		buildOffscreenCommandBuffer();
		prepared = true;
	}

	virtual void render()
	{
		if (!prepared)
			return;
		vkDeviceWaitIdle(device);
		draw();
		vkDeviceWaitIdle(device);
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

#ifdef _WIN32

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (vulkanExample != NULL)
	{
		vulkanExample->handleMessages(hWnd, uMsg, wParam, lParam);
		if (uMsg == WM_KEYDOWN)
		{
			switch (wParam)
			{
			case 0x42:
				vulkanExample->toggleBlur();
				break;
			case 0x54:
				vulkanExample->toggleTextureDisplay();
				break;
			}
		}
	}
	return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}

#else 

static void handleEvent(const xcb_generic_event_t *event)
{
	if (vulkanExample != NULL)
	{
		vulkanExample->handleEvent(event);
	}
}
#endif

#ifdef _WIN32
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
#else
int main(const int argc, const char *argv[])
#endif
{
	vulkanExample = new VulkanExample();
#ifdef _WIN32
	vulkanExample->setupWindow(hInstance, WndProc);
#else
	vulkanExample->setupWindow();
#endif
	vulkanExample->initSwapchain();
	vulkanExample->prepare();
	vulkanExample->renderLoop();
	delete(vulkanExample);
	return 0;
}