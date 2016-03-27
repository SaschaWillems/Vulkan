/*
* Vulkan Example - Parallax Mapping
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
#include <glm/gtc/matrix_inverse.hpp>

#include <vulkan/vulkan.h>
#include "vulkanexamplebase.h"

#define VERTEX_BUFFER_BIND_ID 0
#define ENABLE_VALIDATION false

// Vertex layout for this example
std::vector<vkMeshLoader::VertexLayout> vertexLayout =
{
	vkMeshLoader::VERTEX_LAYOUT_POSITION,
	vkMeshLoader::VERTEX_LAYOUT_UV,
	vkMeshLoader::VERTEX_LAYOUT_NORMAL,
	vkMeshLoader::VERTEX_LAYOUT_TANGENT,
	vkMeshLoader::VERTEX_LAYOUT_BITANGENT
};

class VulkanExample : public CBaseVulkanGame
{
public:
	virtual int32_t			init(CVulkanFramework* pFramework)
	{
		CBaseVulkanGame::init(pFramework);
		m_pFramework->zoom = -1.25f;
		m_pFramework->rotation = glm::vec3(40.0, -33.0, 0.0);
		m_pFramework->rotationSpeed = 0.25f;
		m_pFramework->paused = true;
		m_pFramework->title = "Vulkan Example - Parallax Mapping";
		// Values not set here are initialized in the base class constructor
		return 0;
	};

	bool splitScreen = true;

	struct {
		vkTools::VulkanTexture colorMap;
		// Normals and height are combined in one texture (height = alpha channel)
		vkTools::VulkanTexture normalHeightMap;
	} textures;

	struct {
		VkPipelineVertexInputStateCreateInfo inputState;
		std::vector<VkVertexInputBindingDescription> bindingDescriptions;
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
	} vertices;

	struct {
		vkMeshLoader::MeshBuffer quad;
	} meshes;

	struct {
		vkTools::UniformData vertexShader;
		vkTools::UniformData fragmentShader;
	} uniformData;

	struct {

		struct {
			glm::mat4 projection;
			glm::mat4 model;
			glm::mat4 normal;
			glm::vec4 lightPos = glm::vec4(0.0, 0.0, 0.0, 0.0);
			glm::vec4 cameraPos;
		} vertexShader;

		struct {
			// Scale and bias control the parallax offset effect
			// They need to be tweaked for each material
			// Getting them wrong destroys the depth effect
			float scale = 0.06f;
			float bias = -0.04f;
			float lightRadius = 1.0f;
			int32_t usePom = 1;
			int32_t displayNormalMap = 0;
		} fragmentShader;

	} ubos;

	struct {
		VkPipeline parallaxMapping;
		VkPipeline normalMapping;
	} pipelines;

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
		vkDestroyPipeline(m_pFramework->device, pipelines.parallaxMapping, nullptr);
		vkDestroyPipeline(m_pFramework->device, pipelines.normalMapping, nullptr);

		vkDestroyPipelineLayout(m_pFramework->device, pipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(m_pFramework->device, descriptorSetLayout, nullptr);

		vkMeshLoader::freeMeshBufferResources(m_pFramework->device, &meshes.quad);

		vkTools::destroyUniformData(m_pFramework->device, &uniformData.vertexShader);
		vkTools::destroyUniformData(m_pFramework->device, &uniformData.fragmentShader);

		m_pFramework->textureLoader->destroyTexture(textures.colorMap);
		m_pFramework->textureLoader->destroyTexture(textures.normalHeightMap);
	}

	void loadTextures()
	{
		m_pFramework->textureLoader->loadTexture(
			m_pFramework->getAssetPath() + "textures/rocks_color_bc3.dds",
			VK_FORMAT_BC3_UNORM_BLOCK, 
			&textures.colorMap);
		m_pFramework->textureLoader->loadTexture(
			m_pFramework->getAssetPath() + "textures/rocks_normal_height_rgba.dds", 
			VK_FORMAT_R8G8B8A8_UNORM, 
			&textures.normalHeightMap);
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
				(splitScreen) ? (float)m_pFramework->ScreenRect.Width / 2.0f : (float)m_pFramework->ScreenRect.Width,
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

			vkCmdBindDescriptorSets(m_pFramework->drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);

			VkDeviceSize offsets[1] = { 0 };
			vkCmdBindVertexBuffers(m_pFramework->drawCmdBuffers[i], VERTEX_BUFFER_BIND_ID, 1, &meshes.quad.vertices.buf, offsets);
			vkCmdBindIndexBuffer(m_pFramework->drawCmdBuffers[i], meshes.quad.indices.buf, 0, VK_INDEX_TYPE_UINT32);

			// Parallax enabled
			vkCmdSetViewport(m_pFramework->drawCmdBuffers[i], 0, 1, &viewport);
			vkCmdBindPipeline(m_pFramework->drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.parallaxMapping);
			vkCmdDrawIndexed(m_pFramework->drawCmdBuffers[i], meshes.quad.indexCount, 1, 0, 0, 1);

			// Normal mapping
			if (splitScreen)
			{
				viewport.x = (float)m_pFramework->ScreenRect.Width / 2.0f;
				vkCmdSetViewport(m_pFramework->drawCmdBuffers[i], 0, 1, &viewport);
				vkCmdBindPipeline(m_pFramework->drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.normalMapping);
				vkCmdDrawIndexed(m_pFramework->drawCmdBuffers[i], meshes.quad.indexCount, 1, 0, 0, 1);
			}

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
		m_pFramework->loadMesh(m_pFramework->getAssetPath() + "models/plane_z.obj", &meshes.quad, vertexLayout, 0.1f);
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
		vertices.attributeDescriptions.resize(5);
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
		// Location 2 : Normal
		vertices.attributeDescriptions[2] =
			vkTools::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				2,
				VK_FORMAT_R32G32B32_SFLOAT,
				sizeof(float) * 5);
		// Location 3 : Tangent
		vertices.attributeDescriptions[3] =
			vkTools::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				3,
				VK_FORMAT_R32G32B32_SFLOAT,
				sizeof(float) * 8);
		// Location 4 : Bitangent
		vertices.attributeDescriptions[4] =
			vkTools::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				4,
				VK_FORMAT_R32G32B32_SFLOAT,
				sizeof(float) * 11);

		vertices.inputState = vkTools::initializers::pipelineVertexInputStateCreateInfo();
		vertices.inputState.vertexBindingDescriptionCount = vertices.bindingDescriptions.size();
		vertices.inputState.pVertexBindingDescriptions = vertices.bindingDescriptions.data();
		vertices.inputState.vertexAttributeDescriptionCount = vertices.attributeDescriptions.size();
		vertices.inputState.pVertexAttributeDescriptions = vertices.attributeDescriptions.data();
	}

	void setupDescriptorPool()
	{
		// Example uses two ubos and two image sampler
		std::vector<VkDescriptorPoolSize> poolSizes =
		{
			vkTools::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2),
			vkTools::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2)
		};

		VkDescriptorPoolCreateInfo descriptorPoolInfo =
			vkTools::initializers::descriptorPoolCreateInfo(
				poolSizes.size(),
				poolSizes.data(),
				4);

		VkResult vkRes = vkCreateDescriptorPool(m_pFramework->device, &descriptorPoolInfo, nullptr, &m_pFramework->descriptorPool);
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
			// Binding 1 : Fragment shader color map image sampler
			vkTools::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_SHADER_STAGE_FRAGMENT_BIT,
				1),
			// Binding 2 : Fragment combined normal and heightmap
			vkTools::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_SHADER_STAGE_FRAGMENT_BIT,
				2),
			// Binding 3 : Fragment shader uniform buffer
			vkTools::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				VK_SHADER_STAGE_FRAGMENT_BIT,
				3)
		};

		VkDescriptorSetLayoutCreateInfo descriptorLayout =
			vkTools::initializers::descriptorSetLayoutCreateInfo(
				setLayoutBindings.data(),
				setLayoutBindings.size());

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

		// Color map image descriptor
		VkDescriptorImageInfo texDescriptorColorMap =
			vkTools::initializers::descriptorImageInfo(
				textures.colorMap.sampler,
				textures.colorMap.view,
				VK_IMAGE_LAYOUT_GENERAL);

		VkDescriptorImageInfo texDescriptorNormalHeightMap =
			vkTools::initializers::descriptorImageInfo(
				textures.normalHeightMap.sampler,
				textures.normalHeightMap.view,
				VK_IMAGE_LAYOUT_GENERAL);

		std::vector<VkWriteDescriptorSet> writeDescriptorSets =
		{
			// Binding 0 : Vertex shader uniform buffer
			vkTools::initializers::writeDescriptorSet(
				descriptorSet,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				0,
				&uniformData.vertexShader.descriptor),
			// Binding 1 : Fragment shader image sampler
			vkTools::initializers::writeDescriptorSet(
				descriptorSet,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				1,
				&texDescriptorColorMap),
			// Binding 2 : Combined normal and heightmap
			vkTools::initializers::writeDescriptorSet(
				descriptorSet,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				2,
				&texDescriptorNormalHeightMap),
			// Binding 3 : Fragment shader uniform buffer
			vkTools::initializers::writeDescriptorSet(
				descriptorSet,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				3,
				&uniformData.fragmentShader.descriptor)
		};

		vkUpdateDescriptorSets(m_pFramework->device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);
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

		// Parallax mapping pipeline
		// Load shaders
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;
		shaderStages[0] = m_pFramework->loadShader(m_pFramework->getAssetPath() + "shaders/parallax/parallax.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = m_pFramework->loadShader(m_pFramework->getAssetPath() + "shaders/parallax/parallax.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

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
		pipelineCreateInfo.stageCount = shaderStages.size();
		pipelineCreateInfo.pStages = shaderStages.data();

		VkResult err = vkCreateGraphicsPipelines(m_pFramework->device, m_pFramework->pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.parallaxMapping);
		assert(!err);

		// Normal mapping (no parallax effect)
		shaderStages[0] = m_pFramework->loadShader(m_pFramework->getAssetPath() + "shaders/parallax/normalmap.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = m_pFramework->loadShader(m_pFramework->getAssetPath() + "shaders/parallax/normalmap.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		err = vkCreateGraphicsPipelines(m_pFramework->device, m_pFramework->pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.normalMapping);
		assert(!err);
	}

	void prepareUniformBuffers()
	{
		// Vertex shader ubo
		m_pFramework->createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			sizeof(ubos.vertexShader),
			&ubos.vertexShader,
			&uniformData.vertexShader.buffer,
			&uniformData.vertexShader.memory,
			&uniformData.vertexShader.descriptor);

		// Fragment shader ubo
		m_pFramework->createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			sizeof(ubos.fragmentShader),
			&ubos.fragmentShader,
			&uniformData.fragmentShader.buffer,
			&uniformData.fragmentShader.memory,
			&uniformData.fragmentShader.descriptor);

		updateUniformBuffers();
	}

	void updateUniformBuffers()
	{
		// Vertex shader
		glm::mat4 viewMatrix = glm::mat4();

		ubos.vertexShader.projection = glm::perspective(deg_to_rad(45.0f), (float)(m_pFramework->ScreenRect.Width* ((splitScreen) ? 0.5f : 1.0f)) / (float)m_pFramework->ScreenRect.Height, 0.001f, 256.0f);
		viewMatrix = glm::translate(viewMatrix, glm::vec3(0.0f, 0.0f, m_pFramework->zoom));

		ubos.vertexShader.model = glm::mat4();
		ubos.vertexShader.model = viewMatrix * glm::translate(ubos.vertexShader.model, glm::vec3(0, 0, 0));
		ubos.vertexShader.model = glm::rotate(ubos.vertexShader.model, glm::radians(m_pFramework->rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		ubos.vertexShader.model = glm::rotate(ubos.vertexShader.model, glm::radians(m_pFramework->rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		ubos.vertexShader.model = glm::rotate(ubos.vertexShader.model, glm::radians(m_pFramework->rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

		ubos.vertexShader.normal = glm::inverseTranspose(ubos.vertexShader.model);

		if (!m_pFramework->paused)
		{
			ubos.vertexShader.lightPos.x = sin(glm::radians(m_pFramework->timer * 360.0f)) * 0.5;
			ubos.vertexShader.lightPos.y = cos(glm::radians(m_pFramework->timer * 360.0f)) * 0.5;
		}

		ubos.vertexShader.cameraPos = glm::vec4(0.0, 0.0, m_pFramework->zoom, 0.0);

		uint8_t *pData;
		VkResult err = vkMapMemory(m_pFramework->device, uniformData.vertexShader.memory, 0, sizeof(ubos.vertexShader), 0, (void **)&pData);
		assert(!err);
		memcpy(pData, &ubos.vertexShader, sizeof(ubos.vertexShader));
		vkUnmapMemory(m_pFramework->device, uniformData.vertexShader.memory);

		// Fragment shader
		err = vkMapMemory(m_pFramework->device, uniformData.fragmentShader.memory, 0, sizeof(ubos.fragmentShader), 0, (void **)&pData);
		assert(!err);
		memcpy(pData, &ubos.fragmentShader, sizeof(ubos.fragmentShader));
		vkUnmapMemory(m_pFramework->device, uniformData.fragmentShader.memory);
	}

	int32_t	prepare()
	{
		
		loadTextures();
		loadMeshes();
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

	void toggleParallaxOffset()
	{
		ubos.fragmentShader.usePom = !ubos.fragmentShader.usePom;
		updateUniformBuffers();
	}

	void toggleNormalMapDisplay()
	{
		ubos.fragmentShader.displayNormalMap = !ubos.fragmentShader.displayNormalMap;
		updateUniformBuffers();
	}

	void toggleSplitScreen()
	{
		splitScreen = !splitScreen;
		updateUniformBuffers();
		reBuildCommandBuffers();
	}

	virtual void	keyPressed(uint32_t keyCode)
	{
		switch (keyCode)
		{
		case 0x4F:
			toggleParallaxOffset();
			break;
		case 0x4E:
			toggleNormalMapDisplay();
			break;
		case 0x53:
			toggleSplitScreen();
			break;
		}
	}

};

DEFINE_VULKAN_GAME_CREATE_AND_RELEASE_FUNCTIONS()
