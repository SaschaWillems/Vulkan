/*
* Vulkan Example - Offscreen rendering using a separate framebuffer
*
* Copyright (C) 2015 by Sascha Willems - www.saschawillems.de
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
#include "vulkanMeshLoader.hpp"

#define VERTEX_BUFFER_BIND_ID 0
#define USE_GLSL
#define ENABLE_VALIDATION true

// Texture properties
#define TEX_DIM 256
#define TEX_FORMAT VK_FORMAT_D16_UNORM
#define TEX_FILTER VK_FILTER_LINEAR

// Offscreen frame buffer properties
#define FB_DIM TEX_DIM
#define FB_COLOR_FORMAT VK_FORMAT_R8G8B8A8_UNORM
#define FB_DEPTH_FORMAT TEX_FORMAT

class VulkanExample : public VulkanExampleBase
{
public:
	bool paused = false;
	bool displayShadowMap = true;
	float timer = 0;

	float zNear = 0.1f;
	float zFar = 48.0f;

	float depthBias = 0.0015f;

	VulkanMeshLoader *demoMesh;

	struct {
		VkBuffer buf;
		VkDeviceMemory mem;
		VkPipelineVertexInputStateCreateInfo inputState;
		std::vector<VkVertexInputBindingDescription> bindingDescriptions;
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
	} vertices;

	struct {
		int count;
		VkBuffer buf;
		VkDeviceMemory mem;
	} indices;

	vkTools::UniformData uniformDataVS, uniformDataOffscreenVS;

	struct {
		vkTools::UniformData scene;
	} uniformData;

	struct {
		glm::mat4 projection;
		glm::mat4 model;
	} uboVSquad;

	struct {
		glm::mat4 projection;
		glm::mat4 view;
		glm::mat4 model;
		glm::mat4 depthBiasMVP;
		glm::vec4 lightPos = glm::vec4(0.0f, -10.0f, 0.0f, 1.0f);
	} uboVSscene;

	struct {
		glm::mat4 depthMVP;
	} uboOffscreenVS;

	struct {
		VkPipeline quad;
		VkPipeline offscreen;
		VkPipeline scene;
	} pipelines;

	struct {
		VkPipelineLayout quad;
		VkPipelineLayout offscreen;
	} pipelineLayouts;

	struct {
		VkDescriptorSet offscreen;
		VkDescriptorSet scene;
	} descriptorSets;

	VkDescriptorSet descriptorSet;
	VkDescriptorSetLayout descriptorSetLayout;

	// Texture target for frame buffer blit
	struct Texture {
		VkImageLayout imageLayout;
		VkDeviceMemory deviceMemory;
		VkImage image;
		VkImageView view;
		VkSampler sampler, samplerCompare;
		int32_t width, height;
	} offScreenTex;

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
	} offScreenFrameBuf;

	VkCommandBuffer offScreenCmdBuffer = VK_NULL_HANDLE;
	VkCommandBuffer copyCmdBuffer = VK_NULL_HANDLE;

	VulkanExample() : VulkanExampleBase(ENABLE_VALIDATION)
	{
		zoom = -20.0f;
		rotation = { -22.5f, -218.0f, 0.0f };
		title = "Vulkan Example - Shadow mapping";
		if (ENABLE_VALIDATION) {
			setupConsole("VulkanExample");
		}
	}

	~VulkanExample()
	{
		// Clean up used Vulkan resources 
		// Note : Inherited destructor cleans up resources stored in base class

		// Texture target
		vkDestroyImageView(device, offScreenTex.view, nullptr);
		vkDestroyImage(device, offScreenTex.image, nullptr);
		vkDestroySampler(device, offScreenTex.sampler, nullptr);
		vkFreeMemory(device, offScreenTex.deviceMemory, nullptr);

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

		vkDestroyPipeline(device, pipelines.quad, nullptr);
		vkDestroyPipeline(device, pipelines.offscreen, nullptr);

		vkDestroyPipelineLayout(device, pipelineLayouts.quad, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayouts.offscreen, nullptr);

		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

		// Quad
		vkDestroyBuffer(device, vertices.buf, nullptr);
		vkFreeMemory(device, vertices.mem, nullptr);
		vkDestroyBuffer(device, indices.buf, nullptr);
		vkFreeMemory(device, indices.mem, nullptr);

		// Mesh
		VulkanMeshLoader::freeVulkanResources(device, demoMesh);

		// Uniform buffers
		vkDestroyBuffer(device, uniformDataVS.buffer, nullptr);
		vkFreeMemory(device, uniformDataVS.memory, nullptr);
		vkDestroyBuffer(device, uniformDataOffscreenVS.buffer, nullptr);
		vkFreeMemory(device, uniformDataOffscreenVS.memory, nullptr);

		vkFreeCommandBuffers(device, cmdPool, 1, &offScreenCmdBuffer);
		vkFreeCommandBuffers(device, cmdPool, 1, &copyCmdBuffer);

		delete(demoMesh);
	}

	// Preapre an empty texture as the blit target from 
	// the offscreen framebuffer
	void prepareTextureTarget(int32_t width, int32_t height, VkFormat format)
	{
		createSetupCommandBuffer();

		VkResult err;

		// Get device properites for the requested texture format
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);
		// Check if format is supported for optimal tiling
		assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

		// Prepare blit target texture
		offScreenTex.width = width;
		offScreenTex.height = height;

		VkImageCreateInfo imageCreateInfo = vkTools::initializers::imageCreateInfo();
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.format = format;
		imageCreateInfo.extent = { width, height, 1 };
		imageCreateInfo.mipLevels = 1;
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageCreateInfo.flags = 0;
		imageCreateInfo.pQueueFamilyIndices = 0;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VkMemoryAllocateInfo memAllocInfo = vkTools::initializers::memoryAllocateInfo();
		VkMemoryRequirements memReqs;

		err = vkCreateImage(device, &imageCreateInfo, nullptr, &offScreenTex.image);
		assert(!err);
		vkGetImageMemoryRequirements(device, offScreenTex.image, &memReqs);
		memAllocInfo.allocationSize = memReqs.size;
		assert(getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memAllocInfo.memoryTypeIndex));
		err = vkAllocateMemory(device, &memAllocInfo, nullptr, &offScreenTex.deviceMemory);
		assert(!err);
		err = vkBindImageMemory(device, offScreenTex.image, offScreenTex.deviceMemory, 0);
		assert(!err);

		offScreenTex.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		vkTools::setImageLayout(
			setupCmdBuffer,
			offScreenTex.image,
			VK_IMAGE_ASPECT_DEPTH_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			offScreenTex.imageLayout);

		// Create sampler
		VkSamplerCreateInfo sampler = vkTools::initializers::samplerCreateInfo();
		sampler.magFilter = TEX_FILTER;
		sampler.minFilter = TEX_FILTER;
		sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_BASE;
		sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler.addressModeV = sampler.addressModeU;
		sampler.addressModeW = sampler.addressModeU;
		sampler.mipLodBias = 0.0f;
		sampler.maxAnisotropy = 0;
		sampler.minLod = 0.0f;
		sampler.maxLod = 0.0f;
		sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		err = vkCreateSampler(device, &sampler, nullptr, &offScreenTex.sampler);
		assert(!err);

		sampler.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		sampler.compareEnable = VK_TRUE;
		err = vkCreateSampler(device, &sampler, nullptr, &offScreenTex.samplerCompare);
		assert(!err);

		vkTools::setImageLayout(
			setupCmdBuffer,
			offScreenTex.image,
			VK_IMAGE_ASPECT_DEPTH_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		flushSetupCommandBuffer();

		// Create image view
		VkImageViewCreateInfo view = {};
		view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view.pNext = NULL;
		view.viewType = VK_IMAGE_VIEW_TYPE_2D;
		view.format = format;
		view.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		view.subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };
		view.image = offScreenTex.image;
		err = vkCreateImageView(device, &view, nullptr, &offScreenTex.view);
		assert(!err);

	}

	// Prepare a new framebuffer for offscreen rendering
	// The contents of this framebuffer are then
	// blitted to our render target
	void prepareOffscreenFramebuffer()
	{
		offScreenFrameBuf.width = FB_DIM;
		offScreenFrameBuf.height = FB_DIM;

		VkFormat fbColorFormat = FB_COLOR_FORMAT;
		VkFormat fbDepthFormat = FB_DEPTH_FORMAT;

		VkResult err;

		// Get device properites for the requested texture format
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, fbDepthFormat, &formatProperties);
		// Check if format is supported for optimal tiling
		assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

		createSetupCommandBuffer();

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
		// Image of the framebuffer is blit source
		image.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		image.flags = 0;

		VkMemoryAllocateInfo memAlloc = vkTools::initializers::memoryAllocateInfo();

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

		VkMemoryRequirements memReqs;

		err = vkCreateImage(device, &image, nullptr, &offScreenFrameBuf.color.image);
		assert(!err);
		vkGetImageMemoryRequirements(device, offScreenFrameBuf.color.image, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		assert(getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memAlloc.memoryTypeIndex));
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
		image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		image.initialLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

		VkImageViewCreateInfo depthStencilView = vkTools::initializers::imageViewCreateInfo();
		depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
		depthStencilView.format = fbDepthFormat;
		depthStencilView.flags = 0;
		depthStencilView.subresourceRange = {};
		depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		depthStencilView.subresourceRange.baseMipLevel = 0;
		depthStencilView.subresourceRange.levelCount = 1;
		depthStencilView.subresourceRange.baseArrayLayer = 0;
		depthStencilView.subresourceRange.layerCount = 1;

		err = vkCreateImage(device, &image, nullptr, &offScreenFrameBuf.depth.image);
		assert(!err);
		vkGetImageMemoryRequirements(device, offScreenFrameBuf.depth.image, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		assert(getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memAlloc.memoryTypeIndex));
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

		flushSetupCommandBuffer();

		depthStencilView.image = offScreenFrameBuf.depth.image;
		err = vkCreateImageView(device, &depthStencilView, nullptr, &offScreenFrameBuf.depth.view);
		assert(!err);

		VkImageView attachments[2];
		attachments[0] = offScreenFrameBuf.color.view;
		attachments[1] = offScreenFrameBuf.depth.view;

		VkFramebufferCreateInfo fbufCreateInfo = {};
		fbufCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fbufCreateInfo.pNext = NULL;
		fbufCreateInfo.renderPass = renderPass;
		fbufCreateInfo.attachmentCount = 2;
		fbufCreateInfo.pAttachments = attachments;
		fbufCreateInfo.width = offScreenFrameBuf.width;
		fbufCreateInfo.height = offScreenFrameBuf.height;
		fbufCreateInfo.layers = 1;

		err = vkCreateFramebuffer(device, &fbufCreateInfo, nullptr, &offScreenFrameBuf.frameBuffer);
		assert(!err);
	}

	// The command buffer to copy for rendering 
	// the offscreen scene and blitting it into
	// the texture target is only build once
	// and gets resubmitted 
	void buildOffscreenCommandBuffer()
	{
		VkResult err;

		// Create separate command buffer for offscreen 
		// rendering
		if (offScreenCmdBuffer == VK_NULL_HANDLE)
		{
			VkCommandBufferAllocateInfo cmd = vkTools::initializers::commandBufferAllocateInfo(
				cmdPool,
				VK_COMMAND_BUFFER_LEVEL_PRIMARY,
				1);
			VkResult vkRes = vkAllocateCommandBuffers(device, &cmd, &offScreenCmdBuffer);
			assert(!vkRes);
			vkRes = vkAllocateCommandBuffers(device, &cmd, &copyCmdBuffer);
			assert(!vkRes);
		}

		VkCommandBufferBeginInfo cmdBufInfo = vkTools::initializers::commandBufferBeginInfo();

		VkClearValue clearValues[2];
		clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderPassBeginInfo = {};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.pNext = NULL;
		renderPassBeginInfo.renderPass = renderPass;
		renderPassBeginInfo.framebuffer = offScreenFrameBuf.frameBuffer;
		renderPassBeginInfo.renderArea.offset.x = 0;
		renderPassBeginInfo.renderArea.offset.y = 0;
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
		vkCmdSetViewport(offScreenCmdBuffer, 1, &viewport);

		VkRect2D scissor = vkTools::initializers::rect2D(
			offScreenFrameBuf.width,
			offScreenFrameBuf.height,
			0,
			0);
		vkCmdSetScissor(offScreenCmdBuffer, 1, &scissor);

		// Depth bias (aka "Polygon offset") - I just hope this works
		vkCmdSetDepthBias(			offScreenCmdBuffer,			depthBias,			0.5f,			1.0f/depthBias);

		vkCmdBeginRenderPass(offScreenCmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindDescriptorSets(offScreenCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.offscreen, 0, 1, &descriptorSets.offscreen, 0, NULL);

		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(offScreenCmdBuffer, VERTEX_BUFFER_BIND_ID, 1, &demoMesh->vertexBuffer.buf, offsets);
		vkCmdBindIndexBuffer(offScreenCmdBuffer, demoMesh->indexBuffer.buf, 0, VK_INDEX_TYPE_UINT32);

		vkCmdBindPipeline(offScreenCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.offscreen);
		vkCmdDrawIndexed(offScreenCmdBuffer, demoMesh->indexBuffer.count, 1, 0, 0, 0);

		vkCmdEndRenderPass(offScreenCmdBuffer);

		err = vkEndCommandBuffer(offScreenCmdBuffer);
		assert(!err);

		err = vkBeginCommandBuffer(copyCmdBuffer, &cmdBufInfo);
		assert(!err);

		updateTexture();

		err = vkEndCommandBuffer(copyCmdBuffer);
		assert(!err);

	}

	void buildCommandBuffers()
	{
		VkCommandBufferBeginInfo cmdBufInfo = vkTools::initializers::commandBufferBeginInfo();

		VkClearValue clearValues[2];
		clearValues[0].color = { { 0.0f, 0.0f, 0.2f, 0.0f } };
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
			vkCmdSetViewport(drawCmdBuffers[i], 1, &viewport);

			VkRect2D scissor = vkTools::initializers::rect2D(
				width,
				height,
				0,
				0);
			vkCmdSetScissor(drawCmdBuffers[i], 1, &scissor);

			VkDeviceSize offsets[1] = { 0 };

			vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.quad, 0, 1, &descriptorSet, 0, NULL);
			vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.quad);

			// Visualize shadow map
			if (displayShadowMap)
			{
				vkCmdBindVertexBuffers(drawCmdBuffers[i], VERTEX_BUFFER_BIND_ID, 1, &vertices.buf, offsets);
				vkCmdBindIndexBuffer(drawCmdBuffers[i], indices.buf, 0, VK_INDEX_TYPE_UINT32);
				vkCmdDrawIndexed(drawCmdBuffers[i], indices.count, 1, 0, 0, 0);
			}

			// 3D scene
			vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.quad, 0, 1, &descriptorSets.scene, 0, NULL);
			vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.scene);

			vkCmdBindVertexBuffers(drawCmdBuffers[i], VERTEX_BUFFER_BIND_ID, 1, &demoMesh->vertexBuffer.buf, offsets);
			vkCmdBindIndexBuffer(drawCmdBuffers[i], demoMesh->indexBuffer.buf, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(drawCmdBuffers[i], demoMesh->indexBuffer.count, 1, 0, 0, 0);

			vkCmdEndRenderPass(drawCmdBuffers[i]);

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
			drawCmdBuffers[currentBuffer],
			offScreenCmdBuffer,
			copyCmdBuffer
		};
		
		VkSubmitInfo submitInfo = vkTools::initializers::submitInfo();
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &presentCompleteSemaphore;
		submitInfo.commandBufferCount = submitCmdBuffers.size();
		submitInfo.pCommandBuffers = submitCmdBuffers.data();

		err = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
		assert(!err);

		err = swapChain.queuePresent(queue, currentBuffer);
		assert(!err);

		vkDestroySemaphore(device, presentCompleteSemaphore, nullptr);

		err = vkQueueWaitIdle(queue);
		assert(err == VK_SUCCESS);
	}

	void prepareVertices()
	{
		struct Vertex {
			float pos[3];
			float uv[2];
			float col[3];
			float normal[3];
		};

		// Setup vertices for a single uv-mapped quad
#define dim 1.0f
#define quadcol { 1.0f, 1.0f, 1.0f }
#define quadnormal { 0.0f, 0.0f, 1.0f }
		std::vector<Vertex> vertexBuffer =
		{
			{ { dim, 0.0f, 0.0f },{ 1.0f, 0.0f }, quadcol, quadnormal },
		{ { 0.0f, 0.0f, 0.0f },{ 0.0f, 0.0f }, quadcol, quadnormal },
			{ { 0.0f,  dim, 0.0f },{ 0.0f, 1.0f }, quadcol, quadnormal },
			{ { dim,  dim, 0.0f },{ 1.0f, 1.0f }, quadcol, quadnormal }
		};
#undef dim
#undef quadcol
#undef quadnormal
		int vertexBufferSize = vertexBuffer.size() * sizeof(Vertex);

		// Setup indices
		std::vector<uint32_t> indexBuffer = { 0,1,2, 2,3,0 };
		int indexBufferSize = indexBuffer.size() * sizeof(uint32_t);

		VkMemoryAllocateInfo memAlloc = vkTools::initializers::memoryAllocateInfo();
		VkMemoryRequirements memReqs;

		VkResult err;
		void *data;

		// Generate vertex buffer
		VkBufferCreateInfo vBufferInfo = vkTools::initializers::bufferCreateInfo(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertexBufferSize);
		err = vkCreateBuffer(device, &vBufferInfo, nullptr, &vertices.buf);
		assert(!err);
		vkGetBufferMemoryRequirements(device, vertices.buf, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		assert(getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memAlloc.memoryTypeIndex));
		err = vkAllocateMemory(device, &memAlloc, nullptr, &vertices.mem);
		assert(!err);
		err = vkMapMemory(device, vertices.mem, 0, vertexBufferSize, 0, &data);
		assert(!err);
		memcpy(data, vertexBuffer.data(), vertexBufferSize);
		vkUnmapMemory(device, vertices.mem);
		err = vkBindBufferMemory(device, vertices.buf, vertices.mem, 0);
		assert(!err);

		// Generate index buffer
		VkBufferCreateInfo iBufferInfo = vkTools::initializers::bufferCreateInfo(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, indexBufferSize);
		err = vkCreateBuffer(device, &iBufferInfo, nullptr, &indices.buf);
		assert(!err);
		vkGetBufferMemoryRequirements(device, indices.buf, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		assert(getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memAlloc.memoryTypeIndex));
		err = vkAllocateMemory(device, &memAlloc, nullptr, &indices.mem);
		assert(!err);
		err = vkMapMemory(device, indices.mem, 0, indexBufferSize, 0, &data);
		assert(!err);
		memcpy(data, indexBuffer.data(), indexBufferSize);
		vkUnmapMemory(device, indices.mem);
		err = vkBindBufferMemory(device, indices.buf, indices.mem, 0);
		assert(!err);
		indices.count = indexBuffer.size();

		// Example mesh

		demoMesh = new VulkanMeshLoader();
		demoMesh->LoadMesh("./../data/models/shadowscene_omni.X");

		float scale = 0.25f;
		vertexBuffer.clear();
		for (int m = 0; m < demoMesh->m_Entries.size(); m++)
		{
			for (int i = 0; i < demoMesh->m_Entries[m].Vertices.size(); i++)
			{
				glm::vec3 pos = demoMesh->m_Entries[m].Vertices[i].m_pos * scale;
				glm::vec3 normal = demoMesh->m_Entries[m].Vertices[i].m_normal;
				glm::vec2 uv = demoMesh->m_Entries[m].Vertices[i].m_tex;
				glm::vec3 col = demoMesh->m_Entries[m].Vertices[i].m_color;
				Vertex vert =
				{
					{ pos.x, pos.y, pos.z }, 
					{ uv.s, uv.t },
					{ col.r, col.g, col.b },
					{ normal.x, -normal.y, normal.z }
				};
				vertexBuffer.push_back(vert);
			}
		}
		vertexBufferSize = vertexBuffer.size() * sizeof(Vertex);

		indexBuffer.clear();
		for (int m = 0; m < demoMesh->m_Entries.size(); m++)
		{
			int indexBase = indexBuffer.size();
			for (int i = 0; i < demoMesh->m_Entries[m].Indices.size(); i++) {
				indexBuffer.push_back(demoMesh->m_Entries[m].Indices[i] + indexBase);
			}
		}
		indexBufferSize = indexBuffer.size() * sizeof(UINT32);

		// Generate vertex buffer
		vBufferInfo = vkTools::initializers::bufferCreateInfo(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertexBufferSize);
		err = vkCreateBuffer(device, &vBufferInfo, nullptr, &demoMesh->vertexBuffer.buf);
		assert(!err);
		vkGetBufferMemoryRequirements(device, demoMesh->vertexBuffer.buf, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		assert(getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memAlloc.memoryTypeIndex));
		err = vkAllocateMemory(device, &memAlloc, nullptr, &demoMesh->vertexBuffer.mem);
		assert(!err);
		err = vkMapMemory(device, demoMesh->vertexBuffer.mem, 0, vertexBufferSize, 0, &data);
		assert(!err);
		memcpy(data, vertexBuffer.data(), vertexBufferSize);
		vkUnmapMemory(device, demoMesh->vertexBuffer.mem);
		err = vkBindBufferMemory(device, demoMesh->vertexBuffer.buf, demoMesh->vertexBuffer.mem, 0);
		assert(!err);

		// Generate index buffer
		iBufferInfo = vkTools::initializers::bufferCreateInfo(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, indexBufferSize);
		err = vkCreateBuffer(device, &iBufferInfo, nullptr, &demoMesh->indexBuffer.buf);
		assert(!err);
		vkGetBufferMemoryRequirements(device, demoMesh->indexBuffer.buf, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		assert(getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memAlloc.memoryTypeIndex));
		err = vkAllocateMemory(device, &memAlloc, nullptr, &demoMesh->indexBuffer.mem);
		assert(!err);
		err = vkMapMemory(device, demoMesh->indexBuffer.mem, 0, indexBufferSize, 0, &data);
		assert(!err);
		memcpy(data, indexBuffer.data(), indexBufferSize);
		vkUnmapMemory(device, demoMesh->indexBuffer.mem);
		err = vkBindBufferMemory(device, demoMesh->indexBuffer.buf, demoMesh->indexBuffer.mem, 0);
		assert(!err);
		demoMesh->indexBuffer.count = indexBuffer.size();

		// Binding description
		vertices.bindingDescriptions.resize(1);
		vertices.bindingDescriptions[0] =
			vkTools::initializers::vertexInputBindingDescription(
				VERTEX_BUFFER_BIND_ID,
				sizeof(Vertex),
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
		// Example uses three ubos and two image samplers
		std::vector<VkDescriptorPoolSize> poolSizes =
		{
			vkTools::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3),
			vkTools::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2)
		};

		VkDescriptorPoolCreateInfo descriptorPoolInfo =
			vkTools::initializers::descriptorPoolCreateInfo(
				poolSizes.size(),
				poolSizes.data(),
				3);

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
				1)
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

		err = vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &pipelineLayouts.quad);
		assert(!err);

		// Offscreen pipeline layout
		// TODO : Actually the same as the normal one...
		err = vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &pipelineLayouts.offscreen);
		assert(!err);
	}

	void setupDescriptorSets()
	{
		// Textured quad descriptor set
		VkDescriptorSetAllocateInfo allocInfo =
			vkTools::initializers::descriptorSetAllocateInfo(
				descriptorPool,
				&descriptorSetLayout,
				1);

		VkResult vkRes = vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet);
		assert(!vkRes);

		// Image descriptor for the shadow map texture
		VkDescriptorImageInfo texDescriptor =
			vkTools::initializers::descriptorImageInfo(
				offScreenTex.sampler,
				offScreenTex.view,
				VK_IMAGE_LAYOUT_GENERAL);

		std::vector<VkWriteDescriptorSet> writeDescriptorSets =
		{
			// Binding 0 : Vertex shader uniform buffer
			vkTools::initializers::writeDescriptorSet(
			descriptorSet,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				0,
				&uniformDataVS.descriptor),
			// Binding 1 : Fragment shader texture sampler
			vkTools::initializers::writeDescriptorSet(
				descriptorSet,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				1,
				&texDescriptor)
		};

		vkUpdateDescriptorSets(device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);

		// Offscreen
		vkRes = vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.offscreen);
		assert(!vkRes);

		std::vector<VkWriteDescriptorSet> offScreenWriteDescriptorSets =
		{
			// Binding 0 : Vertex shader uniform buffer
			vkTools::initializers::writeDescriptorSet(
				descriptorSets.offscreen,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				0,
				&uniformDataOffscreenVS.descriptor),
		};
		vkUpdateDescriptorSets(device, offScreenWriteDescriptorSets.size(), offScreenWriteDescriptorSets.data(), 0, NULL);

		// 3D scene
		vkRes = vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.scene);
		assert(!vkRes);

		// Image descriptor for the shadow map texture
		texDescriptor.sampler = offScreenTex.samplerCompare;

		std::vector<VkWriteDescriptorSet> sceneDescriptorSets =
		{
			// Binding 0 : Vertex shader uniform buffer
			vkTools::initializers::writeDescriptorSet(
				descriptorSets.scene,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				0,
				&uniformData.scene.descriptor),
			// Binding 1 : Fragment shader shadow sampler
			vkTools::initializers::writeDescriptorSet(
				descriptorSets.scene,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				1,
				&texDescriptor)
		};
		vkUpdateDescriptorSets(device, sceneDescriptorSets.size(), sceneDescriptorSets.data(), 0, NULL);

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
				VK_CULL_MODE_BACK_BIT,
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

		// Solid rendering pipeline
		// Load shaders
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

#ifdef USE_GLSL
		shaderStages[0] = loadShaderGLSL("./../data/shaders/shadowmap/quad.vert", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShaderGLSL("./../data/shaders/shadowmap/quad.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
#else
		shaderStages[0] = loadShader("./../data/shaders/shadowmap/quad.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader("./../data/shaders/shadowmap/quad.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
#endif

		VkGraphicsPipelineCreateInfo pipelineCreateInfo =
			vkTools::initializers::pipelineCreateInfo(
				pipelineLayouts.quad,
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

		VkResult err = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.quad);
		assert(!err);

		// 3D scene
#ifdef USE_GLSL
		shaderStages[0] = loadShaderGLSL("./../data/shaders/shadowmap/scene.vert", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShaderGLSL("./../data/shaders/shadowmap/scene.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
#else
		shaderStages[0] = loadShader("./../data/shaders/shadowmap/scene.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader("./../data/shaders/shadowmap/scene.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
#endif

		err = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.scene);
		assert(!err);

		// Offscreen pipeline
#ifdef USE_GLSL
		shaderStages[0] = loadShaderGLSL("./../data/shaders/shadowmap/offscreen.vert", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShaderGLSL("./../data/shaders/shadowmap/offscreen.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
#else
		shaderStages[0] = loadShader("./../data/shaders/shadowmap/offscreen.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader("./../data/shaders/shadowmap/offscreen.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
#endif

		pipelineCreateInfo.layout = pipelineLayouts.offscreen;
		depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;
		// Cull front faces
		rasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;
		//rasterizationState.cullMode = VK_CULL_MODE_NONE;
		rasterizationState.depthBiasEnable = VK_TRUE;
		rasterizationState.depthBiasClamp = VK_TRUE;

		dynamicStateEnables.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);

		dynamicState =
			vkTools::initializers::pipelineDynamicStateCreateInfo(
				dynamicStateEnables.data(),
				dynamicStateEnables.size(),
				0);
		
		err = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.offscreen);
		assert(!err);
	}

	// Prepare and initialize uniform buffer containing shader uniforms
	void prepareUniformBuffers()
	{
		VkResult err;

		// Vertex shader uniform buffer block
		VkMemoryAllocateInfo allocInfo = vkTools::initializers::memoryAllocateInfo();
		VkMemoryRequirements memReqs;

		VkBufferCreateInfo bufferInfo = vkTools::initializers::bufferCreateInfo(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			sizeof(uboVSquad));

		err = vkCreateBuffer(device, &bufferInfo, nullptr, &uniformDataVS.buffer);
		assert(!err);
		vkGetBufferMemoryRequirements(device, uniformDataVS.buffer, &memReqs);
		allocInfo.allocationSize = memReqs.size;
		assert(getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &allocInfo.memoryTypeIndex));
		err = vkAllocateMemory(device, &allocInfo, nullptr, &uniformDataVS.memory);
		assert(!err);
		err = vkBindBufferMemory(device, uniformDataVS.buffer, uniformDataVS.memory, 0);
		assert(!err);

		uniformDataVS.descriptor.buffer = uniformDataVS.buffer;
		uniformDataVS.descriptor.offset = 0;
		uniformDataVS.descriptor.range = sizeof(uboVSquad);

		// Offscreen vertex shader uniform buffer block 
		bufferInfo.size = sizeof(uboOffscreenVS);

		err = vkCreateBuffer(device, &bufferInfo, nullptr, &uniformDataOffscreenVS.buffer);
		assert(!err);
		vkGetBufferMemoryRequirements(device, uniformDataOffscreenVS.buffer, &memReqs);
		allocInfo.allocationSize = memReqs.size;
		assert(getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &allocInfo.memoryTypeIndex));
		err = vkAllocateMemory(device, &allocInfo, nullptr, &uniformDataOffscreenVS.memory);
		assert(!err);
		err = vkBindBufferMemory(device, uniformDataOffscreenVS.buffer, uniformDataOffscreenVS.memory, 0);
		assert(!err);

		uniformDataOffscreenVS.descriptor.buffer = uniformDataOffscreenVS.buffer;
		uniformDataOffscreenVS.descriptor.offset = 0;
		uniformDataOffscreenVS.descriptor.range = sizeof(uboOffscreenVS);

		// 3D scene
		bufferInfo.size = sizeof(uboVSscene);

		err = vkCreateBuffer(device, &bufferInfo, nullptr, &uniformData.scene.buffer);
		assert(!err);
		vkGetBufferMemoryRequirements(device, uniformData.scene.buffer, &memReqs);
		allocInfo.allocationSize = memReqs.size;
		assert(getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &allocInfo.memoryTypeIndex));
		err = vkAllocateMemory(device, &allocInfo, nullptr, &uniformData.scene.memory);
		assert(!err);
		err = vkBindBufferMemory(device, uniformData.scene.buffer, uniformData.scene.memory, 0);
		assert(!err);

		uniformData.scene.descriptor.buffer = uniformData.scene.buffer;
		uniformData.scene.descriptor.offset = 0;
		uniformData.scene.descriptor.range = sizeof(uboVSscene);

		updateUniformBufferOffscreen();
		updateUniformBuffers();
	}

	void updateUniformBuffers()
	{
		// Shadow map debug quad
		float AR = (float)height / (float)width;

		uboVSquad.projection = glm::ortho(0.0f, 2.5f / AR, 0.0f, 2.5f, -1.0f, 1.0f);
//		uboVSquad.projection = glm::ortho(0.0f, 1.0f, 0.0f, 1.0f, -1.0f, 1.0f);
		uboVSquad.model = glm::mat4();

		uint8_t *pData;
		VkResult err = vkMapMemory(device, uniformDataVS.memory, 0, sizeof(uboVSquad), 0, (void **)&pData);
		assert(!err);
		memcpy(pData, &uboVSquad, sizeof(uboVSquad));
		vkUnmapMemory(device, uniformDataVS.memory);

		// 3D scene
		uboVSscene.projection = glm::perspective(deg_to_rad(45.0f), (float)width / (float)height, zNear, zFar);
		uboVSscene.view = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, zoom));

		uboVSscene.model = glm::mat4();
		uboVSscene.model = glm::rotate(uboVSscene.model, deg_to_rad(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		uboVSscene.model = glm::rotate(uboVSscene.model, deg_to_rad(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		uboVSscene.model = glm::rotate(uboVSscene.model, deg_to_rad(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

		glm::mat4 biasMat = glm::mat4(
			glm::vec4(0.5f, 0.0f, 0.0f, 0.0f),
			glm::vec4(0.0f, 0.5f, 0.0f, 0.0f), 
			glm::vec4(0.0f, 0.0f, 0.5f, 0.0f),
			glm::vec4(0.5f, 0.5f, 0.5f, 1.0f)
			);

		uboVSscene.depthBiasMVP = biasMat * uboOffscreenVS.depthMVP;

		pData;
		err = vkMapMemory(device, uniformData.scene.memory, 0, sizeof(uboVSscene), 0, (void **)&pData);
		assert(!err);
		memcpy(pData, &uboVSscene, sizeof(uboVSscene));
		vkUnmapMemory(device, uniformData.scene.memory);
	}

	void updateUniformBufferOffscreen()
	{
		// Matrix from light's point of view
		glm::vec3 lightInvDir = glm::vec3(0.5f, -2, 2);

		glm::mat4 depthProjectionMatrix = glm::ortho<float>(-10, 10, -10, 10, -50, 30);
//		glm::mat4 depthProjectionMatrix = glm::ortho<float>(-10, 10, -10, 10, -10, 20);
		glm::mat4 depthViewMatrix = glm::lookAt(lightInvDir, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
		glm::mat4 depthModelMatrix = glm::mat4();

				uboOffscreenVS.depthMVP =  depthProjectionMatrix * depthViewMatrix * depthModelMatrix;

		uint8_t *pData;
		VkResult err = vkMapMemory(device, uniformDataOffscreenVS.memory, 0, sizeof(uboOffscreenVS), 0, (void **)&pData);
		assert(!err);
		memcpy(pData, &uboOffscreenVS, sizeof(uboOffscreenVS));
		vkUnmapMemory(device, uniformDataOffscreenVS.memory);
	}

	// Blits the contents of the offscreen framebuffer to
	// our texture target
	void updateTexture()
	{
		// Make sure depth writes to the offscreen buffer are finished
		VkImageMemoryBarrier imageBarrier = vkTools::initializers::imageMemoryBarrier();
		imageBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageBarrier.subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1 };
		imageBarrier.image = offScreenFrameBuf.depth.image;
		VkImageMemoryBarrier *preBarrier = &imageBarrier;
		vkCmdPipelineBarrier(
			copyCmdBuffer,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_FALSE, 1, (const void * const*)&preBarrier);

		VkImageCopy imgCopy = {};

		imgCopy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		imgCopy.srcSubresource.mipLevel = 0;
		imgCopy.srcSubresource.baseArrayLayer = 0;
		imgCopy.srcSubresource.layerCount = 1;

		imgCopy.srcOffset = { 0, 0, 0 };

		imgCopy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		imgCopy.dstSubresource.mipLevel = 0;
		imgCopy.dstSubresource.baseArrayLayer = 0;
		imgCopy.dstSubresource.layerCount = 1;

		imgCopy.dstOffset = { 0, 0, 0 };

		imgCopy.extent.width = TEX_DIM;
		imgCopy.extent.height = TEX_DIM;
		imgCopy.extent.depth = 1;

		vkCmdCopyImage(
			copyCmdBuffer,
			offScreenFrameBuf.depth.image,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			offScreenTex.image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&imgCopy);

		// Make sure transfer is finished
		imageBarrier = vkTools::initializers::imageMemoryBarrier();
		imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		imageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageBarrier.subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };
		imageBarrier.image = offScreenTex.image;
		VkImageMemoryBarrier *postBarrier = &imageBarrier;
		vkCmdPipelineBarrier(
			copyCmdBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
			0, 1, (const void * const*)&postBarrier);
	}

	void prepare()
	{
		VulkanExampleBase::prepare();
		prepareVertices();
		prepareUniformBuffers();
		prepareTextureTarget(TEX_DIM, TEX_DIM, TEX_FORMAT);
		setupDescriptorSetLayout();
		preparePipelines();
		setupDescriptorPool();
		setupDescriptorSets();
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
			timer += 0.0015f; // TODO : Time based
			if (timer > 1.0)
			{
				timer -= 1.0f;
			}
			updateUniformBufferOffscreen();
			updateUniformBuffers();
		}
	}

	virtual void viewChanged()
	{
		updateUniformBufferOffscreen();
		updateUniformBuffers();
	}

	void changeDepthBias(float delta)
	{
		depthBias += delta;
		buildOffscreenCommandBuffer();
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
			case 0x50:
				vulkanExample->paused = !vulkanExample->paused;
				break;
			case 0x53:
				vulkanExample->displayShadowMap = !vulkanExample->displayShadowMap;
				break;
			case VK_ADD:
				vulkanExample->changeDepthBias(0.000025f);
				break;
			case VK_SUBTRACT:
				vulkanExample->changeDepthBias(-0.000025f);
				break;
			}
		}
	}
	return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}

#else 

static void handle_event(const xcb_generic_event_t *event)
{
	if (vulkanExample != NULL)
	{
		vulkanExample->handle_event(event);
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
	vulkanExample->setupWindow(hInstance, WndProc, false);
#else
	vulkanExample->setupWindow();
#endif
	vulkanExample->initSwapchain();
	vulkanExample->prepare();
	vulkanExample->renderLoop();
	delete(vulkanExample);
	return 0;
}