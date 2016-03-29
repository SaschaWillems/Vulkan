/*
* Vulkan Example - Displacement mapping with tessellation shaders
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
#include <gli/gli.hpp>

#include <vulkan/vulkan.h>
#include "vulkanexamplebase.h"
#include "vulkanMeshLoader.hpp"

#define VERTEX_BUFFER_BIND_ID 0

// Vertex layout for this example
std::vector<vkMeshLoader::VertexLayout> vertexLayout =
{
	vkMeshLoader::VERTEX_LAYOUT_POSITION,
	vkMeshLoader::VERTEX_LAYOUT_NORMAL,
	vkMeshLoader::VERTEX_LAYOUT_UV
};

class VulkanExample : public CBaseVulkanGame
{
private:
	struct {
		vkTools::VulkanTexture colorMap;
		vkTools::VulkanTexture heightMap;
	} textures;
public:
	virtual int32_t			init(CVulkanFramework* pFramework)
	{
		CBaseVulkanGame::init(pFramework);
		m_pFramework->zoom = -35;
		m_pFramework->rotation = glm::vec3(-35.0, 0.0, 0);
		m_pFramework->title = "Vulkan Example - Tessellation shader displacement mapping";
		return 0;
	}

	bool splitScreen = true;

	struct {
		VkPipelineVertexInputStateCreateInfo inputState;
		std::vector<VkVertexInputBindingDescription> bindingDescriptions;
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
	} vertices;

	struct {
		vkMeshLoader::MeshBuffer object;
	} meshes;

	vkTools::UniformData uniformDataTC, uniformDataTE;

	struct {
		float tessLevel = 8.0;
	} uboTC;

	struct {
		glm::mat4 projection;
		glm::mat4 model;
		glm::vec4 lightPos = glm::vec4(0.0, -25.0, 0.0, 0.0);
		float tessAlpha = 1.0;
		float tessStrength = 1.0;
	} uboTE;

	struct {
		VkPipeline solid;
		VkPipeline wire;
		VkPipeline solidPassThrough;
		VkPipeline wirePassThrough;
	} pipelines;
	VkPipeline *pipelineLeft = &pipelines.solidPassThrough;
	VkPipeline *pipelineRight = &pipelines.solid;

	VkPipelineLayout pipelineLayout;
	VkDescriptorSet descriptorSet;
	VkDescriptorSetLayout descriptorSetLayout;

	VulkanExample()
	{
	}

	virtual ~VulkanExample()
	{
		// Clean up used Vulkan resources 
		// Note : Inherited destructor cleans up resources stored in base class
		vkDestroyPipeline(m_pFramework->device, pipelines.solid, nullptr);
		vkDestroyPipeline(m_pFramework->device, pipelines.wire, nullptr);
		vkDestroyPipeline(m_pFramework->device, pipelines.solidPassThrough, nullptr);
		vkDestroyPipeline(m_pFramework->device, pipelines.wirePassThrough, nullptr);

		vkDestroyPipelineLayout(m_pFramework->device, pipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(m_pFramework->device, descriptorSetLayout, nullptr);

		vkMeshLoader::freeMeshBufferResources(m_pFramework->device, &meshes.object);

		vkDestroyBuffer(m_pFramework->device, uniformDataTC.buffer, nullptr);
		vkFreeMemory(m_pFramework->device, uniformDataTC.memory, nullptr);

		vkDestroyBuffer(m_pFramework->device, uniformDataTE.buffer, nullptr);
		vkFreeMemory(m_pFramework->device, uniformDataTE.memory, nullptr);

		m_pFramework->textureLoader->destroyTexture(textures.colorMap);
		m_pFramework->textureLoader->destroyTexture(textures.heightMap);
	}

	void loadTextures()
	{
		m_pFramework->textureLoader->loadTexture(
			m_pFramework->getAssetPath() + "textures/stonewall_colormap_bc3.dds", 
			VK_FORMAT_BC3_UNORM_BLOCK, 
			&textures.colorMap);
		m_pFramework->textureLoader->loadTexture(
			m_pFramework->getAssetPath() + "textures/stonewall_heightmap_rgba.dds", 
			VK_FORMAT_R8G8B8A8_UNORM, 
			&textures.heightMap);
	}

	void reBuildCommandBuffers()
	{
		if (!m_pFramework->checkCommandBuffers())
		{
			m_pFramework->destroyCommandBuffers();
			m_pFramework->createCommandBuffers();
		}
		buildCommandBuffers();
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
				splitScreen ? (float)m_pFramework->ScreenRect.Width / 2.0f : (float)m_pFramework->ScreenRect.Width,
				(float)m_pFramework->ScreenRect.Height,
				0.0f,
				1.0f
				);
			vkCmdSetViewport(m_pFramework->drawCmdBuffers[i], 0, 1, &viewport);

			VkRect2D scissor = vkTools::initializers::rect2D(
				m_pFramework->ScreenRect.Width,
				m_pFramework->ScreenRect.Height,
				0,
				0
				);
			vkCmdSetScissor(m_pFramework->drawCmdBuffers[i], 0, 1, &scissor);

			vkCmdSetLineWidth(m_pFramework->drawCmdBuffers[i], 1.0f);

			vkCmdBindDescriptorSets(m_pFramework->drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);

			VkDeviceSize offsets[1] = { 0 };
			vkCmdBindVertexBuffers(m_pFramework->drawCmdBuffers[i], VERTEX_BUFFER_BIND_ID, 1, &meshes.object.vertices.buf, offsets);
			vkCmdBindIndexBuffer(m_pFramework->drawCmdBuffers[i], meshes.object.indices.buf, 0, VK_INDEX_TYPE_UINT32);

			if (splitScreen)
			{
				vkCmdSetViewport(m_pFramework->drawCmdBuffers[i], 0, 1, &viewport);
				vkCmdBindPipeline(m_pFramework->drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, *pipelineLeft);
				vkCmdDrawIndexed(m_pFramework->drawCmdBuffers[i], meshes.object.indexCount, 1, 0, 0, 0);
				viewport.x = float(m_pFramework->ScreenRect.Width) / 2;
			}

			vkCmdSetViewport(m_pFramework->drawCmdBuffers[i], 0, 1, &viewport);
			vkCmdBindPipeline(m_pFramework->drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, *pipelineRight);
			vkCmdDrawIndexed(m_pFramework->drawCmdBuffers[i], meshes.object.indexCount, 1, 0, 0, 0);

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
	}

	void loadMeshes()
	{
		m_pFramework->loadMesh(m_pFramework->getAssetPath() + "models/torus.obj", &meshes.object, vertexLayout, 0.25f);
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
		// Describes memory layout and shader positions
		vertices.attributeDescriptions.resize(3);

		// Location 0 : Position
		vertices.attributeDescriptions[0] =
			vkTools::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				0,
				VK_FORMAT_R32G32B32_SFLOAT,
				0);

		// Location 1 : Normals
		vertices.attributeDescriptions[1] =
			vkTools::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				1,
				VK_FORMAT_R32G32B32_SFLOAT,
				sizeof(float) * 3);

		// Location 2 : Texture coordinates
		vertices.attributeDescriptions[2] =
			vkTools::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				2,
				VK_FORMAT_R32G32_SFLOAT,
				sizeof(float) * 6);

		vertices.inputState = vkTools::initializers::pipelineVertexInputStateCreateInfo();
		vertices.inputState.vertexBindingDescriptionCount = (uint32_t)vertices.bindingDescriptions.size();
		vertices.inputState.pVertexBindingDescriptions = vertices.bindingDescriptions.data();
		vertices.inputState.vertexAttributeDescriptionCount = (uint32_t)vertices.attributeDescriptions.size();
		vertices.inputState.pVertexAttributeDescriptions = vertices.attributeDescriptions.data();
	}

	void setupDescriptorPool()
	{
		// Example uses two ubos and two image samplers
		std::vector<VkDescriptorPoolSize> poolSizes =
		{
			vkTools::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2),
			vkTools::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2)
		};

		VkDescriptorPoolCreateInfo descriptorPoolInfo =
			vkTools::initializers::descriptorPoolCreateInfo(
				(uint32_t)poolSizes.size(),
				poolSizes.data(),
				2);

		VkResult vkRes = vkCreateDescriptorPool(m_pFramework->device, &descriptorPoolInfo, nullptr, &m_pFramework->descriptorPool);
		assert(!vkRes);
	}

	void setupDescriptorSetLayout()
	{
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings =
		{
			// Binding 0 : Tessellation control shader ubo
			vkTools::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 
				VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, 
				0),
			// Binding 1 : Tessellation evaluation shader ubo
			vkTools::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, 
				1),
			// Binding 2 : Tessellation evaluation shader displacement map image sampler
			vkTools::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
				2),
			// Binding 3 : Fragment shader color map image sampler
			vkTools::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_SHADER_STAGE_FRAGMENT_BIT,
				3),
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

		VkResult vkRes = vkAllocateDescriptorSets(m_pFramework->device, &allocInfo, &descriptorSet);
		assert(!vkRes);

		// Displacement map image descriptor
		VkDescriptorImageInfo texDescriptorDisplacementMap =
			vkTools::initializers::descriptorImageInfo(
				textures.heightMap.sampler,
				textures.heightMap.view,
				VK_IMAGE_LAYOUT_GENERAL);

		// Color map image descriptor
		VkDescriptorImageInfo texDescriptorColorMap =
			vkTools::initializers::descriptorImageInfo(
				textures.colorMap.sampler,
				textures.colorMap.view,
				VK_IMAGE_LAYOUT_GENERAL);

		std::vector<VkWriteDescriptorSet> writeDescriptorSets =
		{
			// Binding 0 : Tessellation control shader ubo
			vkTools::initializers::writeDescriptorSet(
				descriptorSet, 
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 
				0, 
				&uniformDataTC.descriptor),
			// Binding 1 : Tessellation evaluation shader ubo
			vkTools::initializers::writeDescriptorSet(
				descriptorSet, 
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				1, 
				&uniformDataTE.descriptor),
			// Binding 2 : Displacement map
			vkTools::initializers::writeDescriptorSet(
				descriptorSet,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				2,
				&texDescriptorDisplacementMap),
			// Binding 3 : Color map
			vkTools::initializers::writeDescriptorSet(
				descriptorSet,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				3,
				&texDescriptorColorMap),
		};

		vkUpdateDescriptorSets(m_pFramework->device, (uint32_t)writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);
	}

	void preparePipelines()
	{
		VkResult err;

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
			vkTools::initializers::pipelineInputAssemblyStateCreateInfo(
				VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,
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
			VK_DYNAMIC_STATE_SCISSOR,
			VK_DYNAMIC_STATE_LINE_WIDTH
		};
		VkPipelineDynamicStateCreateInfo dynamicState =
			vkTools::initializers::pipelineDynamicStateCreateInfo(
				dynamicStateEnables.data(),
				(uint32_t)dynamicStateEnables.size(),
				0);

		VkPipelineTessellationStateCreateInfo tessellationState =
			vkTools::initializers::pipelineTessellationStateCreateInfo(3);

		// Tessellation pipeline
		// Load shaders
		std::array<VkPipelineShaderStageCreateInfo, 4> shaderStages;
		shaderStages[0] = m_pFramework->loadShader(m_pFramework->getAssetPath() + "shaders/displacement/base.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = m_pFramework->loadShader(m_pFramework->getAssetPath() + "shaders/displacement/base.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		shaderStages[2] = m_pFramework->loadShader(m_pFramework->getAssetPath() + "shaders/displacement/displacement.tesc.spv", VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);
		shaderStages[3] = m_pFramework->loadShader(m_pFramework->getAssetPath() + "shaders/displacement/displacement.tese.spv", VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);

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
		pipelineCreateInfo.pTessellationState = &tessellationState;
		pipelineCreateInfo.stageCount = (uint32_t)shaderStages.size();
		pipelineCreateInfo.pStages = shaderStages.data();
		pipelineCreateInfo.renderPass = m_pFramework->renderPass;

		// Solid pipeline
		err = vkCreateGraphicsPipelines(m_pFramework->device, m_pFramework->pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.solid);
		assert(!err);
		// Wireframe pipeline
		rasterizationState.polygonMode = VK_POLYGON_MODE_LINE;
		err = vkCreateGraphicsPipelines(m_pFramework->device, m_pFramework->pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.wire);
		assert(!err);

		// Pass through pipelines
		// Load pass through tessellation shaders (Vert and frag are reused)
		shaderStages[2] = m_pFramework->loadShader(m_pFramework->getAssetPath() + "shaders/displacement/passthrough.tesc.spv", VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);
		shaderStages[3] = m_pFramework->loadShader(m_pFramework->getAssetPath() + "shaders/displacement/passthrough.tese.spv", VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);
		// Solid
		rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
		err = vkCreateGraphicsPipelines(m_pFramework->device, m_pFramework->pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.solidPassThrough);
		assert(!err);
		// Wireframe
		rasterizationState.polygonMode = VK_POLYGON_MODE_LINE;
		err = vkCreateGraphicsPipelines(m_pFramework->device, m_pFramework->pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.wirePassThrough);
		assert(!err);
	}

	// Prepare and initialize uniform buffer containing shader uniforms
	void prepareUniformBuffers()
	{
		// Tessellation evaluation shader uniform buffer
		m_pFramework->createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			sizeof(uboTE),
			&uboTE,
			&uniformDataTE.buffer,
			&uniformDataTE.memory,
			&uniformDataTE.descriptor);

		// Tessellation control shader uniform buffer
		m_pFramework->createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			sizeof(uboTC),
			&uboTC,
			&uniformDataTC.buffer,
			&uniformDataTC.memory,
			&uniformDataTC.descriptor);

		updateUniformBuffers();
	}

	void updateUniformBuffers()
	{
		// Tessellation eval
		glm::mat4 viewMatrix = glm::mat4();
		uboTE.projection = glm::perspective(deg_to_rad(45.0f), (float)(m_pFramework->ScreenRect.Width* ((splitScreen) ? 0.5f : 1.0f)) / (float)m_pFramework->ScreenRect.Height, 0.1f, 256.0f);
		viewMatrix = glm::translate(viewMatrix, glm::vec3(0.0f, 0.0f, m_pFramework->zoom));

		float offset = 0.5f;
		int uboIndex = 1;
		uboTE.model = glm::mat4();
		uboTE.model = viewMatrix * glm::translate(uboTE.model, glm::vec3(0, 0, 0));
		uboTE.model = glm::rotate(uboTE.model, glm::radians(m_pFramework->rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		uboTE.model = glm::rotate(uboTE.model, glm::radians(m_pFramework->rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		uboTE.model = glm::rotate(uboTE.model, glm::radians(m_pFramework->rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

		uint8_t *pData;
		VkResult err = vkMapMemory(m_pFramework->device, uniformDataTE.memory, 0, sizeof(uboTE), 0, (void **)&pData);
		assert(!err);
		memcpy(pData, &uboTE, sizeof(uboTE));
		vkUnmapMemory(m_pFramework->device, uniformDataTE.memory);

		// Tessellation control
		err = vkMapMemory(m_pFramework->device, uniformDataTC.memory, 0, sizeof(uboTC), 0, (void **)&pData);
		assert(!err);
		memcpy(pData, &uboTC, sizeof(uboTC));
		vkUnmapMemory(m_pFramework->device, uniformDataTC.memory);
	}

	virtual int32_t	prepare()
	{
		
		loadMeshes();
		loadTextures();
		setupVertexDescriptions();
		prepareUniformBuffers();
		setupDescriptorSetLayout();
		preparePipelines();
		setupDescriptorPool();
		setupDescriptorSet();
		buildCommandBuffers();
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
		return 0;
	}

	virtual void viewChanged()
	{
		updateUniformBuffers();
	}

	void changeTessellationLevel(float delta)
	{
		uboTC.tessLevel += delta;
		// Clamp
		uboTC.tessLevel = (float)fmax(1.0, fmin(uboTC.tessLevel, 32.0));
		updateUniformBuffers();
	}

	void togglePipelines()
	{
		if (pipelineRight == &pipelines.solid)
		{
			pipelineRight = &pipelines.wire;
			pipelineLeft = &pipelines.wirePassThrough;
		}
		else
		{
			pipelineRight = &pipelines.solid;
			pipelineLeft = &pipelines.solidPassThrough;
		}
		reBuildCommandBuffers();
	}

	void toggleSplitScreen()
	{
		splitScreen = !splitScreen;
		reBuildCommandBuffers();
		updateUniformBuffers();
	}

	virtual void	keyPressed(uint32_t keyCode)
	{
		switch (keyCode)
		{
		case VK_ADD:
			changeTessellationLevel(0.25);
			break;
		case VK_SUBTRACT:
			changeTessellationLevel(-0.25);
			break;
		case 0x57:
			togglePipelines();
			break;
		case 0x53:
			toggleSplitScreen();
			break;
		}
	}


};

DEFINE_VULKAN_GAME_CREATE_AND_RELEASE_FUNCTIONS()
