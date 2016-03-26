/*
* Vulkan Example - Offscreen rendering using a separate framebuffer
*
*	p - Toggle light source animation
*	l - Toggle between scene and light's POV
*	s - Toggle shadowmap display
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
#include "vulkanMeshLoader.hpp"

#define VERTEX_BUFFER_BIND_ID 0
#define ENABLE_VALIDATION false

// 16 bits of depth is enough for such a small scene
#define DEPTH_FORMAT VK_FORMAT_D16_UNORM

// Texture properties
#define TEX_DIM 2048
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

class VulkanExample : public VulkanExampleBase
{
public:
	bool displayShadowMap = false;
	bool lightPOV = false;

	// Keep depth range as small as possible
	// for better shadow map precision
	float zNear = 1.0f;
	float zFar = 96.0f;

	// Constant depth bias factor (always applied)
	float depthBiasConstant = 1.25f;
	// Slope depth bias factor, applied depending on polygon's slope
	float depthBiasSlope = 1.75f;

	glm::vec3 lightPos = glm::vec3();
	float lightFOV = 45.0f;

	struct {
		vkMeshLoader::MeshBuffer scene;
		vkMeshLoader::MeshBuffer quad;
	} meshes;

	struct {
		VkPipelineVertexInputStateCreateInfo inputState;
		std::vector<VkVertexInputBindingDescription> bindingDescriptions;
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
	} vertices;

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
		glm::vec3 lightPos;
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
		VkRenderPass renderPass;
		vkTools::VulkanTexture textureTarget;
	} offScreenFrameBuf;

	VkCommandBuffer offScreenCmdBuffer = VK_NULL_HANDLE;

	VulkanExample() : VulkanExampleBase(ENABLE_VALIDATION)
	{
		zoom = -20.0f;
		rotation = { -15.0f, -390.0f, 0.0f };
		title = "Vulkan Example - Projected shadow mapping";
		timerSpeed *= 0.5f;
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

		vkDestroyRenderPass(device, offScreenFrameBuf.renderPass, nullptr);

		vkDestroyPipeline(device, pipelines.quad, nullptr);
		vkDestroyPipeline(device, pipelines.offscreen, nullptr);
		vkDestroyPipeline(device, pipelines.scene, nullptr);

		vkDestroyPipelineLayout(device, pipelineLayouts.quad, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayouts.offscreen, nullptr);

		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

		// Meshes
		vkMeshLoader::freeMeshBufferResources(device, &meshes.scene);
		vkMeshLoader::freeMeshBufferResources(device, &meshes.quad);

		// Uniform buffers
		vkTools::destroyUniformData(device, &uniformDataVS);
		vkTools::destroyUniformData(device, &uniformDataOffscreenVS);

		vkFreeCommandBuffers(device, cmdPool, 1, &offScreenCmdBuffer);
	}

	// Preapre an empty texture as the blit target from 
	// the offscreen framebuffer
	void prepareTextureTarget(uint32_t width, uint32_t height, VkFormat format)
	{
		createSetupCommandBuffer();

		VkResult err;

		// Get device properites for the requested texture format
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);
		// Check if format is supported for optimal tiling
		assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

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
		imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;

		VkMemoryAllocateInfo memAllocInfo = vkTools::initializers::memoryAllocateInfo();
		VkMemoryRequirements memReqs;

		err = vkCreateImage(device, &imageCreateInfo, nullptr, &offScreenFrameBuf.textureTarget.image);
		assert(!err);
		vkGetImageMemoryRequirements(device, offScreenFrameBuf.textureTarget.image, &memReqs);
		memAllocInfo.allocationSize = memReqs.size;
		getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memAllocInfo.memoryTypeIndex);
		err = vkAllocateMemory(device, &memAllocInfo, nullptr, &offScreenFrameBuf.textureTarget.deviceMemory);
		assert(!err);
		err = vkBindImageMemory(device, offScreenFrameBuf.textureTarget.image, offScreenFrameBuf.textureTarget.deviceMemory, 0);
		assert(!err);

		offScreenFrameBuf.textureTarget.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		vkTools::setImageLayout(
			setupCmdBuffer,
			offScreenFrameBuf.textureTarget.image,
			VK_IMAGE_ASPECT_DEPTH_BIT,
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
		sampler.minLod = 0.0f;
		sampler.maxLod = 0.0f;
		sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		err = vkCreateSampler(device, &sampler, nullptr, &offScreenFrameBuf.textureTarget.sampler);
		assert(!err);

		// Create image view
		VkImageViewCreateInfo view = {};
		view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view.pNext = NULL;
		view.viewType = VK_IMAGE_VIEW_TYPE_2D;
		view.format = format;
		view.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		view.subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };
		view.image = offScreenFrameBuf.textureTarget.image;
		err = vkCreateImageView(device, &view, nullptr, &offScreenFrameBuf.textureTarget.view);
		assert(!err);

		flushSetupCommandBuffer();
	}

	// Set up a separate render pass for the offscreen frame buffer
	// This is necessary as the offscreen frame buffer attachments
	// use formats different to the ones from the visible frame buffer
	// and at least the depth one may not be compatible
	void setupOffScreenRenderPass()
	{
		VkAttachmentDescription attDesc[2];
		attDesc[0].format = FB_COLOR_FORMAT;
		attDesc[0].samples = VK_SAMPLE_COUNT_1_BIT;
		attDesc[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attDesc[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attDesc[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attDesc[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attDesc[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attDesc[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		attDesc[1].format = DEPTH_FORMAT;
		attDesc[1].samples = VK_SAMPLE_COUNT_1_BIT;
		attDesc[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		// Since we need to copy the depth attachment contents to our texture
		// used for shadow mapping we must use STORE_OP_STORE to make sure that
		// the depth attachment contents are preserved after rendering to it 
		// has finished
		attDesc[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attDesc[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attDesc[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attDesc[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		attDesc[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorReference = {};
		colorReference.attachment = 0;
		colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthReference = {};
		depthReference.attachment = 1;
		depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorReference;
		subpass.pDepthStencilAttachment = &depthReference;

		VkRenderPassCreateInfo renderPassCreateInfo = vkTools::initializers::renderPassCreateInfo();
		renderPassCreateInfo.attachmentCount = 2;
		renderPassCreateInfo.pAttachments = attDesc;
		renderPassCreateInfo.subpassCount = 1;
		renderPassCreateInfo.pSubpasses = &subpass;

		VkResult err = vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &offScreenFrameBuf.renderPass);
		assert(!err);
	}

	void prepareOffscreenFramebuffer()
	{
		createSetupCommandBuffer();

		offScreenFrameBuf.width = FB_DIM;
		offScreenFrameBuf.height = FB_DIM;

		VkFormat fbColorFormat = FB_COLOR_FORMAT;

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
		image.format = DEPTH_FORMAT;
		image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

		VkImageViewCreateInfo depthStencilView = vkTools::initializers::imageViewCreateInfo();
		depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
		depthStencilView.format = DEPTH_FORMAT;
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

		VkImageView attachments[2];
		attachments[0] = offScreenFrameBuf.color.view;
		attachments[1] = offScreenFrameBuf.depth.view;

		setupOffScreenRenderPass();

		// Create frame buffer
		VkFramebufferCreateInfo fbufCreateInfo = vkTools::initializers::framebufferCreateInfo();
		fbufCreateInfo.renderPass = offScreenFrameBuf.renderPass; 
		fbufCreateInfo.attachmentCount = 2;
		fbufCreateInfo.pAttachments = attachments;
		fbufCreateInfo.width = offScreenFrameBuf.width;
		fbufCreateInfo.height = offScreenFrameBuf.height;
		fbufCreateInfo.layers = 1;

		err = vkCreateFramebuffer(device, &fbufCreateInfo, nullptr, &offScreenFrameBuf.frameBuffer);
		assert(!err);

		flushSetupCommandBuffer();
	}

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
		}

		VkCommandBufferBeginInfo cmdBufInfo = vkTools::initializers::commandBufferBeginInfo();

		VkClearValue clearValues[2];
		clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderPassBeginInfo = vkTools::initializers::renderPassBeginInfo();
		renderPassBeginInfo.renderPass = offScreenFrameBuf.renderPass;
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
		vkCmdSetViewport(offScreenCmdBuffer, 0, 1, &viewport);

		VkRect2D scissor = vkTools::initializers::rect2D(
			offScreenFrameBuf.width,
			offScreenFrameBuf.height,
			0,
			0);
		vkCmdSetScissor(offScreenCmdBuffer, 0, 1, &scissor);

		// Set depth bias (aka "Polygon offset")
		vkCmdSetDepthBias(
			offScreenCmdBuffer,
			depthBiasConstant,
			0.0f,
			depthBiasSlope);

		vkCmdBeginRenderPass(offScreenCmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(offScreenCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.offscreen);
		vkCmdBindDescriptorSets(offScreenCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.offscreen, 0, 1, &descriptorSets.offscreen, 0, NULL);

		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(offScreenCmdBuffer, VERTEX_BUFFER_BIND_ID, 1, &meshes.scene.vertices.buf, offsets);
		vkCmdBindIndexBuffer(offScreenCmdBuffer, meshes.scene.indices.buf, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(offScreenCmdBuffer, meshes.scene.indexCount, 1, 0, 0, 0);

		vkCmdEndRenderPass(offScreenCmdBuffer);

		updateTexture();

		err = vkEndCommandBuffer(offScreenCmdBuffer);
		assert(!err);
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

			VkViewport viewport = vkTools::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
			vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

			VkRect2D scissor = vkTools::initializers::rect2D(width, height, 0, 0);
			vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

			VkDeviceSize offsets[1] = { 0 };

			vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.quad, 0, 1, &descriptorSet, 0, NULL);
			vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.quad);

			// Visualize shadow map
			if (displayShadowMap)
			{
				vkCmdBindVertexBuffers(drawCmdBuffers[i], VERTEX_BUFFER_BIND_ID, 1, &meshes.quad.vertices.buf, offsets);
				vkCmdBindIndexBuffer(drawCmdBuffers[i], meshes.quad.indices.buf, 0, VK_INDEX_TYPE_UINT32);
				vkCmdDrawIndexed(drawCmdBuffers[i], meshes.quad.indexCount, 1, 0, 0, 0);
			}

			// 3D scene
			vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.quad, 0, 1, &descriptorSets.scene, 0, NULL);
			vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.scene);

			vkCmdBindVertexBuffers(drawCmdBuffers[i], VERTEX_BUFFER_BIND_ID, 1, &meshes.scene.vertices.buf, offsets);
			vkCmdBindIndexBuffer(drawCmdBuffers[i], meshes.scene.indices.buf, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(drawCmdBuffers[i], meshes.scene.indexCount, 1, 0, 0, 0);

			vkCmdEndRenderPass(drawCmdBuffers[i]);

			err = vkEndCommandBuffer(drawCmdBuffers[i]);
			assert(!err);
		}
	}

	void draw()
	{
		VkResult err;

		// Get next image in the swap chain (back/front buffer)
		err = swapChain.acquireNextImage(semaphores.presentComplete, &currentBuffer);
		assert(!err);

		submitPostPresentBarrier(swapChain.buffers[currentBuffer].image);

		// Gather command buffers to be sumitted to the queue
		std::vector<VkCommandBuffer> submitCmdBuffers = {
			offScreenCmdBuffer,
			drawCmdBuffers[currentBuffer],
		};
		submitInfo.commandBufferCount = submitCmdBuffers.size();
		submitInfo.pCommandBuffers = submitCmdBuffers.data();

		// Submit to queue
		err = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
		assert(!err);

		submitPrePresentBarrier(swapChain.buffers[currentBuffer].image);

		err = swapChain.queuePresent(queue, currentBuffer, semaphores.renderComplete);
		assert(!err);

		err = vkQueueWaitIdle(queue);
		assert(!err);
	}

	void loadMeshes()
	{
		loadMesh(getAssetPath() + "models/vulkanscene_shadow.dae", &meshes.scene, vertexLayout, 4.0f);
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
		// Example uses three ubos and two image samplers
		std::vector<VkDescriptorPoolSize> poolSizes =
		{
			vkTools::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 6),
			vkTools::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4)
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
				offScreenFrameBuf.textureTarget.sampler,
				offScreenFrameBuf.textureTarget.view,
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
		texDescriptor.sampler = offScreenFrameBuf.textureTarget.sampler;
		texDescriptor.imageView = offScreenFrameBuf.textureTarget.view;

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
				VK_CULL_MODE_FRONT_BIT,
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

		shaderStages[0] = loadShader(getAssetPath() + "shaders/shadowmapping/quad.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getAssetPath() + "shaders/shadowmapping/quad.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

		VkGraphicsPipelineCreateInfo pipelineCreateInfo =
			vkTools::initializers::pipelineCreateInfo(
				pipelineLayouts.quad,
				renderPass,
				0);

		rasterizationState.cullMode = VK_CULL_MODE_NONE;

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
		shaderStages[0] = loadShader(getAssetPath() + "shaders/shadowmapping/scene.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getAssetPath() + "shaders/shadowmapping/scene.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		rasterizationState.cullMode = VK_CULL_MODE_NONE;
		err = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.scene);
		assert(!err);

		// Offscreen pipeline
		shaderStages[0] = loadShader(getAssetPath() + "shaders/shadowmapping/offscreen.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getAssetPath() + "shaders/shadowmapping/offscreen.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		pipelineCreateInfo.layout = pipelineLayouts.offscreen;
		// Cull front faces
		depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		// Enable depth bias
		rasterizationState.depthBiasEnable = VK_TRUE;
		// Add depth bias to dynamic state, so we can change it at runtime
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
		// Debug quad vertex shader uniform buffer block
		createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			sizeof(uboVSscene),
			nullptr,
			&uniformDataVS.buffer,
			&uniformDataVS.memory,
			&uniformDataVS.descriptor);

		// Offsvreen vertex shader uniform buffer block
		createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			sizeof(uboOffscreenVS),
			nullptr,
			&uniformDataOffscreenVS.buffer,
			&uniformDataOffscreenVS.memory,
			&uniformDataOffscreenVS.descriptor);

		// Scene vertex shader uniform buffer block 
		createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			sizeof(uboVSscene),
			nullptr,
			&uniformData.scene.buffer,
			&uniformData.scene.memory,
			&uniformData.scene.descriptor);

		updateLight();
		updateUniformBufferOffscreen();
		updateUniformBuffers();
	}

	void updateLight()
	{
		// Animate the light source
		lightPos.x = cos(glm::radians(timer * 360.0f)) * 40.0f;
		lightPos.y = -50.0f + sin(glm::radians(timer * 360.0f)) * 20.0f;
		lightPos.z = 25.0f + sin(glm::radians(timer * 360.0f)) * 5.0f;
	}

	void updateUniformBuffers()
	{
		// Shadow map debug quad
		float AR = (float)height / (float)width;

		uboVSquad.projection = glm::ortho(0.0f, 2.5f / AR, 0.0f, 2.5f, -1.0f, 1.0f);
		uboVSquad.model = glm::mat4();

		uint8_t *pData;
		VkResult err = vkMapMemory(device, uniformDataVS.memory, 0, sizeof(uboVSquad), 0, (void **)&pData);
		assert(!err);
		memcpy(pData, &uboVSquad, sizeof(uboVSquad));
		vkUnmapMemory(device, uniformDataVS.memory);

		// 3D scene
		uboVSscene.projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, zNear, zFar);

		uboVSscene.view = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, zoom));
		uboVSscene.view = glm::rotate(uboVSscene.view, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		uboVSscene.view = glm::rotate(uboVSscene.view, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		uboVSscene.view = glm::rotate(uboVSscene.view, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

		uboVSscene.model = glm::mat4();

		uboVSscene.lightPos = lightPos;

		// Render scene from light's point of view
		if (lightPOV)
		{
			uboVSscene.projection = glm::perspective(glm::radians(lightFOV), (float)width / (float)height, zNear, zFar);
			uboVSscene.view = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0, 1, 0));
		}

	
		uboVSscene.depthBiasMVP = uboOffscreenVS.depthMVP;

		pData;
		err = vkMapMemory(device, uniformData.scene.memory, 0, sizeof(uboVSscene), 0, (void **)&pData);
		assert(!err);
		memcpy(pData, &uboVSscene, sizeof(uboVSscene));
		vkUnmapMemory(device, uniformData.scene.memory);
	}

	void updateUniformBufferOffscreen()
	{
		// Matrix from light's point of view
		glm::mat4 depthProjectionMatrix = glm::perspective(glm::radians(lightFOV), 1.0f, zNear, zFar);
		glm::mat4 depthViewMatrix = glm::lookAt(lightPos, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
		glm::mat4 depthModelMatrix = glm::mat4();

		uboOffscreenVS.depthMVP = depthProjectionMatrix * depthViewMatrix * depthModelMatrix;

		uint8_t *pData;
		VkResult err = vkMapMemory(device, uniformDataOffscreenVS.memory, 0, sizeof(uboOffscreenVS), 0, (void **)&pData);
		assert(!err);
		memcpy(pData, &uboOffscreenVS, sizeof(uboOffscreenVS));
		vkUnmapMemory(device, uniformDataOffscreenVS.memory);
	}

	// Copy offscreen depth frame buffer contents to the depth texture
	void updateTexture()
	{
		// Make sure color writes to the framebuffer are finished before using it as transfer source
		vkTools::setImageLayout(
			offScreenCmdBuffer,
			offScreenFrameBuf.depth.image,
			VK_IMAGE_ASPECT_DEPTH_BIT,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

		// Transform texture target to transfer source
		vkTools::setImageLayout(
			offScreenCmdBuffer,
			offScreenFrameBuf.textureTarget.image,
			VK_IMAGE_ASPECT_DEPTH_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

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
			offScreenCmdBuffer,
			offScreenFrameBuf.depth.image,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			offScreenFrameBuf.textureTarget.image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&imgCopy);

		// Transform framebuffer color attachment back 
		vkTools::setImageLayout(
			offScreenCmdBuffer,
			offScreenFrameBuf.depth.image,
			VK_IMAGE_ASPECT_DEPTH_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

		// Transform texture target back to shader read
		// Makes sure that writes to the textuer are finished before
		// it's accessed in the shader
		vkTools::setImageLayout(
			offScreenCmdBuffer,
			offScreenFrameBuf.textureTarget.image,
			VK_IMAGE_ASPECT_DEPTH_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	void prepare()
	{
		VulkanExampleBase::prepare();
		generateQuad();
		loadMeshes();
		setupVertexDescriptions();
		prepareUniformBuffers();
		prepareTextureTarget(TEX_DIM, TEX_DIM, DEPTH_FORMAT);
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
			updateLight();
			updateUniformBufferOffscreen();
			updateUniformBuffers();
		}
	}

	virtual void viewChanged()
	{
		updateUniformBufferOffscreen();
		updateUniformBuffers();
	}

	void toggleShadowMapDisplay()
	{
		displayShadowMap = !displayShadowMap;
		buildCommandBuffers();
	}

	void toogleLightPOV()
	{
		lightPOV = !lightPOV;
		viewChanged();
	}
};

VulkanExample *vulkanExample;

#if defined(_WIN32)
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (vulkanExample != NULL)
	{
		vulkanExample->handleMessages(hWnd, uMsg, wParam, lParam);
		if (uMsg == WM_KEYDOWN)
		{
			switch (wParam)
			{
			case 0x53:
				vulkanExample->toggleShadowMapDisplay();
				break;
			case 0x4C:
				vulkanExample->toogleLightPOV();
				break;
			}
		}
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