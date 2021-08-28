/*
* Vulkan Example - Compute shader culling and LOD using indirect rendering
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*
*/

#include "vulkanexamplebase.h"
#include "VulkanglTFModel.h"
#include "frustum.hpp"

#define VERTEX_BUFFER_BIND_ID 0
#define INSTANCE_BUFFER_BIND_ID 1
#define ENABLE_VALIDATION false

// Total number of objects (^3) in the scene
#if defined(__ANDROID__)
#define OBJECT_COUNT 32
#else
#define OBJECT_COUNT 64
#endif

#define MAX_LOD_LEVEL 5

class VulkanExample : public VulkanExampleBase
{
public:
	bool fixedFrustum = false;

	// The model contains multiple versions of a single object with different levels of detail
	vkglTF::Model lodModel;

	// Per-instance data block
	struct InstanceData {
		glm::vec3 pos;
		float scale;
	};

	// Contains the instanced data
	vks::Buffer instanceBuffer;
	// Contains the indirect drawing commands
	vks::Buffer indirectCommandsBuffer;
	vks::Buffer indirectDrawCountBuffer;

	// Indirect draw statistics (updated via compute)
	struct {
		uint32_t drawCount;						// Total number of indirect draw counts to be issued
		uint32_t lodCount[MAX_LOD_LEVEL + 1];	// Statistics for number of draws per LOD level (written by compute shader)
	} indirectStats;

	// Store the indirect draw commands containing index offsets and instance count per object
	std::vector<VkDrawIndexedIndirectCommand> indirectCommands;

	struct {
		glm::mat4 projection;
		glm::mat4 modelview;
		glm::vec4 cameraPos;
		glm::vec4 frustumPlanes[6];
	} uboScene;

	struct {
		vks::Buffer scene;
	} uniformData;

	struct {
		VkPipeline plants;
	} pipelines;

	VkPipelineLayout pipelineLayout;
	VkDescriptorSet descriptorSet;
	VkDescriptorSetLayout descriptorSetLayout;

	// Resources for the compute part of the example
	struct {
		vks::Buffer lodLevelsBuffers;				// Contains index start and counts for the different lod levels
		VkQueue queue;								// Separate queue for compute commands (queue family may differ from the one used for graphics)
		VkCommandPool commandPool;					// Use a separate command pool (queue family may differ from the one used for graphics)
		VkCommandBuffer commandBuffer;				// Command buffer storing the dispatch commands and barriers
		VkFence fence;								// Synchronization fence to avoid rewriting compute CB if still in use
		VkSemaphore semaphore;						// Used as a wait semaphore for graphics submission
		VkDescriptorSetLayout descriptorSetLayout;	// Compute shader binding layout
		VkDescriptorSet descriptorSet;				// Compute shader bindings
		VkPipelineLayout pipelineLayout;			// Layout of the compute pipeline
		VkPipeline pipeline;						// Compute pipeline for updating particle positions
	} compute;

	// View frustum for culling invisible objects
	vks::Frustum frustum;

	uint32_t objectCount = 0;

	VulkanExample() : VulkanExampleBase(ENABLE_VALIDATION)
	{
		title = "Vulkan Example - Compute cull and lod";
		camera.type = Camera::CameraType::firstperson;
		camera.setPerspective(60.0f, (float)width / (float)height, 0.1f, 512.0f);
		camera.setTranslation(glm::vec3(0.5f, 0.0f, 0.0f));
		camera.movementSpeed = 5.0f;
		memset(&indirectStats, 0, sizeof(indirectStats));
	}

	~VulkanExample()
	{
		vkDestroyPipeline(device, pipelines.plants, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
		instanceBuffer.destroy();
		indirectCommandsBuffer.destroy();
		uniformData.scene.destroy();
		indirectDrawCountBuffer.destroy();
		compute.lodLevelsBuffers.destroy();
		vkDestroyPipelineLayout(device, compute.pipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, compute.descriptorSetLayout, nullptr);
		vkDestroyPipeline(device, compute.pipeline, nullptr);
		vkDestroyFence(device, compute.fence, nullptr);
		vkDestroyCommandPool(device, compute.commandPool, nullptr);
		vkDestroySemaphore(device, compute.semaphore, nullptr);
	}

	virtual void getEnabledFeatures()
	{
		// Enable multi draw indirect if supported
		if (deviceFeatures.multiDrawIndirect) {
			enabledFeatures.multiDrawIndirect = VK_TRUE;
		}
	}

	void buildCommandBuffers()
	{
		VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

		VkClearValue clearValues[2];
		clearValues[0].color = { { 0.18f, 0.27f, 0.5f, 0.0f } };
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
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

			VkViewport viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
			vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

			VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);
			vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

			VkDeviceSize offsets[1] = { 0 };
			vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);

			// Mesh containing the LODs
			vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.plants);
			vkCmdBindVertexBuffers(drawCmdBuffers[i], VERTEX_BUFFER_BIND_ID, 1, &lodModel.vertices.buffer, offsets);
			vkCmdBindVertexBuffers(drawCmdBuffers[i], INSTANCE_BUFFER_BIND_ID, 1, &instanceBuffer.buffer, offsets);

			vkCmdBindIndexBuffer(drawCmdBuffers[i], lodModel.indices.buffer, 0, VK_INDEX_TYPE_UINT32);

			if (vulkanDevice->features.multiDrawIndirect)
			{
				vkCmdDrawIndexedIndirect(drawCmdBuffers[i], indirectCommandsBuffer.buffer, 0, indirectCommands.size(), sizeof(VkDrawIndexedIndirectCommand));
			}
			else
			{
				// If multi draw is not available, we must issue separate draw commands
				for (auto j = 0; j < indirectCommands.size(); j++)
				{
					vkCmdDrawIndexedIndirect(drawCmdBuffers[i], indirectCommandsBuffer.buffer, j * sizeof(VkDrawIndexedIndirectCommand), 1, sizeof(VkDrawIndexedIndirectCommand));
				}
			}

			drawUI(drawCmdBuffers[i]);

			vkCmdEndRenderPass(drawCmdBuffers[i]);

			VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
		}
	}

	void loadAssets()
	{
		const uint32_t glTFLoadingFlags = vkglTF::FileLoadingFlags::PreTransformVertices | vkglTF::FileLoadingFlags::PreMultiplyVertexColors | vkglTF::FileLoadingFlags::FlipY;
		lodModel.loadFromFile(getAssetPath() + "models/suzanne_lods.gltf", vulkanDevice, queue, glTFLoadingFlags);
	}

	void buildComputeCommandBuffer()
	{
		VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

		VK_CHECK_RESULT(vkBeginCommandBuffer(compute.commandBuffer, &cmdBufInfo));

		// Add memory barrier to ensure that the indirect commands have been consumed before the compute shader updates them
		VkBufferMemoryBarrier bufferBarrier = vks::initializers::bufferMemoryBarrier();
		bufferBarrier.buffer = indirectCommandsBuffer.buffer;
		bufferBarrier.size = indirectCommandsBuffer.descriptor.range;
		bufferBarrier.srcAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
		bufferBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		bufferBarrier.srcQueueFamilyIndex = vulkanDevice->queueFamilyIndices.graphics;
		bufferBarrier.dstQueueFamilyIndex = vulkanDevice->queueFamilyIndices.compute;

		vkCmdPipelineBarrier(
			compute.commandBuffer,
			VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_FLAGS_NONE,
			0, nullptr,
			1, &bufferBarrier,
			0, nullptr);

		vkCmdBindPipeline(compute.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute.pipeline);
		vkCmdBindDescriptorSets(compute.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute.pipelineLayout, 0, 1, &compute.descriptorSet, 0, 0);

		// Dispatch the compute job
		// The compute shader will do the frustum culling and adjust the indirect draw calls depending on object visibility.
		// It also determines the lod to use depending on distance to the viewer.
		vkCmdDispatch(compute.commandBuffer, objectCount / 16, 1, 1);

		// Add memory barrier to ensure that the compute shader has finished writing the indirect command buffer before it's consumed
		bufferBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		bufferBarrier.dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
		bufferBarrier.buffer = indirectCommandsBuffer.buffer;
		bufferBarrier.size = indirectCommandsBuffer.descriptor.range;
		bufferBarrier.srcQueueFamilyIndex = vulkanDevice->queueFamilyIndices.compute;
		bufferBarrier.dstQueueFamilyIndex = vulkanDevice->queueFamilyIndices.graphics;

		vkCmdPipelineBarrier(
			compute.commandBuffer,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
			VK_FLAGS_NONE,
			0, nullptr,
			1, &bufferBarrier,
			0, nullptr);

		// todo: barrier for indirect stats buffer?

		vkEndCommandBuffer(compute.commandBuffer);
	}

	void setupDescriptorPool()
	{
		std::vector<VkDescriptorPoolSize> poolSizes = {
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2),
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4)
		};
		VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, 2);
		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));
	}

	void setupDescriptorSetLayout()
	{
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
			// Binding 0: Vertex shader uniform buffer
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT,0),
		};
		VkDescriptorSetLayoutCreateInfo descriptorLayout = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayout));

		VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(&descriptorSetLayout, 1);		 
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &pipelineLayout));
	}

	void setupDescriptorSet()
	{
		VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));

		std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
			// Binding 0: Vertex shader uniform buffer
			vks::initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformData.scene.descriptor),
		};
		vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
	}

	void preparePipelines()
	{
		// This example uses two different input states, one for the instanced part and one for non-instanced rendering
		VkPipelineVertexInputStateCreateInfo inputState = vks::initializers::pipelineVertexInputStateCreateInfo();
		std::vector<VkVertexInputBindingDescription> bindingDescriptions;
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;

		// Vertex input bindings
		// The instancing pipeline uses a vertex input state with two bindings
		bindingDescriptions = {
		    // Binding point 0: Mesh vertex layout description at per-vertex rate
		    vks::initializers::vertexInputBindingDescription(VERTEX_BUFFER_BIND_ID, sizeof(vkglTF::Vertex), VK_VERTEX_INPUT_RATE_VERTEX),
		    // Binding point 1: Instanced data at per-instance rate
		    vks::initializers::vertexInputBindingDescription(INSTANCE_BUFFER_BIND_ID, sizeof(InstanceData), VK_VERTEX_INPUT_RATE_INSTANCE)
		};

		// Vertex attribute bindings
		attributeDescriptions = {
		    // Per-vertex attributes
		    // These are advanced for each vertex fetched by the vertex shader
		    vks::initializers::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vkglTF::Vertex, pos)),	// Location 0: Position
		    vks::initializers::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vkglTF::Vertex, normal)),	// Location 1: Normal
		    vks::initializers::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vkglTF::Vertex, color)),	// Location 2: Texture coordinates
		    // Per-Instance attributes
		    // These are fetched for each instance rendered
		    vks::initializers::vertexInputAttributeDescription(INSTANCE_BUFFER_BIND_ID, 4, VK_FORMAT_R32G32B32_SFLOAT, offsetof(InstanceData, pos)),	// Location 4: Position
		    vks::initializers::vertexInputAttributeDescription(INSTANCE_BUFFER_BIND_ID, 5, VK_FORMAT_R32_SFLOAT, offsetof(InstanceData, scale)),		// Location 5: Scale
		};
		inputState.pVertexBindingDescriptions = bindingDescriptions.data();
		inputState.pVertexAttributeDescriptions = attributeDescriptions.data();
		inputState.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
		inputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
		VkPipelineRasterizationStateCreateInfo rasterizationState = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
		VkPipelineColorBlendAttachmentState blendAttachmentState = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
		VkPipelineColorBlendStateCreateInfo colorBlendState = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
		VkPipelineDepthStencilStateCreateInfo depthStencilState = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
		VkPipelineViewportStateCreateInfo viewportState = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
		VkPipelineMultisampleStateCreateInfo multisampleState = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
		std::vector<VkDynamicState> dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
		VkPipelineDynamicStateCreateInfo dynamicState = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

		VkGraphicsPipelineCreateInfo pipelineCreateInfo = vks::initializers::pipelineCreateInfo(pipelineLayout, renderPass);
		pipelineCreateInfo.pVertexInputState = &inputState;
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
		shaderStages[0] = loadShader(getShadersPath() + "computecullandlod/indirectdraw.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getShadersPath() + "computecullandlod/indirectdraw.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.plants));
	}

	void prepareBuffers()
	{
		objectCount = OBJECT_COUNT * OBJECT_COUNT * OBJECT_COUNT;

		vks::Buffer stagingBuffer;

		std::vector<InstanceData> instanceData(objectCount);
		indirectCommands.resize(objectCount);

		// Indirect draw commands
		for (uint32_t x = 0; x < OBJECT_COUNT; x++)
		{
			for (uint32_t y = 0; y < OBJECT_COUNT; y++)
			{
				for (uint32_t z = 0; z < OBJECT_COUNT; z++)
				{
					uint32_t index = x + y * OBJECT_COUNT + z * OBJECT_COUNT * OBJECT_COUNT;
					indirectCommands[index].instanceCount = 1;
					indirectCommands[index].firstInstance = index;
					// firstIndex and indexCount are written by the compute shader
				}
			}
		}

		indirectStats.drawCount = static_cast<uint32_t>(indirectCommands.size());

		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&stagingBuffer,
			indirectCommands.size() * sizeof(VkDrawIndexedIndirectCommand),
			indirectCommands.data()));

		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			&indirectCommandsBuffer,
			stagingBuffer.size));

		vulkanDevice->copyBuffer(&stagingBuffer, &indirectCommandsBuffer, queue);

		stagingBuffer.destroy();

		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&indirectDrawCountBuffer,
			sizeof(indirectStats)));

		// Map for host access
		VK_CHECK_RESULT(indirectDrawCountBuffer.map());

		// Instance data
		for (uint32_t x = 0; x < OBJECT_COUNT; x++)
		{
			for (uint32_t y = 0; y < OBJECT_COUNT; y++)
			{
				for (uint32_t z = 0; z < OBJECT_COUNT; z++)
				{
					uint32_t index = x + y * OBJECT_COUNT + z * OBJECT_COUNT * OBJECT_COUNT;
					instanceData[index].pos = glm::vec3((float)x, (float)y, (float)z) - glm::vec3((float)OBJECT_COUNT / 2.0f);
					instanceData[index].scale = 2.0f;
				}
			}
		}

		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&stagingBuffer,
			instanceData.size() * sizeof(InstanceData),
			instanceData.data()));

		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			&instanceBuffer,
			stagingBuffer.size));

		vulkanDevice->copyBuffer(&stagingBuffer, &instanceBuffer, queue);

		stagingBuffer.destroy();

		// Shader storage buffer containing index offsets and counts for the LODs
		struct LOD
		{
			uint32_t firstIndex;
			uint32_t indexCount;
			float distance;
			float _pad0;
		};
		std::vector<LOD> LODLevels;
		uint32_t n = 0;
		for (auto node : lodModel.nodes)
		{
			LOD lod;
			lod.firstIndex = node->mesh->primitives[0]->firstIndex;	// First index for this LOD
			lod.indexCount = node->mesh->primitives[0]->indexCount;	// Index count for this LOD
			lod.distance = 5.0f + n * 5.0f;							// Starting distance (to viewer) for this LOD
			n++;
			LODLevels.push_back(lod);
		}

		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&stagingBuffer,
			LODLevels.size() * sizeof(LOD),
			LODLevels.data()));

		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			&compute.lodLevelsBuffers,
			stagingBuffer.size));

		vulkanDevice->copyBuffer(&stagingBuffer, &compute.lodLevelsBuffers, queue);

		stagingBuffer.destroy();

		// Scene uniform buffer
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&uniformData.scene,
			sizeof(uboScene)));

		VK_CHECK_RESULT(uniformData.scene.map());

		updateUniformBuffer(true);
	}

	void prepareCompute()
	{
		// Get a compute capable device queue
		vkGetDeviceQueue(device, vulkanDevice->queueFamilyIndices.compute, 0, &compute.queue);

		// Create compute pipeline
		// Compute pipelines are created separate from graphics pipelines even if they use the same queue (family index)

		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
			// Binding 0: Instance input data buffer
			vks::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				VK_SHADER_STAGE_COMPUTE_BIT,
				0),
			// Binding 1: Indirect draw command output buffer (input)
			vks::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				VK_SHADER_STAGE_COMPUTE_BIT,
				1),
			// Binding 2: Uniform buffer with global matrices (input)
			vks::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				VK_SHADER_STAGE_COMPUTE_BIT,
				2),
			// Binding 3: Indirect draw stats (output)
			vks::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				VK_SHADER_STAGE_COMPUTE_BIT,
				3),
			// Binding 4: LOD info (input)
			vks::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				VK_SHADER_STAGE_COMPUTE_BIT,
				4),
		};

		VkDescriptorSetLayoutCreateInfo descriptorLayout =
			vks::initializers::descriptorSetLayoutCreateInfo(
				setLayoutBindings.data(),
				static_cast<uint32_t>(setLayoutBindings.size()));

		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &compute.descriptorSetLayout));

		VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo =
			vks::initializers::pipelineLayoutCreateInfo(
				&compute.descriptorSetLayout,
				1);

		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &compute.pipelineLayout));

		VkDescriptorSetAllocateInfo allocInfo =
			vks::initializers::descriptorSetAllocateInfo(
				descriptorPool,
				&compute.descriptorSetLayout,
				1);

		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &compute.descriptorSet));

		std::vector<VkWriteDescriptorSet> computeWriteDescriptorSets =
		{
			// Binding 0: Instance input data buffer
			vks::initializers::writeDescriptorSet(
				compute.descriptorSet,
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				0,
				&instanceBuffer.descriptor),
			// Binding 1: Indirect draw command output buffer
			vks::initializers::writeDescriptorSet(
				compute.descriptorSet,
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				1,
				&indirectCommandsBuffer.descriptor),
			// Binding 2: Uniform buffer with global matrices
			vks::initializers::writeDescriptorSet(
				compute.descriptorSet,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				2,
				&uniformData.scene.descriptor),
			// Binding 3: Atomic counter (written in shader)
			vks::initializers::writeDescriptorSet(
				compute.descriptorSet,
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				3,
				&indirectDrawCountBuffer.descriptor),
			// Binding 4: LOD info
			vks::initializers::writeDescriptorSet(
				compute.descriptorSet,
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				4,
				&compute.lodLevelsBuffers.descriptor)
		};

		vkUpdateDescriptorSets(device, static_cast<uint32_t>(computeWriteDescriptorSets.size()), computeWriteDescriptorSets.data(), 0, NULL);

		// Create pipeline
		VkComputePipelineCreateInfo computePipelineCreateInfo = vks::initializers::computePipelineCreateInfo(compute.pipelineLayout, 0);
		computePipelineCreateInfo.stage = loadShader(getShadersPath() + "computecullandlod/cull.comp.spv", VK_SHADER_STAGE_COMPUTE_BIT);

		// Use specialization constants to pass max. level of detail (determined by no. of meshes)
		VkSpecializationMapEntry specializationEntry{};
		specializationEntry.constantID = 0;
		specializationEntry.offset = 0;
		specializationEntry.size = sizeof(uint32_t);

		uint32_t specializationData = static_cast<uint32_t>(lodModel.nodes.size()) - 1;

		VkSpecializationInfo specializationInfo;
		specializationInfo.mapEntryCount = 1;
		specializationInfo.pMapEntries = &specializationEntry;
		specializationInfo.dataSize = sizeof(specializationData);
		specializationInfo.pData = &specializationData;

		computePipelineCreateInfo.stage.pSpecializationInfo = &specializationInfo;

		VK_CHECK_RESULT(vkCreateComputePipelines(device, pipelineCache, 1, &computePipelineCreateInfo, nullptr, &compute.pipeline));

		// Separate command pool as queue family for compute may be different than graphics
		VkCommandPoolCreateInfo cmdPoolInfo = {};
		cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolInfo.queueFamilyIndex = vulkanDevice->queueFamilyIndices.compute;
		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		VK_CHECK_RESULT(vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &compute.commandPool));

		// Create a command buffer for compute operations
		VkCommandBufferAllocateInfo cmdBufAllocateInfo =
			vks::initializers::commandBufferAllocateInfo(
				compute.commandPool,
				VK_COMMAND_BUFFER_LEVEL_PRIMARY,
				1);

		VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &compute.commandBuffer));

		// Fence for compute CB sync
		VkFenceCreateInfo fenceCreateInfo = vks::initializers::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
		VK_CHECK_RESULT(vkCreateFence(device, &fenceCreateInfo, nullptr, &compute.fence));

		VkSemaphoreCreateInfo semaphoreCreateInfo = vks::initializers::semaphoreCreateInfo();
		VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &compute.semaphore));

		// Build a single command buffer containing the compute dispatch commands
		buildComputeCommandBuffer();
	}

	void updateUniformBuffer(bool viewChanged)
	{
		if (viewChanged)
		{
			uboScene.projection = camera.matrices.perspective;
			uboScene.modelview = camera.matrices.view;
			if (!fixedFrustum)
			{
				uboScene.cameraPos = glm::vec4(camera.position, 1.0f) * -1.0f;
				frustum.update(uboScene.projection * uboScene.modelview);
				memcpy(uboScene.frustumPlanes, frustum.planes.data(), sizeof(glm::vec4) * 6);
			}
		}

		memcpy(uniformData.scene.mapped, &uboScene, sizeof(uboScene));
	}

	void draw()
	{
		VulkanExampleBase::prepareFrame();

		// Submit compute shader for frustum culling

		// Wait for fence to ensure that compute buffer writes have finished
		vkWaitForFences(device, 1, &compute.fence, VK_TRUE, UINT64_MAX);
		vkResetFences(device, 1, &compute.fence);

		VkSubmitInfo computeSubmitInfo = vks::initializers::submitInfo();
		computeSubmitInfo.commandBufferCount = 1;
		computeSubmitInfo.pCommandBuffers = &compute.commandBuffer;
		computeSubmitInfo.signalSemaphoreCount = 1;
		computeSubmitInfo.pSignalSemaphores = &compute.semaphore;

		VK_CHECK_RESULT(vkQueueSubmit(compute.queue, 1, &computeSubmitInfo, VK_NULL_HANDLE));

		// Submit graphics command buffer

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];

		// Wait on present and compute semaphores
		std::array<VkPipelineStageFlags,2> stageFlags = {
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		};
		std::array<VkSemaphore,2> waitSemaphores = {
			semaphores.presentComplete,						// Wait for presentation to finished
			compute.semaphore								// Wait for compute to finish
		};

		submitInfo.pWaitSemaphores = waitSemaphores.data();
		submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
		submitInfo.pWaitDstStageMask = stageFlags.data();

		// Submit to queue
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, compute.fence));

		VulkanExampleBase::submitFrame();

		// Get draw count from compute
		memcpy(&indirectStats, indirectDrawCountBuffer.mapped, sizeof(indirectStats));
	}

	void prepare()
	{
		VulkanExampleBase::prepare();
		loadAssets();
		prepareBuffers();
		setupDescriptorSetLayout();
		preparePipelines();
		setupDescriptorPool();
		setupDescriptorSet();
		prepareCompute();
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
		if (camera.updated)
		{
			updateUniformBuffer(true);
		}
	}

	virtual void OnUpdateUIOverlay(vks::UIOverlay *overlay)
	{
		if (overlay->header("Settings")) {
			if (overlay->checkBox("Freeze frustum", &fixedFrustum)) {
				updateUniformBuffer(true);
			}
		}
		if (overlay->header("Statistics")) {
			overlay->text("Visible objects: %d", indirectStats.drawCount);
			for (uint32_t i = 0; i < MAX_LOD_LEVEL + 1; i++) {
				overlay->text("LOD %d: %d", i, indirectStats.lodCount[i]);
			}
		}
	}
};

VULKAN_EXAMPLE_MAIN()