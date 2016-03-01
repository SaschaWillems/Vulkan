/*
* Vulkan Example - Compute shader image processing
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

// Vertex layout for this example
struct Vertex {
	float pos[3];
	float uv[2];
};

class VulkanExample : public VulkanExampleBase
{
private:
	vkTools::VulkanTexture textureColorMap;
	vkTools::VulkanTexture textureComputeTarget;
public:
	struct {
		VkPipelineVertexInputStateCreateInfo inputState;
		std::vector<VkVertexInputBindingDescription> bindingDescriptions;
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
	} vertices;

	struct {
		vkMeshLoader::MeshBuffer quad;
	} meshes;

	vkTools::UniformData uniformDataVS;

	struct {
		glm::mat4 projection;
		glm::mat4 model;
	} uboVS;

	struct {
		VkPipeline postCompute;
		// Compute pipelines are separated from 
		// graphics pipelines in Vulkan
		std::vector<VkPipeline> compute;
		uint32_t computeIndex = 0;
	} pipelines;

	int vertexBufferSize;

	VkQueue computeQueue;
	VkCommandBuffer computeCmdBuffer;
	VkPipelineLayout computePipelineLayout;
	VkDescriptorSet computeDescriptorSet;
	VkDescriptorSetLayout computeDescriptorSetLayout;
	VkDescriptorPool computeDescriptorPool;

	VkPipelineLayout pipelineLayout;
	VkDescriptorSet descriptorSetPostCompute, descriptorSetBaseImage;
	VkDescriptorSetLayout descriptorSetLayout;

	VulkanExample() : VulkanExampleBase(ENABLE_VALIDATION)
	{
		width = 1280;
		height = 720;
		zoom = -2.0f;
		title = "Vulkan Example - Compute shader image processing";
	}

	~VulkanExample()
	{
		// Clean up used Vulkan resources 
		// Note : Inherited destructor cleans up resources stored in base class

		vkDestroyPipeline(device, pipelines.postCompute, nullptr);
		for (auto& pipeline : pipelines.compute)
		{
			vkDestroyPipeline(device, pipeline, nullptr);
		}

		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

		vkMeshLoader::freeMeshBufferResources(device, &meshes.quad);

		vkTools::destroyUniformData(device, &uniformDataVS);

		vkFreeCommandBuffers(device, cmdPool, 1, &computeCmdBuffer);

		textureLoader->destroyTexture(textureColorMap);
	}

	// Prepare an empty texture as the blit target from 
	// the offscreen framebuffer
	void prepareTextureTarget(vkTools::VulkanTexture *tex, uint32_t width, uint32_t height, VkFormat format)
	{
		VkFormatProperties formatProperties;
		VkResult err;

		// Get device properties for the requested texture format
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
		imageCreateInfo.flags = 0;

		VkMemoryAllocateInfo memAllocInfo = vkTools::initializers::memoryAllocateInfo();
		VkMemoryRequirements memReqs;

		err = vkCreateImage(device, &imageCreateInfo, nullptr, &tex->image);
		assert(!err);
		vkGetImageMemoryRequirements(device, tex->image, &memReqs);
		memAllocInfo.allocationSize = memReqs.size;
		getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memAllocInfo.memoryTypeIndex);
		err = vkAllocateMemory(device, &memAllocInfo, nullptr, &tex->deviceMemory);
		assert(!err);
		err = vkBindImageMemory(device, tex->image, tex->deviceMemory, 0);
		assert(!err);

		tex->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		vkTools::setImageLayout(setupCmdBuffer, tex->image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, tex->imageLayout);

		// Create sampler
		VkSamplerCreateInfo sampler = vkTools::initializers::samplerCreateInfo();
		sampler.magFilter = VK_FILTER_LINEAR;
		sampler.minFilter = VK_FILTER_LINEAR;
		sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
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

	void loadTextures()
	{
		textureLoader->loadTexture(
			"./../data/textures/igor_and_pal_rgba.ktx", 
			VK_FORMAT_R8G8B8A8_UNORM, 
			&textureColorMap);
	}

	void buildCommandBuffers()
	{
		// Destroy command buffers if already present
		if (!checkCommandBuffers())
		{
			destroyCommandBuffers();
			createCommandBuffers();
		}

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

			// Image memory barrier to make sure that compute
			// shader writes are finished before sampling
			// from the texture
			VkImageMemoryBarrier imageMemoryBarrier = {};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.pNext = NULL;
			imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
			imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
			imageMemoryBarrier.image = textureComputeTarget.image;
			imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
			vkCmdPipelineBarrier(
				drawCmdBuffers[i],
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_FLAGS_NONE,
				0, nullptr,
				0, nullptr,
				1, &imageMemoryBarrier);

			vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport viewport = vkTools::initializers::viewport(
				(float)width * 0.5f,
				(float)height,
				0.0f,
				1.0f
				);
			vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

			VkRect2D scissor = vkTools::initializers::rect2D(
				width,
				height,
				0,
				0
				);
			vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

			VkDeviceSize offsets[1] = { 0 };
			vkCmdBindVertexBuffers(drawCmdBuffers[i], VERTEX_BUFFER_BIND_ID, 1, &meshes.quad.vertices.buf, offsets);
			vkCmdBindIndexBuffer(drawCmdBuffers[i], meshes.quad.indices.buf, 0, VK_INDEX_TYPE_UINT32);

			// Left (pre compute)
			vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSetBaseImage, 0, NULL);
			vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.postCompute);

			vkCmdDrawIndexed(drawCmdBuffers[i], meshes.quad.indexCount, 1, 0, 0, 0);

			// Right (post compute)
			vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSetPostCompute, 0, NULL);
			vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.postCompute);

			viewport.x = (float)width / 2.0f;
			vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);
			vkCmdDrawIndexed(drawCmdBuffers[i], meshes.quad.indexCount, 1, 0, 0, 0);

			vkCmdEndRenderPass(drawCmdBuffers[i]);

			VkImageMemoryBarrier prePresentBarrier = vkTools::prePresentBarrier(swapChain.buffers[i].image);
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

	void buildComputeCommandBuffer()
	{
		VkCommandBufferBeginInfo cmdBufInfo = vkTools::initializers::commandBufferBeginInfo();

		VkResult err = vkBeginCommandBuffer(computeCmdBuffer, &cmdBufInfo);
		assert(!err);

		vkCmdBindPipeline(computeCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelines.compute[pipelines.computeIndex]);
		vkCmdBindDescriptorSets(computeCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 0, 1, &computeDescriptorSet, 0, 0);

		vkCmdDispatch(computeCmdBuffer, textureComputeTarget.width / 16, textureComputeTarget.height / 16, 1);

		vkEndCommandBuffer(computeCmdBuffer);
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

		VkSubmitInfo submitInfo = vkTools::initializers::submitInfo();
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &presentCompleteSemaphore;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];

		// Submit draw command buffer
		err = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
		assert(!err);

		err = swapChain.queuePresent(queue, currentBuffer);
		assert(!err);

		vkDestroySemaphore(device, presentCompleteSemaphore, nullptr);

		submitPostPresentBarrier(swapChain.buffers[currentBuffer].image);

		err = vkQueueWaitIdle(queue);
		assert(!err);

		// Compute
		VkSubmitInfo computeSubmitInfo = vkTools::initializers::submitInfo();
		computeSubmitInfo.commandBufferCount = 1;
		computeSubmitInfo.pCommandBuffers = &computeCmdBuffer;

		err = vkQueueSubmit(computeQueue, 1, &computeSubmitInfo, VK_NULL_HANDLE);
		assert(!err);

		err = vkQueueWaitIdle(computeQueue);
		assert(!err);
	}

	// Setup vertices for a single uv-mapped quad
	void generateQuad()
	{
#define dim 1.0f
		std::vector<Vertex> vertexBuffer =
		{
			{ {  dim,  dim, 0.0f }, { 1.0f, 1.0f } },
			{ { -dim,  dim, 0.0f }, { 0.0f, 1.0f } },
			{ { -dim, -dim, 0.0f }, { 0.0f, 0.0f } },
			{ {  dim, -dim, 0.0f }, { 1.0f, 0.0f } }
		};
#undef dim

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
				sizeof(Vertex),
				VK_VERTEX_INPUT_RATE_VERTEX);

		// Attribute descriptions
		// Describes memory layout and shader positions
		vertices.attributeDescriptions.resize(2);
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

		// Assign to vertex buffer
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
			vkTools::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2),
			vkTools::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4),
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

		err = vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &pipelineLayout);
		assert(!err);
	}

	void setupDescriptorSet()
	{
		VkDescriptorSetAllocateInfo allocInfo =
			vkTools::initializers::descriptorSetAllocateInfo(
				descriptorPool,
				&descriptorSetLayout,
				1);

		VkResult vkRes = vkAllocateDescriptorSets(device, &allocInfo, &descriptorSetPostCompute);
		assert(!vkRes);

		// Image descriptor for the color map texture
		VkDescriptorImageInfo texDescriptor =
			vkTools::initializers::descriptorImageInfo(
				textureComputeTarget.sampler,
				textureComputeTarget.view,
				VK_IMAGE_LAYOUT_GENERAL);

		std::vector<VkWriteDescriptorSet> writeDescriptorSets =
		{
			// Binding 0 : Vertex shader uniform buffer
			vkTools::initializers::writeDescriptorSet(
				descriptorSetPostCompute,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				0,
				&uniformDataVS.descriptor),
			// Binding 1 : Fragment shader texture sampler
			vkTools::initializers::writeDescriptorSet(
				descriptorSetPostCompute,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				1,
				&texDescriptor)
		};

		vkUpdateDescriptorSets(device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);

		// Base image (before compute post process)
		allocInfo =
			vkTools::initializers::descriptorSetAllocateInfo(
				descriptorPool,
				&descriptorSetLayout,
				1);

		vkRes = vkAllocateDescriptorSets(device, &allocInfo, &descriptorSetBaseImage);
		assert(!vkRes);
		
		VkDescriptorImageInfo texDescriptorBaseImage =
			vkTools::initializers::descriptorImageInfo(
				textureColorMap.sampler,
				textureColorMap.view,
				VK_IMAGE_LAYOUT_GENERAL);

		std::vector<VkWriteDescriptorSet> baseImageWriteDescriptorSets =
		{
			// Binding 0 : Vertex shader uniform buffer
			vkTools::initializers::writeDescriptorSet(
				descriptorSetBaseImage,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				0,
				&uniformDataVS.descriptor),
			// Binding 1 : Fragment shader texture sampler
			vkTools::initializers::writeDescriptorSet(
				descriptorSetBaseImage,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				1,
				&texDescriptorBaseImage)
		};

		vkUpdateDescriptorSets(device, baseImageWriteDescriptorSets.size(), baseImageWriteDescriptorSets.data(), 0, NULL);
	}

	// Create a separate command buffer for compute commands
	void createComputeCommandBuffer()
	{
		VkCommandBufferAllocateInfo cmdBufAllocateInfo =
			vkTools::initializers::commandBufferAllocateInfo(
				cmdPool,
				VK_COMMAND_BUFFER_LEVEL_PRIMARY,
				1);

		VkResult vkRes = vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &computeCmdBuffer);
		assert(!vkRes);
	}

	void preparePipelines()
	{
		VkResult err;

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

		// Rendering pipeline
		// Load shaders
		std::array<VkPipelineShaderStageCreateInfo,2> shaderStages;

#ifdef USE_GLSL
		shaderStages[0] = loadShaderGLSL("./../data/shaders/computeshader/texture.vert", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShaderGLSL("./../data/shaders/computeshader/texture.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
#else
		shaderStages[0] = loadShader("./../data/shaders/computeshader/texture.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader("./../data/shaders/computeshader/texture.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
#endif

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
		pipelineCreateInfo.renderPass = renderPass;

		err = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.postCompute);
		assert(!err);
	}

	void prepareCompute()
	{
		// Create compute pipeline
		// Compute pipelines are created separate from graphics pipelines
		// even if they use the same queue

		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
			// Binding 0 : Sampled image (read)
			vkTools::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_SHADER_STAGE_COMPUTE_BIT,
				0),
			// Binding 1 : Sampled image (write)
			vkTools::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_SHADER_STAGE_COMPUTE_BIT,
				1),
		};

		VkDescriptorSetLayoutCreateInfo descriptorLayout =
			vkTools::initializers::descriptorSetLayoutCreateInfo(
				setLayoutBindings.data(),
				setLayoutBindings.size());

		VkResult err = vkCreateDescriptorSetLayout(
			device,
			&descriptorLayout,
			nullptr,
			&computeDescriptorSetLayout);
		assert(!err);

		VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo =
			vkTools::initializers::pipelineLayoutCreateInfo(
				&computeDescriptorSetLayout,
				1);

		err = vkCreatePipelineLayout(
			device,
			&pPipelineLayoutCreateInfo,
			nullptr,
			&computePipelineLayout);
		assert(!err);

		VkDescriptorSetAllocateInfo allocInfo =
			vkTools::initializers::descriptorSetAllocateInfo(
				descriptorPool,
				&computeDescriptorSetLayout,
				1);

		err = vkAllocateDescriptorSets(device, &allocInfo, &computeDescriptorSet);
		assert(!err);

		std::vector<VkDescriptorImageInfo> computeTexDescriptors =
		{
			vkTools::initializers::descriptorImageInfo(
			textureColorMap.sampler,
				textureColorMap.view,
				VK_IMAGE_LAYOUT_GENERAL),

			vkTools::initializers::descriptorImageInfo(
				textureComputeTarget.sampler,
				textureComputeTarget.view,
				VK_IMAGE_LAYOUT_GENERAL)
		};

		std::vector<VkWriteDescriptorSet> computeWriteDescriptorSets =
		{
			// Binding 0 : Sampled image (read)
			vkTools::initializers::writeDescriptorSet(
			computeDescriptorSet,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				0,
				&computeTexDescriptors[0]),
			// Binding 1 : Sampled image (write)
			vkTools::initializers::writeDescriptorSet(
				computeDescriptorSet,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				1,
				&computeTexDescriptors[1])
		};

		vkUpdateDescriptorSets(device, computeWriteDescriptorSets.size(), computeWriteDescriptorSets.data(), 0, NULL);


		// Create compute shader pipelines
		VkComputePipelineCreateInfo computePipelineCreateInfo =
			vkTools::initializers::computePipelineCreateInfo(
				computePipelineLayout,
				0);

		std::vector<std::string> shaderNames = { "edgedetect", "emboss", "sharpen" };

		for (auto& shaderName : shaderNames)
		{
			std::string fileName = "./../data/shaders/computeshader/" + shaderName + ".comp";
#ifdef USE_GLSL
			computePipelineCreateInfo.stage = loadShaderGLSL(fileName.c_str(), VK_SHADER_STAGE_COMPUTE_BIT);
#else
			fileName += ".spv";
			computePipelineCreateInfo.stage = loadShader(fileName.c_str(), VK_SHADER_STAGE_COMPUTE_BIT);
#endif
			VkPipeline pipeline;
			err = vkCreateComputePipelines(device, pipelineCache, 1, &computePipelineCreateInfo, nullptr, &pipeline);
			assert(!err);

			pipelines.compute.push_back(pipeline);
		}
	}

	// Prepare and initialize uniform buffer containing shader uniforms
	void prepareUniformBuffers()
	{
		// Vertex shader uniform buffer block
		createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			sizeof(uboVS),
			&uboVS,
			&uniformDataVS.buffer,
			&uniformDataVS.memory,
			&uniformDataVS.descriptor);

		updateUniformBuffers();
	}

	void updateUniformBuffers()
	{
		// Vertex shader
		glm::mat4 viewMatrix = glm::mat4();
		uboVS.projection = glm::perspective(deg_to_rad(60.0f), (float)width*0.5f / (float)height, 0.1f, 256.0f);
		viewMatrix = glm::translate(viewMatrix, glm::vec3(0.0f, 0.0f, zoom));

		uboVS.model = glm::mat4();
		uboVS.model = viewMatrix * glm::translate(uboVS.model, glm::vec3(0, 0, 0));
		uboVS.model = glm::rotate(uboVS.model, deg_to_rad(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		uboVS.model = glm::rotate(uboVS.model, deg_to_rad(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		uboVS.model = glm::rotate(uboVS.model, deg_to_rad(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

		uint8_t *pData;
		VkResult err = vkMapMemory(device, uniformDataVS.memory, 0, sizeof(uboVS), 0, (void **)&pData);
		assert(!err);
		memcpy(pData, &uboVS, sizeof(uboVS));
		vkUnmapMemory(device, uniformDataVS.memory);
	}

	// Find and create a compute capable device queue
	void getComputeQueue()
	{
		uint32_t queueIndex = 0;
		uint32_t queueCount;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, NULL);
		assert(queueCount >= 1);

		std::vector<VkQueueFamilyProperties> queueProps;
		queueProps.resize(queueCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, queueProps.data());
		
		for (queueIndex = 0; queueIndex < queueCount; queueIndex++)
		{
			if (queueProps[queueIndex].queueFlags & VK_QUEUE_COMPUTE_BIT)
				break;
		}
		assert(queueIndex < queueCount);

		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.queueFamilyIndex = queueIndex;
		queueCreateInfo.queueCount = 1;
		vkGetDeviceQueue(device, queueIndex, 0, &computeQueue);
	}

	void prepare()
	{
		VulkanExampleBase::prepare();
		loadTextures();
		generateQuad();
		getComputeQueue();
		createComputeCommandBuffer();
		setupVertexDescriptions();
		prepareUniformBuffers();
		prepareTextureTarget(&textureComputeTarget, textureColorMap.width, textureColorMap.height, VK_FORMAT_R8G8B8A8_UNORM);
		setupDescriptorSetLayout();
		preparePipelines();
		setupDescriptorPool();
		setupDescriptorSet();
		prepareCompute();
		buildCommandBuffers(); 
		buildComputeCommandBuffer();
		prepared = true;
	}

	virtual void render()
	{
		if (!prepared)
			return;
		vkDeviceWaitIdle(device);
		draw();
		vkDeviceWaitIdle(device);
		updateUniformBuffers();
	}

	virtual void viewChanged()
	{
		updateUniformBuffers();
	}

	virtual void switchComputePipeline(int32_t dir)
	{
		if ((dir < 0) && (pipelines.computeIndex > 0))
		{
			pipelines.computeIndex--;
			buildComputeCommandBuffer();
		}
		if ((dir > 0) && (pipelines.computeIndex < pipelines.compute.size()-1))
		{
			pipelines.computeIndex++;
			buildComputeCommandBuffer();
		}
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
			case VK_ADD:
			case VK_SUBTRACT:
				vulkanExample->switchComputePipeline((wParam == VK_ADD) ? 1 : -1);
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
