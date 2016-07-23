/*
* Vulkan Example - Using occlusion query for visbility testing
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

// Vertex layout used in this example
// Vertex layout for this example
std::vector<vkMeshLoader::VertexLayout> vertexLayout =
{
	vkMeshLoader::VERTEX_LAYOUT_POSITION,
	vkMeshLoader::VERTEX_LAYOUT_NORMAL,
	vkMeshLoader::VERTEX_LAYOUT_COLOR,
};

class VulkanExample : public VulkanExampleBase
{
public:
	struct {
		VkPipelineVertexInputStateCreateInfo inputState;
		std::vector<VkVertexInputBindingDescription> bindingDescriptions;
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
	} vertices;

	struct {
		vkMeshLoader::MeshBuffer teapot;
		vkMeshLoader::MeshBuffer plane;
		vkMeshLoader::MeshBuffer sphere;
	} meshes;

	struct {
		vkTools::UniformData vsScene;
		vkTools::UniformData teapot;
		vkTools::UniformData sphere;
	} uniformData;

	struct {
		glm::mat4 projection;
		glm::mat4 model;
		glm::vec4 lightPos = glm::vec4(10.0f, 10.0f, 10.0f, 1.0f);
		float visible;
	} uboVS;

	struct {
		VkPipeline solid;
		VkPipeline occluder;
		// Pipeline with basic shaders used for occlusion pass
		VkPipeline simple;
	} pipelines;

	struct {
		VkDescriptorSet teapot;
		VkDescriptorSet sphere;
	} descriptorSets;

	VkPipelineLayout pipelineLayout;
	VkDescriptorSet descriptorSet;
	VkDescriptorSetLayout descriptorSetLayout;

	// Stores occlusion query results
	struct {
		VkBuffer buffer;
		VkDeviceMemory memory;
	} queryResult;

	// Pool that stores all occlusion queries
	VkQueryPool queryPool;

	// Passed query samples
	uint64_t passedSamples[2] = { 1,1 };

	VulkanExample() : VulkanExampleBase(ENABLE_VALIDATION)
	{
		width = 1280;
		height = 720;
		zoom = -35.0f;
		zoomSpeed = 2.5f;
		rotationSpeed = 0.5f;
		rotation = { 0.0, -123.75, 0.0 };
		enableTextOverlay = true;
		title = "Vulkan Example - Occlusion queries";
	}

	~VulkanExample()
	{
		// Clean up used Vulkan resources 
		// Note : Inherited destructor cleans up resources stored in base class
		vkDestroyPipeline(device, pipelines.solid, nullptr);
		vkDestroyPipeline(device, pipelines.occluder, nullptr);
		vkDestroyPipeline(device, pipelines.simple, nullptr);

		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

		vkDestroyQueryPool(device, queryPool, nullptr);

		vkDestroyBuffer(device, queryResult.buffer, nullptr);
		vkFreeMemory(device, queryResult.memory, nullptr);

		vkTools::destroyUniformData(device, &uniformData.vsScene);
		vkTools::destroyUniformData(device, &uniformData.sphere);
		vkTools::destroyUniformData(device, &uniformData.teapot);

		vkMeshLoader::freeMeshBufferResources(device, &meshes.sphere);
		vkMeshLoader::freeMeshBufferResources(device, &meshes.plane);
		vkMeshLoader::freeMeshBufferResources(device, &meshes.teapot);
	}

	// Create a buffer for storing the query result
	// Setup a query pool
	void setupQueryResultBuffer()
	{
		uint32_t bufSize = 2 * sizeof(uint64_t);

		VkMemoryRequirements memReqs;
		VkMemoryAllocateInfo memAlloc = vkTools::initializers::memoryAllocateInfo();
		VkBufferCreateInfo bufferCreateInfo = 
			vkTools::initializers::bufferCreateInfo(
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
				bufSize);

		// Results are saved in a host visible buffer for easy access by the application
		VK_CHECK_RESULT(vkCreateBuffer(device, &bufferCreateInfo, nullptr, &queryResult.buffer));
		vkGetBufferMemoryRequirements(device, queryResult.buffer, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		memAlloc.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &queryResult.memory));
		VK_CHECK_RESULT(vkBindBufferMemory(device, queryResult.buffer, queryResult.memory, 0));

		// Create query pool
		VkQueryPoolCreateInfo queryPoolInfo = {};
		queryPoolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
		// Query pool will be created for occlusion queries
		queryPoolInfo.queryType = VK_QUERY_TYPE_OCCLUSION;
		queryPoolInfo.queryCount = 2;

		VK_CHECK_RESULT(vkCreateQueryPool(device, &queryPoolInfo, NULL, &queryPool));
	}

	// Retrieves the results of the occlusion queries submitted to the command buffer
	void getQueryResults()
	{
		// We use vkGetQueryResults to copy the results into a host visible buffer
		vkGetQueryPoolResults(
			device, 
			queryPool,
			0,
			2,
			sizeof(passedSamples),
			passedSamples,
			sizeof(uint64_t),
			// Store results a 64 bit values and wait until the results have been finished
			// If you don't want to wait, you can use VK_QUERY_RESULT_WITH_AVAILABILITY_BIT
			// which also returns the state of the result (ready) in the result
			VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
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

			// Reset query pool
			// Must be done outside of render pass
			vkCmdResetQueryPool(drawCmdBuffers[i], queryPool, 0, 2);

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

			glm::mat4 modelMatrix = glm::mat4();

			// Occlusion pass
			vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.simple);

			// Occluder first
			vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);
			vkCmdBindVertexBuffers(drawCmdBuffers[i], VERTEX_BUFFER_BIND_ID, 1, &meshes.plane.vertices.buf, offsets);
			vkCmdBindIndexBuffer(drawCmdBuffers[i], meshes.plane.indices.buf, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(drawCmdBuffers[i], meshes.plane.indexCount, 1, 0, 0, 0);

			// Teapot
			vkCmdBeginQuery(drawCmdBuffers[i], queryPool, 0, VK_FLAGS_NONE);

			vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets.teapot, 0, NULL);
			vkCmdBindVertexBuffers(drawCmdBuffers[i], VERTEX_BUFFER_BIND_ID, 1, &meshes.teapot.vertices.buf, offsets);
			vkCmdBindIndexBuffer(drawCmdBuffers[i], meshes.teapot.indices.buf, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(drawCmdBuffers[i], meshes.teapot.indexCount, 1, 0, 0, 0);

			vkCmdEndQuery(drawCmdBuffers[i], queryPool, 0);

			// Sphere
			vkCmdBeginQuery(drawCmdBuffers[i], queryPool, 1, VK_FLAGS_NONE);

			vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets.sphere, 0, NULL);
			vkCmdBindVertexBuffers(drawCmdBuffers[i], VERTEX_BUFFER_BIND_ID, 1, &meshes.sphere.vertices.buf, offsets);
			vkCmdBindIndexBuffer(drawCmdBuffers[i], meshes.sphere.indices.buf, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(drawCmdBuffers[i], meshes.sphere.indexCount, 1, 0, 0, 0);

			vkCmdEndQuery(drawCmdBuffers[i], queryPool, 1);

			// Visible pass
			// Clear color and depth attachments
			VkClearAttachment clearAttachments[2] = {};

			clearAttachments[0].aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			clearAttachments[0].clearValue.color = defaultClearColor;
			clearAttachments[0].colorAttachment = 0;

			clearAttachments[1].aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			clearAttachments[1].clearValue.depthStencil = { 1.0f, 0 };

			VkClearRect clearRect = {};
			clearRect.layerCount = 1;
			clearRect.rect.offset = { 0, 0 };
			clearRect.rect.extent = { width, height };

			vkCmdClearAttachments(
				drawCmdBuffers[i],
				2,
				clearAttachments,
				1,
				&clearRect);

			vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.solid);

			// Teapot
			vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets.teapot, 0, NULL);
			vkCmdBindVertexBuffers(drawCmdBuffers[i], VERTEX_BUFFER_BIND_ID, 1, &meshes.teapot.vertices.buf, offsets);
			vkCmdBindIndexBuffer(drawCmdBuffers[i], meshes.teapot.indices.buf, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(drawCmdBuffers[i], meshes.teapot.indexCount, 1, 0, 0, 0);

			// Sphere
			vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets.sphere, 0, NULL);
			vkCmdBindVertexBuffers(drawCmdBuffers[i], VERTEX_BUFFER_BIND_ID, 1, &meshes.sphere.vertices.buf, offsets);
			vkCmdBindIndexBuffer(drawCmdBuffers[i], meshes.sphere.indices.buf, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(drawCmdBuffers[i], meshes.sphere.indexCount, 1, 0, 0, 0);

			// Occluder
			vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.occluder);
			vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);
			vkCmdBindVertexBuffers(drawCmdBuffers[i], VERTEX_BUFFER_BIND_ID, 1, &meshes.plane.vertices.buf, offsets);
			vkCmdBindIndexBuffer(drawCmdBuffers[i], meshes.plane.indices.buf, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(drawCmdBuffers[i], meshes.plane.indexCount, 1, 0, 0, 0);

			vkCmdEndRenderPass(drawCmdBuffers[i]);

			VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
		}
	}

	void draw()
	{
		VulkanExampleBase::prepareFrame();

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

		// Read query results for displaying in next frame
		getQueryResults();

		VulkanExampleBase::submitFrame();
	}

	void loadMeshes()
	{
		loadMesh(getAssetPath() + "models/plane_z.3ds", &meshes.plane, vertexLayout, 0.4f);
		loadMesh(getAssetPath() + "models/teapot.3ds", &meshes.teapot, vertexLayout, 0.3f);
		loadMesh(getAssetPath() + "models/sphere.3ds", &meshes.sphere, vertexLayout, 0.3f);
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
		// Location 1 : Normal
		vertices.attributeDescriptions[1] =
			vkTools::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				1,
				VK_FORMAT_R32G32B32_SFLOAT,
				sizeof(float) * 3);
		// Location 3 : Color
		vertices.attributeDescriptions[2] =
			vkTools::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				2,
				VK_FORMAT_R32G32B32_SFLOAT,
				sizeof(float) * 6);

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
			// One uniform buffer block for each mesh
			vkTools::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3)
		};

		VkDescriptorPoolCreateInfo descriptorPoolInfo =
			vkTools::initializers::descriptorPoolCreateInfo(
				poolSizes.size(),
				poolSizes.data(),
				3);

		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));
	}

	void setupDescriptorSetLayout()
	{
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings =
		{
			// Binding 0 : Vertex shader uniform buffer
			vkTools::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				VK_SHADER_STAGE_VERTEX_BIT,
				0)
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

		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &pipelineLayout));
	}

	void setupDescriptorSets()
	{
		VkDescriptorSetAllocateInfo allocInfo =
			vkTools::initializers::descriptorSetAllocateInfo(
				descriptorPool,
				&descriptorSetLayout,
				1);

		// Occluder (plane)
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));

		std::vector<VkWriteDescriptorSet> writeDescriptorSets =
		{
			// Binding 0 : Vertex shader uniform buffer
			vkTools::initializers::writeDescriptorSet(
				descriptorSet,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				0,
				&uniformData.vsScene.descriptor)
		};

		vkUpdateDescriptorSets(device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);

		// Teapot
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.teapot));
		writeDescriptorSets[0].dstSet = descriptorSets.teapot;
		writeDescriptorSets[0].pBufferInfo = &uniformData.teapot.descriptor;
		vkUpdateDescriptorSets(device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);

		// Sphere
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.sphere));
		writeDescriptorSets[0].dstSet = descriptorSets.sphere;
		writeDescriptorSets[0].pBufferInfo = &uniformData.sphere.descriptor;
		vkUpdateDescriptorSets(device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);
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

		shaderStages[0] = loadShader(getAssetPath() + "shaders/occlusionquery/mesh.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getAssetPath() + "shaders/occlusionquery/mesh.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

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

		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.solid));

		// Basic pipeline for coloring occluded objects
		shaderStages[0] = loadShader(getAssetPath() + "shaders/occlusionquery/simple.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getAssetPath() + "shaders/occlusionquery/simple.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		rasterizationState.cullMode = VK_CULL_MODE_NONE;

		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.simple));

		// Visual pipeline for the occluder
		shaderStages[0] = loadShader(getAssetPath() + "shaders/occlusionquery/occluder.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getAssetPath() + "shaders/occlusionquery/occluder.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

		// Enable blending
		blendAttachmentState.blendEnable = VK_TRUE;
		blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
		blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
		blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;

		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.occluder));
	}

	// Prepare and initialize uniform buffer containing shader uniforms
	void prepareUniformBuffers()
	{
		// Vertex shader uniform buffer block
		createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			sizeof(uboVS),
			&uboVS,
			&uniformData.vsScene.buffer,
			&uniformData.vsScene.memory,
			&uniformData.vsScene.descriptor);

		// Teapot
		createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			sizeof(uboVS),
			&uboVS,
			&uniformData.teapot.buffer,
			&uniformData.teapot.memory,
			&uniformData.teapot.descriptor);

		// Sphere
		createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			sizeof(uboVS),
			&uboVS,
			&uniformData.sphere.buffer,
			&uniformData.sphere.memory,
			&uniformData.sphere.descriptor);

		updateUniformBuffers();
	}

	void updateUniformBuffers()
	{
		// Vertex shader
		uboVS.projection = glm::perspective(glm::radians(60.0f), (float)width / (float)height, 0.1f, 256.0f);
		glm::mat4 viewMatrix = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, zoom));

		glm::mat4 rotMatrix = glm::mat4();
		rotMatrix = glm::rotate(rotMatrix, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		rotMatrix = glm::rotate(rotMatrix, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		rotMatrix = glm::rotate(rotMatrix, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

		uboVS.model = viewMatrix * rotMatrix;

		uint8_t *pData;

		// Occluder
		uboVS.visible = 1.0f;
		VK_CHECK_RESULT(vkMapMemory(device, uniformData.vsScene.memory, 0, sizeof(uboVS), 0, (void **)&pData));
		memcpy(pData, &uboVS, sizeof(uboVS));
		vkUnmapMemory(device, uniformData.vsScene.memory);

		// Teapot
		// Toggle color depending on visibility
		uboVS.visible = (passedSamples[0] > 0) ? 1.0f : 0.0f;
		uboVS.model = viewMatrix * rotMatrix * glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, -10.0f));
		VK_CHECK_RESULT(vkMapMemory(device, uniformData.teapot.memory, 0, sizeof(uboVS), 0, (void **)&pData));
		memcpy(pData, &uboVS, sizeof(uboVS));
		vkUnmapMemory(device, uniformData.teapot.memory);

		// Sphere
		// Toggle color depending on visibility
		uboVS.visible = (passedSamples[1] > 0) ? 1.0f : 0.0f;
		uboVS.model = viewMatrix * rotMatrix * glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, 10.0f));
		VK_CHECK_RESULT(vkMapMemory(device, uniformData.sphere.memory, 0, sizeof(uboVS), 0, (void **)&pData));
		memcpy(pData, &uboVS, sizeof(uboVS));
		vkUnmapMemory(device, uniformData.sphere.memory);
	}

	void prepare()
	{
		VulkanExampleBase::prepare();
		loadMeshes();
		setupQueryResultBuffer();
		setupVertexDescriptions();
		prepareUniformBuffers();
		setupDescriptorSetLayout();
		preparePipelines();
		setupDescriptorPool();
		setupDescriptorSets();
		buildCommandBuffers();
		prepared = true;
	}

	virtual void render()
	{
		if (!prepared)
			return;
		draw();
	}

	virtual void viewChanged()
	{
		vkDeviceWaitIdle(device);
		updateUniformBuffers();
		VulkanExampleBase::updateTextOverlay();
	}

	virtual void getOverlayText(VulkanTextOverlay *textOverlay)
	{
		textOverlay->addText("Occlusion queries:", 5.0f, 85.0f, VulkanTextOverlay::alignLeft);
		textOverlay->addText("Teapot: " + std::to_string(passedSamples[0]) + " samples passed" , 5.0f, 105.0f, VulkanTextOverlay::alignLeft);
		textOverlay->addText("Sphere: " + std::to_string(passedSamples[1]) + " samples passed", 5.0f, 125.0f, VulkanTextOverlay::alignLeft);
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
