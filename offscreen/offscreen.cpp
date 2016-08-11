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
#define ENABLE_VALIDATION false

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

class VulkanExample : public VulkanExampleBase
{
public:
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
		VkPipeline shadedOffscreen;
		VkPipeline mirror;
	} pipelines;

	struct {
		VkPipelineLayout textured;
		VkPipelineLayout shaded;
	} pipelineLayouts;

	struct {
		VkDescriptorSet offscreen;
		VkDescriptorSet mirror;
		VkDescriptorSet model;
		VkDescriptorSet debugQuad;
	} descriptorSets;

	struct {
		VkDescriptorSetLayout textured;
		VkDescriptorSetLayout shaded;
	} descriptorSetLayouts;

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

	glm::vec3 meshPos = glm::vec3(0.0f, -1.5f, 0.0f);
	glm::vec3 meshRot = glm::vec3(0.0f);

	VulkanExample() : VulkanExampleBase(ENABLE_VALIDATION)
	{
		zoom = -6.0f;
		rotation = { -2.5f, 0.0f, 0.0f };
		cameraPos = { 0.0f, 1.0f, 0.0f };
		timerSpeed *= 0.25f;
		enableTextOverlay = true;
		title = "Vulkan Example - Offscreen rendering";
	}

	~VulkanExample()
	{
		// Clean up used Vulkan resources 
		// Note : Inherited destructor cleans up resources stored in base class

		// Textures
		textureLoader->destroyTexture(textures.colorMap);
		vkDestroySampler(device, offScreenFrameBuf.colorSampler, nullptr);

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

		vkDestroyPipeline(device, pipelines.debug, nullptr);
		vkDestroyPipeline(device, pipelines.shaded, nullptr);
		vkDestroyPipeline(device, pipelines.shadedOffscreen, nullptr);
		vkDestroyPipeline(device, pipelines.mirror, nullptr);

		vkDestroyPipelineLayout(device, pipelineLayouts.textured, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayouts.shaded, nullptr);

		vkDestroyDescriptorSetLayout(device, descriptorSetLayouts.shaded, nullptr);
		vkDestroyDescriptorSetLayout(device, descriptorSetLayouts.textured, nullptr);

		// Meshes
		vkMeshLoader::freeMeshBufferResources(device, &meshes.example);
		vkMeshLoader::freeMeshBufferResources(device, &meshes.quad);
		vkMeshLoader::freeMeshBufferResources(device, &meshes.plane);

		// Uniform buffers
		vkTools::destroyUniformData(device, &uniformData.vsShared);
		vkTools::destroyUniformData(device, &uniformData.vsMirror);
		vkTools::destroyUniformData(device, &uniformData.vsOffScreen);
		vkTools::destroyUniformData(device, &uniformData.vsDebugQuad);

		vkFreeCommandBuffers(device, cmdPool, 1, &offScreenCmdBuffer);
		vkDestroySemaphore(device, offscreenSemaphore, nullptr);
	}

	// Setup the offscreen framebuffer for rendering the mirrored scene
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

		// Create sampler to sample from to color attachment 
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

		vkCmdBeginRenderPass(offScreenCmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = vkTools::initializers::viewport((float)offScreenFrameBuf.width, (float)offScreenFrameBuf.height, 0.0f, 1.0f);
		vkCmdSetViewport(offScreenCmdBuffer, 0, 1, &viewport);

		VkRect2D scissor = vkTools::initializers::rect2D(offScreenFrameBuf.width, offScreenFrameBuf.height, 0, 0);
		vkCmdSetScissor(offScreenCmdBuffer, 0, 1, &scissor);

		VkDeviceSize offsets[1] = { 0 };

		// Mirrored scene
		vkCmdBindDescriptorSets(offScreenCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.shaded, 0, 1, &descriptorSets.offscreen, 0, NULL);
		vkCmdBindPipeline(offScreenCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.shadedOffscreen);
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

			VkRect2D scissor = vkTools::initializers::rect2D(width, height,	0, 0);
			vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

			VkDeviceSize offsets[1] = { 0 };

			if (debugDisplay)
			{
				vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.textured, 0, 1, &descriptorSets.debugQuad, 0, NULL);
				vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.debug);
				vkCmdBindVertexBuffers(drawCmdBuffers[i], VERTEX_BUFFER_BIND_ID, 1, &meshes.quad.vertices.buf, offsets);
				vkCmdBindIndexBuffer(drawCmdBuffers[i], meshes.quad.indices.buf, 0, VK_INDEX_TYPE_UINT32);
				vkCmdDrawIndexed(drawCmdBuffers[i], meshes.quad.indexCount, 1, 0, 0, 0);
			}

			// Scene
			vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.debug);

			// Reflection plane
			vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.textured, 0, 1, &descriptorSets.mirror, 0, NULL);
			vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.mirror);

			vkCmdBindVertexBuffers(drawCmdBuffers[i], VERTEX_BUFFER_BIND_ID, 1, &meshes.plane.vertices.buf, offsets);
			vkCmdBindIndexBuffer(drawCmdBuffers[i], meshes.plane.indices.buf, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(drawCmdBuffers[i], meshes.plane.indexCount, 1, 0, 0, 0);

			// Model
			vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.shaded, 0, 1, &descriptorSets.model, 0, NULL);
			vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.shaded);

			vkCmdBindVertexBuffers(drawCmdBuffers[i], VERTEX_BUFFER_BIND_ID, 1, &meshes.example.vertices.buf, offsets);
			vkCmdBindIndexBuffer(drawCmdBuffers[i], meshes.example.indices.buf, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(drawCmdBuffers[i], meshes.example.indexCount, 1, 0, 0, 0);

			vkCmdEndRenderPass(drawCmdBuffers[i]);

			VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
		}
	}

	void loadMeshes()
	{
		loadMesh(getAssetPath() + "models/plane.obj", &meshes.plane, vertexLayout, 0.4f);
		loadMesh(getAssetPath() + "models/chinesedragon.dae", &meshes.example, vertexLayout, 0.3f);
	}

	void loadTextures()
	{
		textureLoader->loadTexture(
			getAssetPath() + "textures/darkmetal_bc3.ktx",
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
		std::vector<VkDescriptorPoolSize> poolSizes =
		{
			vkTools::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 6),
			vkTools::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 8)
		};

		VkDescriptorPoolCreateInfo descriptorPoolInfo =
			vkTools::initializers::descriptorPoolCreateInfo(
				poolSizes.size(),
				poolSizes.data(),
				5);

		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));
	}

	void setupDescriptorSetLayout()
	{
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings;
		VkDescriptorSetLayoutCreateInfo descriptorLayoutInfo;
		VkPipelineLayoutCreateInfo pipelineLayoutInfo;

		// Binding 0 : Vertex shader uniform buffer
		setLayoutBindings.push_back(vkTools::initializers::descriptorSetLayoutBinding(
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			VK_SHADER_STAGE_VERTEX_BIT,
			0));
		// Binding 1 : Fragment shader image sampler
		setLayoutBindings.push_back(vkTools::initializers::descriptorSetLayoutBinding(
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			1));
		// Binding 2 : Fragment shader image sampler
		setLayoutBindings.push_back(vkTools::initializers::descriptorSetLayoutBinding(
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			2));

		// Shaded layouts (only use first layout binding)
		descriptorLayoutInfo = vkTools::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings.data(), 1);
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayoutInfo, nullptr, &descriptorSetLayouts.shaded));

		pipelineLayoutInfo = vkTools::initializers::pipelineLayoutCreateInfo(&descriptorSetLayouts.shaded, 1);
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayouts.shaded));

		// Textured layouts (use all layout bindings)
		descriptorLayoutInfo = vkTools::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings.data(), static_cast<uint32_t>(setLayoutBindings.size()));
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayoutInfo, nullptr, &descriptorSetLayouts.textured));

		pipelineLayoutInfo = vkTools::initializers::pipelineLayoutCreateInfo(&descriptorSetLayouts.textured, 1);
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayouts.textured));
	}

	void setupDescriptorSet()
	{
		// Mirror plane descriptor set
		VkDescriptorSetAllocateInfo allocInfo =
			vkTools::initializers::descriptorSetAllocateInfo(
				descriptorPool,
				&descriptorSetLayouts.textured,
				1);

		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.mirror));

		// Image descriptor for the offscreen mirror color attachment image
		VkDescriptorImageInfo texDescriptorMirror =
			vkTools::initializers::descriptorImageInfo(
				offScreenFrameBuf.colorSampler,
				offScreenFrameBuf.color.view,
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

		vkUpdateDescriptorSets(device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);

		// Debug quad
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.debugQuad));

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
		vkUpdateDescriptorSets(device, debugQuadWriteDescriptorSets.size(), debugQuadWriteDescriptorSets.data(), 0, NULL);

		// Shaded descriptor sets
		allocInfo.pSetLayouts = &descriptorSetLayouts.shaded;

		// Model
		// No texture
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.model));

		std::vector<VkWriteDescriptorSet> modelWriteDescriptorSets =
		{
			// Binding 0 : Vertex shader uniform buffer
			vkTools::initializers::writeDescriptorSet(
				descriptorSets.model,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				0,
				&uniformData.vsShared.descriptor)
		};
		vkUpdateDescriptorSets(device, modelWriteDescriptorSets.size(), modelWriteDescriptorSets.data(), 0, NULL);

		// Offscreen
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.offscreen));

		std::vector<VkWriteDescriptorSet> offScreenWriteDescriptorSets =
		{
			// Binding 0 : Vertex shader uniform buffer
			vkTools::initializers::writeDescriptorSet(
				descriptorSets.offscreen,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				0,
				&uniformData.vsOffScreen.descriptor)
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

		shaderStages[0] = loadShader(getAssetPath() + "shaders/offscreen/quad.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getAssetPath() + "shaders/offscreen/quad.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

		VkGraphicsPipelineCreateInfo pipelineCreateInfo =
			vkTools::initializers::pipelineCreateInfo(
				pipelineLayouts.textured,
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

		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.debug));

		// Mirror
		shaderStages[0] = loadShader(getAssetPath() + "shaders/offscreen/mirror.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getAssetPath() + "shaders/offscreen/mirror.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		rasterizationState.cullMode = VK_CULL_MODE_NONE;
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.mirror));

		// Flip culling
		rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;

		// Phong shading pipelines
		pipelineCreateInfo.layout = pipelineLayouts.shaded;
		// Scene
		shaderStages[0] = loadShader(getAssetPath() + "shaders/offscreen/phong.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getAssetPath() + "shaders/offscreen/phong.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.shaded));
		// Offscreen
		// Flip culling
		rasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.shadedOffscreen));

	}

	// Prepare and initialize uniform buffer containing shader uniforms
	void prepareUniformBuffers()
	{
		// Mesh vertex shader uniform buffer block
		createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			sizeof(ubos.vsShared),
			nullptr,
			&uniformData.vsShared.buffer,
			&uniformData.vsShared.memory,
			&uniformData.vsShared.descriptor);

		// Mirror plane vertex shader uniform buffer block
		createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			sizeof(ubos.vsShared),
			nullptr,
			&uniformData.vsMirror.buffer,
			&uniformData.vsMirror.memory,
			&uniformData.vsMirror.descriptor);

		// Offscreen vertex shader uniform buffer block 
		createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			sizeof(ubos.vsShared),
			nullptr,
			&uniformData.vsOffScreen.buffer,
			&uniformData.vsOffScreen.memory,
			&uniformData.vsOffScreen.descriptor);

		// Debug quad vertex shader uniform buffer block 
		createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
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
		ubos.vsShared.projection = glm::perspective(glm::radians(60.0f), (float)width / (float)height, 0.1f, 256.0f);
		glm::mat4 viewMatrix = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, zoom));

		ubos.vsShared.model = viewMatrix * glm::translate(glm::mat4(), cameraPos);
		ubos.vsShared.model = glm::rotate(ubos.vsShared.model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		ubos.vsShared.model = glm::rotate(ubos.vsShared.model, glm::radians(rotation.y + meshRot.y), glm::vec3(0.0f, 1.0f, 0.0f));
		ubos.vsShared.model = glm::rotate(ubos.vsShared.model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

		ubos.vsShared.model = glm::translate(ubos.vsShared.model, meshPos);

		uint8_t *pData;
		VK_CHECK_RESULT(vkMapMemory(device, uniformData.vsShared.memory, 0, sizeof(ubos.vsShared), 0, (void **)&pData));
		memcpy(pData, &ubos.vsShared, sizeof(ubos.vsShared));
		vkUnmapMemory(device, uniformData.vsShared.memory);

		// Mirror
		ubos.vsShared.model = viewMatrix * glm::translate(glm::mat4(), cameraPos);
		ubos.vsShared.model = glm::rotate(ubos.vsShared.model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		ubos.vsShared.model = glm::rotate(ubos.vsShared.model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		ubos.vsShared.model = glm::rotate(ubos.vsShared.model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

		VK_CHECK_RESULT(vkMapMemory(device, uniformData.vsMirror.memory, 0, sizeof(ubos.vsShared), 0, (void **)&pData));
		memcpy(pData, &ubos.vsShared, sizeof(ubos.vsShared));
		vkUnmapMemory(device, uniformData.vsMirror.memory);

		// Debug quad
		ubos.vsShared.projection = glm::ortho(4.0f, 0.0f, 0.0f, 4.0f*(float)height / (float)width, -1.0f, 1.0f);
		ubos.vsShared.model = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, 0.0f));

		VK_CHECK_RESULT(vkMapMemory(device, uniformData.vsDebugQuad.memory, 0, sizeof(ubos.vsShared), 0, (void **)&pData));
		memcpy(pData, &ubos.vsShared, sizeof(ubos.vsShared));
		vkUnmapMemory(device, uniformData.vsDebugQuad.memory);
	}

	void updateUniformBufferOffscreen()
	{
		ubos.vsShared.projection = glm::perspective(glm::radians(60.0f), (float)width / (float)height, 0.1f, 256.0f);
		glm::mat4 viewMatrix = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, zoom));

		ubos.vsShared.model = viewMatrix * glm::translate(glm::mat4(), cameraPos);
		ubos.vsShared.model = glm::rotate(ubos.vsShared.model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		ubos.vsShared.model = glm::rotate(ubos.vsShared.model, glm::radians(rotation.y + meshRot.y), glm::vec3(0.0f, 1.0f, 0.0f));
		ubos.vsShared.model = glm::rotate(ubos.vsShared.model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

		ubos.vsShared.model = glm::scale(ubos.vsShared.model, glm::vec3(1.0f, -1.0f, 1.0f));
		ubos.vsShared.model = glm::translate(ubos.vsShared.model, meshPos);

		uint8_t *pData;
		VK_CHECK_RESULT(vkMapMemory(device, uniformData.vsOffScreen.memory, 0, sizeof(ubos.vsShared), 0, (void **)&pData));
		memcpy(pData, &ubos.vsShared, sizeof(ubos.vsShared));
		vkUnmapMemory(device, uniformData.vsOffScreen.memory);
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
		loadTextures();
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
			meshRot.y += frameTimer * 10.0f;
			updateUniformBuffers();
			updateUniformBufferOffscreen();
		}
	}

	virtual void viewChanged()
	{
		updateUniformBuffers();
		updateUniformBufferOffscreen();
	}

	virtual void keyPressed(uint32_t keyCode)
	{
		switch (keyCode)
		{
		case KEY_D:
		case GAMEPAD_BUTTON_A:
			toggleDebugDisplay();
			break;
		}
	}

	virtual void getOverlayText(VulkanTextOverlay *textOverlay)
	{
#if defined(__ANDROID__)
		textOverlay->addText("Press \"Button A\" to display offscreen target", 5.0f, 85.0f, VulkanTextOverlay::alignLeft);
#else
		textOverlay->addText("Press \"d\" to display offscreen target", 5.0f, 85.0f, VulkanTextOverlay::alignLeft);
#endif
	}

	void toggleDebugDisplay()
	{
		debugDisplay = !debugDisplay;
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
