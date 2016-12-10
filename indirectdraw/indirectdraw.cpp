/*
* Vulkan Example - Indirect drawing 
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*
* Summary:
* Use a device local buffer that stores draw commands for instanced rendering of different meshes stored
* in the same buffer.
*
* Indirect drawing offloads draw command generation and offers the ability to update them on the GPU 
* without the CPU having to touch the buffer again, also reducing the number of drawcalls.
*
* The example shows how to setup and fill such a buffer on the CPU side, stages it to the device and
* shows how to render it using only one draw command.
*
* See readme.md for details
*
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h> 
#include <vector>
#include <random>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vulkan/vulkan.h>
#include "vulkanexamplebase.h"
#include "vulkanbuffer.hpp"

#define VERTEX_BUFFER_BIND_ID 0
#define INSTANCE_BUFFER_BIND_ID 1
#define ENABLE_VALIDATION false

// Number of instances per object
#if defined(__ANDROID__)
#define OBJECT_INSTANCE_COUNT 1024
// Circular range of plant distribution
#define PLANT_RADIUS 20.0f
#else
#define OBJECT_INSTANCE_COUNT 2048
// Circular range of plant distribution
#define PLANT_RADIUS 25.0f
#endif

// Vertex layout for this example
std::vector<vkMeshLoader::VertexLayout> vertexLayout =
{
	vkMeshLoader::VERTEX_LAYOUT_POSITION,
	vkMeshLoader::VERTEX_LAYOUT_NORMAL,
	vkMeshLoader::VERTEX_LAYOUT_UV,
	vkMeshLoader::VERTEX_LAYOUT_COLOR
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
		vkMeshLoader::MeshBuffer plants;
		vkMeshLoader::MeshBuffer ground;
		vkMeshLoader::MeshBuffer skysphere;
	} meshes;

	struct {
		vkTools::VulkanTexture plants;
		vkTools::VulkanTexture ground;
	} textures;

	// Per-instance data block
	struct InstanceData {
		glm::vec3 pos;
		glm::vec3 rot;
		float scale;
		uint32_t texIndex;
	};

	// Contains the instanced data
	vk::Buffer instanceBuffer;
	// Contains the indirect drawing commands
	vk::Buffer indirectCommandsBuffer;
	uint32_t indirectDrawCount;

	struct {
		glm::mat4 projection;
		glm::mat4 view;
	} uboVS;

	struct {
		vk::Buffer scene;
	} uniformData;

	struct {
		VkPipeline plants;
		VkPipeline ground;
		VkPipeline skysphere;
	} pipelines;

	VkPipelineLayout pipelineLayout;
	VkDescriptorSet descriptorSet;
	VkDescriptorSetLayout descriptorSetLayout;

	VkSampler samplerRepeat;

	uint32_t objectCount = 0;

	// Store the indirect draw commands containing index offsets and instance count per object
	std::vector<VkDrawIndexedIndirectCommand> indirectCommands;

	VulkanExample() : VulkanExampleBase(ENABLE_VALIDATION)
	{
		enableTextOverlay = true;
		title = "Vulkan Example - Indirect rendering";
		camera.type = Camera::CameraType::firstperson;
		camera.setPerspective(60.0f, (float)width / (float)height, 0.1f, 512.0f);
		camera.setRotation(glm::vec3(-12.0f, 159.0f, 0.0f));
		camera.setTranslation(glm::vec3(0.4f, 1.25f, 0.0f));
		camera.movementSpeed = 5.0f;
	}

	~VulkanExample()
	{
		vkDestroyPipeline(device, pipelines.plants, nullptr);
		vkDestroyPipeline(device, pipelines.ground, nullptr);
		vkDestroyPipeline(device, pipelines.skysphere, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
		vkMeshLoader::freeMeshBufferResources(device, &meshes.plants);
		vkMeshLoader::freeMeshBufferResources(device, &meshes.ground);
		vkMeshLoader::freeMeshBufferResources(device, &meshes.skysphere);
		textureLoader->destroyTexture(textures.plants);
		textureLoader->destroyTexture(textures.ground);
		instanceBuffer.destroy();
		indirectCommandsBuffer.destroy();
		uniformData.scene.destroy();
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
		clearValues[0].color = { { 0.18f, 0.27f, 0.5f, 0.0f } };
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderPassBeginInfo = vkTools::initializers::renderPassBeginInfo();
		renderPassBeginInfo.renderPass = renderPass;
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

			VkRect2D scissor = vkTools::initializers::rect2D(width, height, 0, 0);
			vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

			VkDeviceSize offsets[1] = { 0 };
			vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);

			// Plants
			vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.plants);
			// Binding point 0 : Mesh vertex buffer
			vkCmdBindVertexBuffers(drawCmdBuffers[i], VERTEX_BUFFER_BIND_ID, 1, &meshes.plants.vertices.buf, offsets);
			// Binding point 1 : Instance data buffer
			vkCmdBindVertexBuffers(drawCmdBuffers[i], INSTANCE_BUFFER_BIND_ID, 1, &instanceBuffer.buffer, offsets);
			
			vkCmdBindIndexBuffer(drawCmdBuffers[i], meshes.plants.indices.buf, 0, VK_INDEX_TYPE_UINT32);

			// If the multi draw feature is supported:
			// One draw call for an arbitrary number of ojects
			// Index offsets and instance count are taken from the indirect buffer
			if (vulkanDevice->features.multiDrawIndirect)
			{
				vkCmdDrawIndexedIndirect(drawCmdBuffers[i], indirectCommandsBuffer.buffer, 0, indirectDrawCount, sizeof(VkDrawIndexedIndirectCommand));
			}
			else
			{
				// If multi draw is not available, we must issue separate draw commands
				for (auto j = 0; j < indirectCommands.size(); j++)
				{
					vkCmdDrawIndexedIndirect(drawCmdBuffers[i], indirectCommandsBuffer.buffer, j * sizeof(VkDrawIndexedIndirectCommand), 1, sizeof(VkDrawIndexedIndirectCommand));
				}
			}

			// Ground
			vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.ground);
			vkCmdBindVertexBuffers(drawCmdBuffers[i], VERTEX_BUFFER_BIND_ID, 1, &meshes.ground.vertices.buf, offsets);
			vkCmdBindIndexBuffer(drawCmdBuffers[i], meshes.ground.indices.buf, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(drawCmdBuffers[i], meshes.ground.indexCount, 1, 0, 0, 0);
			// Skysphere
			vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.skysphere);
			vkCmdBindVertexBuffers(drawCmdBuffers[i], VERTEX_BUFFER_BIND_ID, 1, &meshes.skysphere.vertices.buf, offsets);
			vkCmdBindIndexBuffer(drawCmdBuffers[i], meshes.skysphere.indices.buf, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(drawCmdBuffers[i], meshes.skysphere.indexCount, 1, 0, 0, 0);

			vkCmdEndRenderPass(drawCmdBuffers[i]);

			VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
		}
	}

	void loadAssets()
	{
		loadMesh(getAssetPath() + "models/plants.dae", &meshes.plants, vertexLayout, 0.0025f);
		loadMesh(getAssetPath() + "models/plane_circle.dae", &meshes.ground, vertexLayout, PLANT_RADIUS + 1.0f);
		loadMesh(getAssetPath() + "models/skysphere.dae", &meshes.skysphere, vertexLayout, 512.0f / 10.0f);

		textureLoader->loadTextureArray(getAssetPath() + "textures/texturearray_plants_bc3.ktx", VK_FORMAT_BC3_UNORM_BLOCK, &textures.plants);
		textureLoader->loadTexture(getAssetPath() + "textures/ground_dry_bc3.ktx", VK_FORMAT_BC3_UNORM_BLOCK, &textures.ground);
	}

	void setupVertexDescriptions()
	{
		// Binding description
		vertices.bindingDescriptions.resize(2);

		// Mesh vertex buffer (description) at binding point 0
		vertices.bindingDescriptions[0] =
			vkTools::initializers::vertexInputBindingDescription(
				VERTEX_BUFFER_BIND_ID,
				vkMeshLoader::vertexSize(vertexLayout),
				// Input rate for the data passed to shader
				// Step for each vertex rendered
				VK_VERTEX_INPUT_RATE_VERTEX);

		vertices.bindingDescriptions[1] =
			vkTools::initializers::vertexInputBindingDescription(
				INSTANCE_BUFFER_BIND_ID,
				sizeof(InstanceData), 
				// Input rate for the data passed to shader
				// Step for each instance rendered
				VK_VERTEX_INPUT_RATE_INSTANCE);

		// Attribute descriptions
		// Describes memory layout and shader positions
		vertices.attributeDescriptions.clear();

		// Per-Vertex attributes
		// Location 0 : Position
		vertices.attributeDescriptions.push_back(
			vkTools::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				0,
				VK_FORMAT_R32G32B32_SFLOAT,
				0)
			);
		// Location 1 : Normal
		vertices.attributeDescriptions.push_back(
			vkTools::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				1,
				VK_FORMAT_R32G32B32_SFLOAT,
				sizeof(float) * 3)
			);
		// Location 2 : Texture coordinates
		vertices.attributeDescriptions.push_back(
			vkTools::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				2,
				VK_FORMAT_R32G32_SFLOAT,
				sizeof(float) * 6)
			);
		// Location 3 : Color
		vertices.attributeDescriptions.push_back(
			vkTools::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				3,
				VK_FORMAT_R32G32B32_SFLOAT,
				sizeof(float) * 8)
			);

		// Instanced attributes
		// Location 4: Position
		vertices.attributeDescriptions.push_back(
			vkTools::initializers::vertexInputAttributeDescription(
				INSTANCE_BUFFER_BIND_ID, 4, VK_FORMAT_R32G32B32_SFLOAT, offsetof(InstanceData, pos))
			);
		// Location 5: Rotation
		vertices.attributeDescriptions.push_back(
			vkTools::initializers::vertexInputAttributeDescription(
				INSTANCE_BUFFER_BIND_ID, 5, VK_FORMAT_R32G32B32_SFLOAT, offsetof(InstanceData, rot))
			);
		// Location 6: Scale
		vertices.attributeDescriptions.push_back(
			vkTools::initializers::vertexInputAttributeDescription(
				INSTANCE_BUFFER_BIND_ID, 6, VK_FORMAT_R32_SFLOAT, offsetof(InstanceData, scale))
			);
		// Location 7: Texture array layer index
		vertices.attributeDescriptions.push_back(
			vkTools::initializers::vertexInputAttributeDescription(
				INSTANCE_BUFFER_BIND_ID, 7, VK_FORMAT_R32_SINT, offsetof(InstanceData, texIndex))
			);

		vertices.inputState = vkTools::initializers::pipelineVertexInputStateCreateInfo();
		vertices.inputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertices.bindingDescriptions.size());
		vertices.inputState.pVertexBindingDescriptions = vertices.bindingDescriptions.data();
		vertices.inputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertices.attributeDescriptions.size());
		vertices.inputState.pVertexAttributeDescriptions = vertices.attributeDescriptions.data();
	}

	void setupDescriptorPool()
	{
		// Example uses one ubo 
		std::vector<VkDescriptorPoolSize> poolSizes =
		{
			vkTools::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1),
			vkTools::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2),
		};

		VkDescriptorPoolCreateInfo descriptorPoolInfo =
			vkTools::initializers::descriptorPoolCreateInfo(
				static_cast<uint32_t>(poolSizes.size()),
				poolSizes.data(),
				2);

		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));
	}

	void setupDescriptorSetLayout()
	{
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings =
		{
			// Binding 0: Vertex shader uniform buffer
			vkTools::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				VK_SHADER_STAGE_VERTEX_BIT,
				0),
			// Binding 1: Fragment shader combined sampler (plants texture array)
			vkTools::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_SHADER_STAGE_FRAGMENT_BIT,
				1),
			// Binding 1: Fragment shader combined sampler (ground texture)
			vkTools::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_SHADER_STAGE_FRAGMENT_BIT,
				2),
		};

		VkDescriptorSetLayoutCreateInfo descriptorLayout =
			vkTools::initializers::descriptorSetLayoutCreateInfo(
				setLayoutBindings.data(),
				static_cast<uint32_t>(setLayoutBindings.size()));

		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayout));

		VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo =
			vkTools::initializers::pipelineLayoutCreateInfo(
				&descriptorSetLayout,
				1);

		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &pipelineLayout));
	}

	void setupDescriptorSet()
	{
		VkDescriptorSetAllocateInfo allocInfo =
			vkTools::initializers::descriptorSetAllocateInfo(
				descriptorPool,
				&descriptorSetLayout,
				1);

		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));

		std::vector<VkWriteDescriptorSet> writeDescriptorSets =
		{
			// Binding 0: Vertex shader uniform buffer
			vkTools::initializers::writeDescriptorSet(
			descriptorSet,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				0,
				&uniformData.scene.descriptor),
			// Binding 1: Plants texture array combined 
			vkTools::initializers::writeDescriptorSet(
				descriptorSet,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				1,
				&textures.plants.descriptor),
			// Binding 2: Ground texture combined 
			vkTools::initializers::writeDescriptorSet(
				descriptorSet,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				2,
				&textures.ground.descriptor)
		};

		vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, NULL);
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
				static_cast<uint32_t>(dynamicStateEnables.size()),
				0);

		VkGraphicsPipelineCreateInfo pipelineCreateInfo =
			vkTools::initializers::pipelineCreateInfo(
				pipelineLayout,
				renderPass,
				0);

		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

		pipelineCreateInfo.pVertexInputState = &vertices.inputState;
		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
		pipelineCreateInfo.pRasterizationState = &rasterizationState;
		pipelineCreateInfo.pColorBlendState = &colorBlendState;
		pipelineCreateInfo.pMultisampleState = &multisampleState;
		pipelineCreateInfo.pViewportState = &viewportState;
		pipelineCreateInfo.pDepthStencilState = &depthStencilState;
		pipelineCreateInfo.pDynamicState = &dynamicState;
		pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineCreateInfo.pStages = shaderStages.data();

		// Indirect (and instanced) pipeline for the plants
		shaderStages[0] = loadShader(getAssetPath() + "shaders/indirectdraw/indirectdraw.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getAssetPath() + "shaders/indirectdraw/indirectdraw.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.plants));

		// Ground
		shaderStages[0] = loadShader(getAssetPath() + "shaders/indirectdraw/ground.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getAssetPath() + "shaders/indirectdraw/ground.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		//rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.ground));

		// Skysphere
		shaderStages[0] = loadShader(getAssetPath() + "shaders/indirectdraw/skysphere.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getAssetPath() + "shaders/indirectdraw/skysphere.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		//rasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.skysphere));
	}

	// Prepare (and stage) a buffer containing the indirect draw commands
	void prepareIndirectData()
	{
		indirectCommands.clear();

		// Create on indirect command for each mesh in the scene
		uint32_t m = 0;
		for (auto& meshDescriptor : meshes.plants.meshDescriptors)
		{
			VkDrawIndexedIndirectCommand indirectCmd{};
			indirectCmd.instanceCount = OBJECT_INSTANCE_COUNT;
			indirectCmd.firstInstance = m * OBJECT_INSTANCE_COUNT;
			indirectCmd.firstIndex = meshDescriptor.indexBase;
			indirectCmd.indexCount = meshDescriptor.indexCount;
			
			indirectCommands.push_back(indirectCmd);

			m++;
		}

		indirectDrawCount = static_cast<uint32_t>(indirectCommands.size());

		objectCount = 0;
		for (auto indirectCmd : indirectCommands)
		{
			objectCount += indirectCmd.instanceCount;
		}

		vk::Buffer stagingBuffer;
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&stagingBuffer,
			indirectCommands.size() * sizeof(VkDrawIndexedIndirectCommand),
			indirectCommands.data()));

		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			&indirectCommandsBuffer,
			stagingBuffer.size));

		vulkanDevice->copyBuffer(&stagingBuffer, &indirectCommandsBuffer, queue);

		stagingBuffer.destroy();
	}

	// Prepare (and stage) a buffer containing instanced data for the mesh draws
	void prepareInstanceData()
	{
		std::vector<InstanceData> instanceData;
		instanceData.resize(objectCount);

		std::mt19937 rndGenerator((unsigned)time(NULL));
		std::uniform_real_distribution<float> uniformDist(0.0f, 1.0f);

		for (uint32_t i = 0; i < objectCount; i++)
		{
			instanceData[i].rot = glm::vec3(0.0f, float(M_PI) * uniformDist(rndGenerator), 0.0f);
			float theta = 2 * float(M_PI) * uniformDist(rndGenerator);
			float phi = acos(1 - 2 * uniformDist(rndGenerator));
			instanceData[i].pos = glm::vec3(sin(phi) * cos(theta), 0.0f, cos(phi)) * PLANT_RADIUS;
			instanceData[i].scale = 1.0f + uniformDist(rndGenerator) * 2.0f;
			instanceData[i].texIndex = i / OBJECT_INSTANCE_COUNT;
		}

		vk::Buffer stagingBuffer;
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&stagingBuffer,
			instanceData.size() * sizeof(InstanceData),
			instanceData.data()));

		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			&instanceBuffer,
			stagingBuffer.size));

		vulkanDevice->copyBuffer(&stagingBuffer, &instanceBuffer, queue);

		stagingBuffer.destroy();
	}

	void prepareUniformBuffers()
	{
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&uniformData.scene,
			sizeof(uboVS)));

		VK_CHECK_RESULT(uniformData.scene.map());

		updateUniformBuffer(true);
	}

	void updateUniformBuffer(bool viewChanged)
	{
		if (viewChanged)
		{
			uboVS.projection = camera.matrices.perspective;
			uboVS.view = camera.matrices.view;
		}

		memcpy(uniformData.scene.mapped, &uboVS, sizeof(uboVS));
	}

	void draw()
	{
		VulkanExampleBase::prepareFrame();

		// Command buffer to be sumitted to the queue
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];

		// Submit to queue
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

		VulkanExampleBase::submitFrame();
	}

	void prepare()
	{
		VulkanExampleBase::prepare();
		loadAssets();
		prepareIndirectData();
		prepareInstanceData();
		setupVertexDescriptions();
		prepareUniformBuffers();
		setupDescriptorSetLayout();
		preparePipelines();
		setupDescriptorPool();
		setupDescriptorSet();
		buildCommandBuffers();
		prepared = true;
	}

	virtual void render()
	{
		if (!prepared)
		{
			return;
		}
		draw();
	}

	virtual void viewChanged()
	{
		updateUniformBuffer(true);
	}

	virtual void getOverlayText(VulkanTextOverlay *textOverlay)
	{
		textOverlay->addText(std::to_string(objectCount) + " objects", 5.0f, 85.0f, VulkanTextOverlay::alignLeft);
		if (!vulkanDevice->features.multiDrawIndirect)
		{
			textOverlay->addText("multiDrawIndirect not supported", 5.0f, 105.0f, VulkanTextOverlay::alignLeft);
		}
	}
};

VULKAN_EXAMPLE_MAIN()