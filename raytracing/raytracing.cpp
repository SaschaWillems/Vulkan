/*
* Vulkan Example - Compute shader ray tracing
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

#define TEX_DIM 2048

// Vertex layout for this example
struct Vertex {
	float pos[3];
	float uv[2];
};

class VulkanExample : public CBaseVulkanGame
{
private:
	vkTools::VulkanTexture textureComputeTarget;
public:
	virtual int32_t			init(CVulkanFramework* pFramework)
	{
		CBaseVulkanGame::init(pFramework);
		m_pFramework->zoom = -2.0f;
		m_pFramework->title = "Vulkan Example - Compute shader ray tracing";
		uboCompute.aspectRatio = (float)m_pFramework->ScreenRect.Width / (float)m_pFramework->ScreenRect.Height;
		m_pFramework->paused = true;
		m_pFramework->timerSpeed *= 0.5f;
		// Values not set here are initialized in the base class constructor
		return 0;
	};

	struct {
		VkPipelineVertexInputStateCreateInfo inputState;
		std::vector<VkVertexInputBindingDescription> bindingDescriptions;
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
	} vertices;

	struct {
		vkMeshLoader::MeshBuffer quad;
	} meshes;

	vkTools::UniformData uniformDataCompute;

	struct {
		glm::vec3 lightPos;
		// Aspect ratio of the viewport
		float aspectRatio;
		glm::vec4 fogColor = glm::vec4(0.025f, 0.025f, 0.025f, 0.0f);
		struct {
			glm::vec3 pos = glm::vec3(0.0f, 1.5f, 4.0f);
			glm::vec3 lookat = glm::vec3(0.0f, 0.5f, 0.0f);
			float fov = 10.0f;
		} camera;
	} uboCompute;

	struct {
		VkPipeline display;
		VkPipeline compute;
	} pipelines;

	int vertexBufferSize;

	VkQueue computeQueue;
	VkCommandBuffer computeCmdBuffer;
	VkPipelineLayout computePipelineLayout;
	VkDescriptorSet computeDescriptorSet;
	VkDescriptorSetLayout computeDescriptorSetLayout;
	VkDescriptorPool computeDescriptorPool;

	VkPipelineLayout pipelineLayout;
	VkDescriptorSet descriptorSetPostCompute;
	VkDescriptorSetLayout descriptorSetLayout;

	VulkanExample()
	{
	}

	~VulkanExample()
	{
		// Clean up used Vulkan resources 
		// Note : Inherited destructor cleans up resources stored in base class

		vkDestroyPipeline(m_pFramework->device, pipelines.display, nullptr);
		vkDestroyPipeline(m_pFramework->device, pipelines.compute, nullptr);

		vkDestroyPipelineLayout(m_pFramework->device, pipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(m_pFramework->device, descriptorSetLayout, nullptr);

		vkMeshLoader::freeMeshBufferResources(m_pFramework->device, &meshes.quad);

		vkTools::destroyUniformData(m_pFramework->device, &uniformDataCompute);

		vkFreeCommandBuffers(m_pFramework->device, m_pFramework->cmdPool, 1, &computeCmdBuffer);

		m_pFramework->textureLoader->destroyTexture(textureComputeTarget);
	}

	// Prepare a texture target that is used to store compute shader calculations
	void prepareTextureTarget(vkTools::VulkanTexture *tex, uint32_t width, uint32_t height, VkFormat format)
	{
		// Get device properties for the requested texture format
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(m_pFramework->physicalDevice, format, &formatProperties);
		// Check if requested image format supports image storage operations
		assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT);

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
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
		// Image will be sampled in the fragment shader and used as storage target in the compute shader
		imageCreateInfo.usage = 
			VK_IMAGE_USAGE_SAMPLED_BIT | 
			VK_IMAGE_USAGE_STORAGE_BIT;
		imageCreateInfo.flags = 0;

		VkMemoryAllocateInfo memAllocInfo = vkTools::initializers::memoryAllocateInfo();
		VkMemoryRequirements memReqs;

		vkTools::checkResult(vkCreateImage(m_pFramework->device, &imageCreateInfo, nullptr, &tex->image));
		vkGetImageMemoryRequirements(m_pFramework->device, tex->image, &memReqs);
		memAllocInfo.allocationSize = memReqs.size;
		m_pFramework->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memAllocInfo.memoryTypeIndex);
		vkTools::checkResult(vkAllocateMemory(m_pFramework->device, &memAllocInfo, nullptr, &tex->deviceMemory));
		vkTools::checkResult(vkBindImageMemory(m_pFramework->device, tex->image, tex->deviceMemory, 0));

		tex->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		vkTools::setImageLayout(
			m_pFramework->setupCmdBuffer, tex->image, 
			VK_IMAGE_ASPECT_COLOR_BIT, 
			VK_IMAGE_LAYOUT_PREINITIALIZED,
			tex->imageLayout);

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
		vkTools::checkResult(vkCreateSampler(m_pFramework->device, &sampler, nullptr, &tex->sampler));

		// Create image view
		VkImageViewCreateInfo view = vkTools::initializers::imageViewCreateInfo();
		view.viewType = VK_IMAGE_VIEW_TYPE_2D;
		view.format = format;
		view.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		view.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		view.image = tex->image;
		vkTools::checkResult(vkCreateImageView(m_pFramework->device, &view, nullptr, &tex->view));
	}

	void buildCommandBuffers()
	{
		// Destroy command buffers if already present
		if (!m_pFramework->checkCommandBuffers())
		{
			m_pFramework->destroyCommandBuffers();
			m_pFramework->createCommandBuffers();
		}

		VkCommandBufferBeginInfo cmdBufInfo = vkTools::initializers::commandBufferBeginInfo();

		VkClearValue clearValues[2];
		clearValues[0].color = m_pFramework->defaultClearColor;
		clearValues[0].color = { {0.0f, 0.0f, 0.2f, 0.0f} };
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderPassBeginInfo = vkTools::initializers::renderPassBeginInfo();
		renderPassBeginInfo.renderPass =m_pFramework-> renderPass;
		renderPassBeginInfo.renderArea.offset.x = 0;
		renderPassBeginInfo.renderArea.offset.y = 0;
		renderPassBeginInfo.renderArea.extent.width = m_pFramework->ScreenRect.Width;
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
				m_pFramework->drawCmdBuffers[i],
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_FLAGS_NONE,
				0, nullptr,
				0, nullptr,
				1, &imageMemoryBarrier);

			vkCmdBeginRenderPass(m_pFramework->drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport viewport = vkTools::initializers::viewport((float)m_pFramework->ScreenRect.Width, (float)m_pFramework->ScreenRect.Height, 0.0f, 1.0f);
			vkCmdSetViewport(m_pFramework->drawCmdBuffers[i], 0, 1, &viewport);

			VkRect2D scissor = vkTools::initializers::rect2D(m_pFramework->ScreenRect.Width, m_pFramework->ScreenRect.Height, 0, 0);
			vkCmdSetScissor(m_pFramework->drawCmdBuffers[i], 0, 1, &scissor);

			VkDeviceSize offsets[1] = { 0 };
			vkCmdBindVertexBuffers(m_pFramework->drawCmdBuffers[i], VERTEX_BUFFER_BIND_ID, 1, &meshes.quad.vertices.buf, offsets);
			vkCmdBindIndexBuffer(m_pFramework->drawCmdBuffers[i], meshes.quad.indices.buf, 0, VK_INDEX_TYPE_UINT32);

			// Display ray traced image generated by compute shader as a full screen quad

			vkCmdBindDescriptorSets(m_pFramework->drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSetPostCompute, 0, NULL);
			vkCmdBindPipeline(m_pFramework->drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.display);

			vkCmdDrawIndexed(m_pFramework->drawCmdBuffers[i], meshes.quad.indexCount, 1, 0, 0, 0);

			vkCmdEndRenderPass(m_pFramework->drawCmdBuffers[i]);

			err = vkEndCommandBuffer(m_pFramework->drawCmdBuffers[i]);
			assert(!err);
		}

	}

	void buildComputeCommandBuffer()
	{
		VkCommandBufferBeginInfo cmdBufInfo = vkTools::initializers::commandBufferBeginInfo();

		VkResult err = vkBeginCommandBuffer(computeCmdBuffer, &cmdBufInfo);
		assert(!err);

		vkCmdBindPipeline(computeCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelines.compute);
		vkCmdBindDescriptorSets(computeCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 0, 1, &computeDescriptorSet, 0, 0);

		vkCmdDispatch(computeCmdBuffer, textureComputeTarget.width / 16, textureComputeTarget.height / 16, 1);

		vkEndCommandBuffer(computeCmdBuffer);
	}

	void draw()
	{
		VkResult err;

		// Get next image in the swap chain (back/front buffer)
		err = m_pFramework->swapChain.acquireNextImage(m_pFramework->semaphores.presentComplete, &m_pFramework->currentBuffer);
		assert(!err);

		m_pFramework->submitPostPresentBarrier(m_pFramework->swapChain.buffers[m_pFramework->currentBuffer].image);

		// Command buffer to be sumitted to the queue
		m_pFramework->submitInfo.commandBufferCount = 1;
		m_pFramework->submitInfo.pCommandBuffers = &m_pFramework->drawCmdBuffers[m_pFramework->currentBuffer];

		// Submit to queue
		err = vkQueueSubmit(m_pFramework->queue, 1, &m_pFramework->submitInfo, VK_NULL_HANDLE);
		assert(!err);

		m_pFramework->submitPrePresentBarrier(m_pFramework->swapChain.buffers[m_pFramework->currentBuffer].image);

		err = m_pFramework->swapChain.queuePresent(m_pFramework->queue, m_pFramework->currentBuffer, m_pFramework->semaphores.renderComplete);
		assert(!err);

		err = vkQueueWaitIdle(m_pFramework->queue);
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
		vertices.inputState.vertexBindingDescriptionCount = (uint32_t)vertices.bindingDescriptions.size();
		vertices.inputState.pVertexBindingDescriptions = vertices.bindingDescriptions.data();
		vertices.inputState.vertexAttributeDescriptionCount = (uint32_t)vertices.attributeDescriptions.size();
		vertices.inputState.pVertexAttributeDescriptions = vertices.attributeDescriptions.data();
	}

	void setupDescriptorPool()
	{
		std::vector<VkDescriptorPoolSize> poolSizes =
		{
			vkTools::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2),
			// Graphics pipeline uses image samplers for display
			vkTools::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4),
			// Compute pipeline uses storage images image loads and stores
			vkTools::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1),
		};

		VkDescriptorPoolCreateInfo descriptorPoolInfo =
			vkTools::initializers::descriptorPoolCreateInfo(
				(uint32_t)poolSizes.size(),
				poolSizes.data(),
				3);

		VkResult vkRes = vkCreateDescriptorPool(m_pFramework->device, &descriptorPoolInfo, nullptr, &m_pFramework->descriptorPool);
		assert(!vkRes);
	}

	void setupDescriptorSetLayout()
	{
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings =
		{
			// Binding 0 : Fragment shader image sampler
			vkTools::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_SHADER_STAGE_FRAGMENT_BIT,
				0)
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

		err = vkCreatePipelineLayout(m_pFramework->device, &pPipelineLayoutCreateInfo, nullptr, &pipelineLayout);
		assert(!err);
	}

	void setupDescriptorSet()
	{
		VkDescriptorSetAllocateInfo allocInfo =
			vkTools::initializers::descriptorSetAllocateInfo(
				m_pFramework->descriptorPool,
				&descriptorSetLayout,
				1);

		VkResult vkRes = vkAllocateDescriptorSets(m_pFramework->device, &allocInfo, &descriptorSetPostCompute);
		assert(!vkRes);

		// Image descriptor for the color map texture
		VkDescriptorImageInfo texDescriptor =
			vkTools::initializers::descriptorImageInfo(
				textureComputeTarget.sampler,
				textureComputeTarget.view,
				VK_IMAGE_LAYOUT_GENERAL);

		std::vector<VkWriteDescriptorSet> writeDescriptorSets =
		{
			// Binding 0 : Fragment shader texture sampler
			vkTools::initializers::writeDescriptorSet(
				descriptorSetPostCompute,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				0,
				&texDescriptor)
		};

		vkUpdateDescriptorSets(m_pFramework->device, (uint32_t)writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);
	}

	// Create a separate command buffer for compute commands
	void createComputeCommandBuffer()
	{
		VkCommandBufferAllocateInfo cmdBufAllocateInfo =
			vkTools::initializers::commandBufferAllocateInfo(
				m_pFramework->cmdPool,
				VK_COMMAND_BUFFER_LEVEL_PRIMARY,
				1);

		VkResult vkRes = vkAllocateCommandBuffers(m_pFramework->device, &cmdBufAllocateInfo, &computeCmdBuffer);
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
				(uint32_t)dynamicStateEnables.size(),
				0);

		// Display pipeline
		std::array<VkPipelineShaderStageCreateInfo,2> shaderStages;

		shaderStages[0] = m_pFramework->loadShader(m_pFramework->getAssetPath() + "shaders/raytracing/texture.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = m_pFramework->loadShader(m_pFramework->getAssetPath() + "shaders/raytracing/texture.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

		VkGraphicsPipelineCreateInfo pipelineCreateInfo =
			vkTools::initializers::pipelineCreateInfo(
				pipelineLayout,
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
		pipelineCreateInfo.renderPass = m_pFramework->renderPass;

		err = vkCreateGraphicsPipelines(m_pFramework->device, m_pFramework->pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.display);
		assert(!err);
	}

	// Prepare the compute pipeline that generates the ray traced image
	void prepareCompute()
	{
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
			// Binding 0 : Sampled image (write)
			vkTools::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
				VK_SHADER_STAGE_COMPUTE_BIT,
				0),
			// Binding 1 : Uniform buffer block
			vkTools::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				VK_SHADER_STAGE_COMPUTE_BIT,
				1)
		};

		VkDescriptorSetLayoutCreateInfo descriptorLayout =
			vkTools::initializers::descriptorSetLayoutCreateInfo(
				setLayoutBindings.data(),
				(uint32_t)setLayoutBindings.size());

		VkResult err = vkCreateDescriptorSetLayout(
			m_pFramework->device,
			&descriptorLayout,
			nullptr,
			&computeDescriptorSetLayout);
		assert(!err);

		VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo =
			vkTools::initializers::pipelineLayoutCreateInfo(
				&computeDescriptorSetLayout,
				1);

		err = vkCreatePipelineLayout(
			m_pFramework->device,
			&pPipelineLayoutCreateInfo,
			nullptr,
			&computePipelineLayout);
		assert(!err);

		VkDescriptorSetAllocateInfo allocInfo =
			vkTools::initializers::descriptorSetAllocateInfo(
				m_pFramework->descriptorPool,
				&computeDescriptorSetLayout,
				1);

		err = vkAllocateDescriptorSets(m_pFramework->device, &allocInfo, &computeDescriptorSet);
		assert(!err);

		std::vector<VkDescriptorImageInfo> computeTexDescriptors =
		{
			vkTools::initializers::descriptorImageInfo(
				VK_NULL_HANDLE,
				textureComputeTarget.view,
				VK_IMAGE_LAYOUT_GENERAL)
		};

		std::vector<VkWriteDescriptorSet> computeWriteDescriptorSets =
		{
			// Binding 0 : Output storage image
			vkTools::initializers::writeDescriptorSet(
				computeDescriptorSet,
				VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
				0,
				&computeTexDescriptors[0]),
			// Binding 1 : Uniform buffer block
			vkTools::initializers::writeDescriptorSet(
				computeDescriptorSet,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				1,
				&uniformDataCompute.descriptor)
		};

		vkUpdateDescriptorSets(m_pFramework->device, (uint32_t)computeWriteDescriptorSets.size(), computeWriteDescriptorSets.data(), 0, NULL);


		// Create compute shader pipelines
		VkComputePipelineCreateInfo computePipelineCreateInfo =
			vkTools::initializers::computePipelineCreateInfo(
				computePipelineLayout,
				0);

		computePipelineCreateInfo.stage = m_pFramework->loadShader(m_pFramework->getAssetPath() + "shaders/raytracing/raytracing.comp.spv", VK_SHADER_STAGE_COMPUTE_BIT);
		vkTools::checkResult(vkCreateComputePipelines(m_pFramework->device, m_pFramework->pipelineCache, 1, &computePipelineCreateInfo, nullptr, &pipelines.compute));
	}

	// Prepare and initialize uniform buffer containing shader uniforms
	void prepareUniformBuffers()
	{
		// Vertex shader uniform buffer block
		m_pFramework->createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			sizeof(uboCompute),
			&uboCompute,
			&uniformDataCompute.buffer,
			&uniformDataCompute.memory,
			&uniformDataCompute.descriptor);

		updateUniformBuffers();
	}

	void updateUniformBuffers()
	{
		uboCompute.lightPos.x = 0.0f + sin(glm::radians(m_pFramework->timer * 360.0f)) * 2.0f;
		uboCompute.lightPos.y = 5.0f;
		uboCompute.lightPos.z = 1.0f;
		uboCompute.lightPos.z = 0.0f + cos(glm::radians(m_pFramework->timer * 360.0f)) * 2.0f;
		uint8_t *pData;
		vkTools::checkResult(vkMapMemory(m_pFramework->device, uniformDataCompute.memory, 0, sizeof(uboCompute), 0, (void **)&pData));
		memcpy(pData, &uboCompute, sizeof(uboCompute));
		vkUnmapMemory(m_pFramework->device, uniformDataCompute.memory);
	}

	// Find and create a compute capable device queue
	void getComputeQueue()
	{
		uint32_t queueIndex = 0;
		uint32_t queueCount;
		vkGetPhysicalDeviceQueueFamilyProperties(m_pFramework->physicalDevice, &queueCount, NULL);
		assert(queueCount >= 1);

		std::vector<VkQueueFamilyProperties> queueProps;
		queueProps.resize(queueCount);
		vkGetPhysicalDeviceQueueFamilyProperties(m_pFramework->physicalDevice, &queueCount, queueProps.data());
		
		for (queueIndex = 0; queueIndex < queueCount; queueIndex++)
		{
			if (queueProps[queueIndex].queueFlags & VK_QUEUE_COMPUTE_BIT)
				break;
		}
		assert(queueIndex < queueCount);

		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.queueFamilyIndex = queueIndex;
		queueCreateInfo.queueCount = 1;
		vkGetDeviceQueue(m_pFramework->device, queueIndex, 0, &computeQueue);
	}

	virtual int32_t prepare()
	{
		CBaseVulkanGame::prepare();
		generateQuad();
		getComputeQueue();
		createComputeCommandBuffer();
		setupVertexDescriptions();
		prepareUniformBuffers();
		prepareTextureTarget(
			&textureComputeTarget, 
			TEX_DIM,
			TEX_DIM,
			VK_FORMAT_R8G8B8A8_UNORM);
		setupDescriptorSetLayout();
		preparePipelines();
		setupDescriptorPool();
		setupDescriptorSet();
		prepareCompute();
		buildCommandBuffers(); 
		buildComputeCommandBuffer();
		m_pFramework->prepared = true;
		return 0;
	}

	virtual int32_t render()
	{
		if (!m_pFramework->prepared)
			return 0;
		vkDeviceWaitIdle(m_pFramework->device);
		draw();
		vkDeviceWaitIdle(m_pFramework->device);
		if (!m_pFramework->paused)
		{
			updateUniformBuffers();
		}
		return 0;
	}

	virtual void viewChanged()
	{
		updateUniformBuffers();
	}
};

DEFINE_VULKAN_GAME_CREATE_AND_RELEASE_FUNCTIONS()
