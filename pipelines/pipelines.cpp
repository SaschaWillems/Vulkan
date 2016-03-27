/*
* Vulkan Example - Using different pipelines in one single renderpass
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

#define VERTEX_BUFFER_BIND_ID 0
#define ENABLE_VALIDATION false

// Vertex layout for this example
struct Vertex {
	float pos[3];
	float col[3];
	float uv[2];
	float normal[3];
};

class VulkanExample: public CBaseVulkanGame
{
private:
	vkTools::VulkanTexture textureColorMap;
public:
	virtual int32_t			init(CVulkanFramework* pFramework)
	{
		CBaseVulkanGame::init(pFramework);
		m_pFramework->zoom = -5.0f;
		m_pFramework->rotation = glm::vec3(-32.5f, 45.0f, 0.0f);
		m_pFramework->title = "Vulkan Example - Using pipelines";
		// Values not set here are initialized in the base class constructor
		return 0;
	};

	struct {
		int count;
		VkPipelineVertexInputStateCreateInfo inputState;
		std::vector<VkVertexInputBindingDescription> bindingDescriptions;
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
	} vertices;

	struct {
		vkMeshLoader::MeshBuffer cube;
	} meshes;

	vkTools::UniformData uniformDataVS;

	// Same uniform buffer layout as shader
	struct {
		glm::mat4 projectionMatrix;
		glm::mat4 modelMatrix;
		glm::mat4 viewMatrix;
	} uboVS;

	VkPipelineLayout pipelineLayout;
	VkDescriptorSet descriptorSet;
	VkDescriptorSetLayout descriptorSetLayout;

	struct {
		VkPipeline solidColor;
		VkPipeline wireFrame;
		VkPipeline texture;
	} pipelines;

	VulkanExample()
	{
	}

	virtual ~VulkanExample()
	{
		// Clean up used Vulkan resources 
		// Note : Inherited destructor cleans up resources stored in base class
		vkDestroyPipeline(m_pFramework->device, pipelines.solidColor, nullptr);
		vkDestroyPipeline(m_pFramework->device, pipelines.wireFrame, nullptr);
		vkDestroyPipeline(m_pFramework->device, pipelines.texture, nullptr);
		
		vkDestroyPipelineLayout(m_pFramework->device, pipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(m_pFramework->device, descriptorSetLayout, nullptr);

		vkMeshLoader::freeMeshBufferResources(m_pFramework->device, &meshes.cube);

		vkDestroyBuffer(m_pFramework->device, uniformDataVS.buffer, nullptr);
		vkFreeMemory(m_pFramework->device, uniformDataVS.memory, nullptr);

		m_pFramework->textureLoader->destroyTexture(textureColorMap);
	}

	void loadTextures()
	{
		m_pFramework->textureLoader->loadTexture(
			m_pFramework->getAssetPath() + "textures/crate_bc3.ktx", 
			VK_FORMAT_BC3_UNORM_BLOCK, 
			&textureColorMap);
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

			vkCmdBindDescriptorSets(m_pFramework->drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);

			VkDeviceSize offsets[1] = { 0 };
			vkCmdBindVertexBuffers(m_pFramework->drawCmdBuffers[i], VERTEX_BUFFER_BIND_ID, 1, &meshes.cube.vertices.buf, offsets);

			vkCmdSetLineWidth(m_pFramework->drawCmdBuffers[i], 2.0f);

			// Left : Solid colored 
			viewport.width = (float)(m_pFramework->ScreenRect.Width / 3.0);
			vkCmdSetViewport(m_pFramework->drawCmdBuffers[i], 0, 1, &viewport);
			vkCmdBindPipeline(m_pFramework->drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.solidColor);
			
			vkCmdDraw(m_pFramework->drawCmdBuffers[i], vertices.count, 1, 0, 0);

			// Center : Textured
			viewport.x = (float)(m_pFramework->ScreenRect.Width / 3.0);
			vkCmdSetViewport(m_pFramework->drawCmdBuffers[i], 0, 1, &viewport);
			vkCmdBindPipeline(m_pFramework->drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.texture);
			vkCmdSetLineWidth(m_pFramework->drawCmdBuffers[i], 2.0f);
			vkCmdDraw(m_pFramework->drawCmdBuffers[i], vertices.count, 1, 0, 0);

			// Right : Wireframe 
			viewport.x = (float)(m_pFramework->ScreenRect.Width / 3.0 + m_pFramework->ScreenRect.Width / 3.0);
			vkCmdSetViewport(m_pFramework->drawCmdBuffers[i], 0, 1, &viewport);
			vkCmdBindPipeline(m_pFramework->drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.wireFrame);
			vkCmdDraw(m_pFramework->drawCmdBuffers[i], vertices.count, 1, 0, 0);

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

	// Create vertices and buffers for uv mapped cube
	void generateCube()
	{

		// Setup vertices
#define colred { 1.0f, 0.0f, 0.0f }
#define colgreen { 0.0f, 1.0f, 0.0f }
#define colblue { 0.0f, 0.0f, 1.0f }
#define d 1.0f

		std::vector<Vertex> vertexBuffer = {

			// -Y
			{ { d,-d, d }, colred,{ 1.0, 1.0 }, { 0.0f, 1.0f, 0.0f } },
			{ { -d,-d,-d }, colred,{ 0.0, 0.0 },{ 0.0f, 1.0f, 0.0f } },
			{ { d,-d,-d }, colred,{ 1.0, 0.0 },{ 0.0f, 1.0f, 0.0f } },
			{ { d,-d, d }, colred,{ 1.0, 1.0 },{ 0.0f, 1.0f, 0.0f } },
			{ { -d,-d, d }, colred,{ 0.0, 1.0 },{ 0.0f, 1.0f, 0.0f } },
			{ { -d,-d,-d }, colred,{ 0.0, 0.0 },{ 0.0f, 1.0f, 0.0f } },
			// +Y
			{ { d, d, d }, colred,{ 1.0, 1.0 },{ 0.0f, -1.0f, 0.0f } },
			{ { d, d,-d }, colred,{ 1.0, 0.0 },{ 0.0f, -1.0f, 0.0f } },
			{ { -d, d,-d }, colred,{ 0.0, 0.0 },{ 0.0f, -1.0f, 0.0f } },
			{ { d, d, d }, colred,{ 1.0, 1.0 },{ 0.0f, -1.0f, 0.0f } },
			{ { -d, d,-d }, colred,{ 0.0, 0.0 },{ 0.0f, -1.0f, 0.0f } },
			{ { -d, d, d }, colred,{ 0.0, 1.0 },{ 0.0f, -1.0f, 0.0f } },
			// -X
			{ { -d,-d,-d }, colblue,{ 0.0, 0.0 },{ -1.0f, 0.0f, 0.0f } },
			{ { -d,-d, d }, colblue,{ 0.0, 1.0 },{ -1.0f, 0.0f, 0.0f } },
			{ { -d, d, d }, colblue,{ 1.0, 1.0 },{ -1.0f, 0.0f, 0.0f } },
			{ { -d,-d,-d }, colblue,{ 0.0, 0.0 },{ -1.0f, 0.0f, 0.0f } },
			{ { -d, d, d }, colblue,{ 1.0, 1.0 },{ -1.0f, 0.0f, 0.0f } },
			{ { -d, d,-d }, colblue,{ 1.0, 0.0 },{ -1.0f, 0.0f, 0.0f } },
			// +X
			{ { d, d, d }, colblue,{ 1.0, 1.0 },{ 1.0f, 0.0f, 0.0f } },
			{ { d,-d,-d }, colblue,{ 0.0, 0.0 },{ 1.0f, 0.0f, 0.0f } },
			{ { d, d,-d }, colblue,{ 1.0, 0.0 },{ 1.0f, 0.0f, 0.0f } },
			{ { d,-d,-d }, colblue,{ 0.0, 0.0 },{ 1.0f, 0.0f, 0.0f } },
			{ { d, d, d }, colblue,{ 1.0, 1.0 },{ 1.0f, 0.0f, 0.0f } },
			{ { d,-d, d }, colblue,{ 0.0, 1.0 },{ 1.0f, 0.0f, 0.0f } },
			// -Z
			{ { d, d,-d }, colgreen,{ 1.0, 1.0 },{ 0.0f, 0.0f, -1.0f } },
			{ { -d,-d,-d }, colgreen,{ 0.0, 0.0 },{ 0.0f, 0.0f, -1.0f } },
			{ { -d, d,-d }, colgreen,{ 0.0, 1.0 },{ 0.0f, 0.0f, -1.0f } },
			{ { d, d,-d }, colgreen,{ 1.0, 1.0 },{ 0.0f, 0.0f, -1.0f } },
			{ { d,-d,-d }, colgreen,{ 1.0, 0.0 },{ 0.0f, 0.0f, -1.0f } },
			{ { -d,-d,-d }, colgreen,{ 0.0, 0.0 },{ 0.0f, 0.0f, -1.0f } },
			// +Z
			{ { -d, d, d }, colgreen,{ 0.0, 1.0 },{ 0.0f, 0.0f, 1.0f } },
			{ { -d,-d, d }, colgreen,{ 0.0, 0.0 },{ 0.0f, 0.0f, 1.0f } },
			{ { d,-d, d }, colgreen,{ 1.0, 0.0 },{ 0.0f, 0.0f, 1.0f } },
			{ { d, d, d }, colgreen,{ 1.0, 1.0 },{ 0.0f, 0.0f, 1.0f } },
			{ { -d, d, d }, colgreen,{ 0.0, 1.0 },{ 0.0f, 0.0f, 1.0f } },
			{ { d,-d, d }, colgreen,{ 1.0, 0.0 },{ 0.0f, 0.0f, 1.0f } }

		};
#undef d

		vertices.count = (int)vertexBuffer.size();

		m_pFramework->createBuffer(
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			vertexBuffer.size() * sizeof(Vertex),
			vertexBuffer.data(),
			&meshes.cube.vertices.buf,
			&meshes.cube.vertices.mem);		
	}

	void prepareVertices()
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
		vertices.attributeDescriptions.resize(4);
		// Location 0 : Position
		vertices.attributeDescriptions[0] =
			vkTools::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				0,
				VK_FORMAT_R32G32B32_SFLOAT,
				0);
		// Location 1 : Color
		vertices.attributeDescriptions[1] =
			vkTools::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				1,
				VK_FORMAT_R32G32B32_SFLOAT,
				sizeof(float) * 3);
		// Location 3 : Texture coordinates
		vertices.attributeDescriptions[2] =
			vkTools::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				2,
				VK_FORMAT_R32G32_SFLOAT,
				sizeof(float) * 6);
		// Location 2 : Normal
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
		// Example uses one ubo and one combined image sampler 
		std::vector<VkDescriptorPoolSize> poolSizes =
		{
			vkTools::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1),
			vkTools::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1),
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

		// Color map image descriptor
		VkDescriptorImageInfo texDescriptorColorMap =
			vkTools::initializers::descriptorImageInfo(
				textureColorMap.sampler,
				textureColorMap.view,
				VK_IMAGE_LAYOUT_GENERAL);

		std::vector<VkWriteDescriptorSet> writeDescriptorSets =
		{
			// Binding 0 : Vertex shader uniform buffer
			vkTools::initializers::writeDescriptorSet(
			descriptorSet,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				0,
				&uniformDataVS.descriptor),
			// Binding 1 : Fragment shader image sampler
			vkTools::initializers::writeDescriptorSet(
				descriptorSet,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				1,
				&texDescriptorColorMap)
		};

		vkUpdateDescriptorSets(m_pFramework->device, (uint32_t)writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);
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
			VK_DYNAMIC_STATE_SCISSOR,
			VK_DYNAMIC_STATE_LINE_WIDTH
		};
		VkPipelineDynamicStateCreateInfo dynamicState =
			vkTools::initializers::pipelineDynamicStateCreateInfo(
				dynamicStateEnables.data(),
				(uint32_t)dynamicStateEnables.size(),
				0);

		// Color pipeline
		// Load shaders
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

		shaderStages[0] = m_pFramework->loadShader(m_pFramework->getAssetPath() + "shaders/pipelines/base.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = m_pFramework->loadShader(m_pFramework->getAssetPath() + "shaders/pipelines/color.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	
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

		// Textured pipeline
		VkResult err = vkCreateGraphicsPipelines(m_pFramework->device, m_pFramework->pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.solidColor);
		assert(!err);

		// Reuse most of the initial pipeline for the next pipelines and only change affected parameters
		// Cull back faces
		rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;

		// Pipeline for textured rendering
		// Use different fragment shader
		shaderStages[1] = m_pFramework->loadShader(m_pFramework->getAssetPath() + "shaders/pipelines/texture.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		err = vkCreateGraphicsPipelines(m_pFramework->device, m_pFramework->pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.texture);
		assert(!err);

		// Pipeline for wire frame rendering
		// Solid polygon fill
		rasterizationState.polygonMode = VK_POLYGON_MODE_LINE;
		// Use different fragment shader
		shaderStages[1] = m_pFramework->loadShader(m_pFramework->getAssetPath() + "shaders/pipelines/wireframe.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		err = vkCreateGraphicsPipelines(m_pFramework->device, m_pFramework->pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.wireFrame);
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
			sizeof(uboVS));

		err = vkCreateBuffer(m_pFramework->device, &bufferInfo, nullptr, &uniformDataVS.buffer);
		assert(!err);
		vkGetBufferMemoryRequirements(m_pFramework->device, uniformDataVS.buffer, &memReqs);
		allocInfo.allocationSize = memReqs.size;
		m_pFramework->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &allocInfo.memoryTypeIndex);
		err = vkAllocateMemory(m_pFramework->device, &allocInfo, nullptr, &uniformDataVS.memory);
		assert(!err);
		err = vkBindBufferMemory(m_pFramework->device, uniformDataVS.buffer, uniformDataVS.memory, 0);
		assert(!err);

		uniformDataVS.descriptor.buffer = uniformDataVS.buffer;
		uniformDataVS.descriptor.offset = 0;
		uniformDataVS.descriptor.range = sizeof(uboVS);

		updateUniformBuffers();
	}

	void updateUniformBuffers()
	{
		uboVS.projectionMatrix = glm::perspective(deg_to_rad(60.0f), (float)(m_pFramework->ScreenRect.Width / 3.0f) / (float)m_pFramework->ScreenRect.Height, 0.1f, 256.0f);
		uboVS.viewMatrix = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, m_pFramework->zoom));

		uboVS.modelMatrix = glm::mat4();
		uboVS.modelMatrix = glm::rotate(uboVS.modelMatrix, glm::radians(m_pFramework->rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		uboVS.modelMatrix = glm::rotate(uboVS.modelMatrix, glm::radians(m_pFramework->rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		uboVS.modelMatrix = glm::rotate(uboVS.modelMatrix, glm::radians(m_pFramework->rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

		uint8_t *pData;
		VkResult err = vkMapMemory(m_pFramework->device, uniformDataVS.memory, 0, sizeof(uboVS), 0, (void **)&pData);
		assert(!err);
		memcpy(pData, &uboVS, sizeof(uboVS));
		vkUnmapMemory(m_pFramework->device, uniformDataVS.memory);
		assert(!err);
	}

	int32_t	prepare()
	{
		
		loadTextures();
		prepareVertices();
		prepareUniformBuffers();
		setupDescriptorSetLayout();
		generateCube();
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

	virtual void	keyPressed(uint32_t keyCode)
	{
	}

};

DEFINE_VULKAN_GAME_CREATE_AND_RELEASE_FUNCTIONS()
