/*
* Vulkan Example - Compute shader culling and LOD using indirect rendering
*
* Copyright (C) 2016-2025 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*
*/

#include "vulkanexamplebase.h"
#include "VulkanglTFModel.h"
#include "frustum.hpp"


// Total number of objects (^3) in the scene
#if defined(__ANDROID__)
constexpr auto OBJECT_COUNT = 32;
#else
constexpr auto OBJECT_COUNT = 64;
#endif

constexpr auto MAX_LOD_LEVEL = 5;

class VulkanExample : public VulkanExampleBase
{
public:
	bool fixedFrustum = false;

	// The model contains multiple versions of a single object with different levels of detail
	vkglTF::Model lodModel;

	// Per-instance data block
	struct InstanceData {
		glm::vec3 pos{ 0.0f };
		float scale{ 1.0f };
	};

	// Contains the instanced data
	vks::Buffer instanceBuffer;
	// Contains the indirect drawing commands
	std::array<vks::Buffer, maxConcurrentFrames> indirectCommandsBuffers;
	std::array<vks::Buffer, maxConcurrentFrames> indirectDrawCountBuffers;

	// Indirect draw statistics (updated via compute)
	struct {
		uint32_t drawCount;						// Total number of indirect draw counts to be issued
		uint32_t lodCount[MAX_LOD_LEVEL + 1];	// Statistics for number of draws per LOD level (written by compute shader)
	} indirectStats{};

	// Store the indirect draw commands containing index offsets and instance count per object
	std::vector<VkDrawIndexedIndirectCommand> indirectCommands;

	struct UniformData {
		glm::mat4 projection;
		glm::mat4 modelview;
		glm::vec4 cameraPos;
		glm::vec4 frustumPlanes[6];
	} uniformData;
	std::array<vks::Buffer, maxConcurrentFrames> uniformBuffers;

	VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };
	VkPipeline pipeline{ VK_NULL_HANDLE };
	VkDescriptorSetLayout descriptorSetLayout{ VK_NULL_HANDLE };
	std::array<VkDescriptorSet, maxConcurrentFrames> descriptorSets{};

	// Resources for the compute part of the example
	struct Compute {
		vks::Buffer lodLevelsBuffers;										// Contains index start and counts for the different lod levels
		VkQueue queue;														// Separate queue for compute commands (queue family may differ from the one used for graphics)
		VkCommandPool commandPool;											// Use a separate command pool (queue family may differ from the one used for graphics)
		std::array<VkCommandBuffer, maxConcurrentFrames> commandBuffers;	// Command buffer storing the dispatch commands and barriers
		std::array<VkFence, maxConcurrentFrames> fences;					// Synchronization fence to avoid rewriting compute CB if still in use
		struct ComputeSemaphores {
			VkSemaphore ready{ VK_NULL_HANDLE };
			VkSemaphore complete{ VK_NULL_HANDLE };
		};
		std::array<ComputeSemaphores, maxConcurrentFrames> semaphores{};	// Used as a wait semaphore for graphics submission
		//std::array<VkSemaphore, maxConcurrentFrames> semaphores;			// Used as a wait semaphore for graphics submission
		VkDescriptorSetLayout descriptorSetLayout;							// Compute shader binding layout
		std::array<VkDescriptorSet, maxConcurrentFrames> descriptorSets{};	// Compute shader bindings
		VkPipelineLayout pipelineLayout;									// Layout of the compute pipeline
		VkPipeline pipeline;												// Compute pipeline
	} compute{};

	// View frustum for culling invisible objects
	vks::Frustum frustum;

	uint32_t objectCount = 0;

	VulkanExample() : VulkanExampleBase()
	{
		title = "Compute cull and lod";
		camera.type = Camera::CameraType::firstperson;
		camera.setPerspective(60.0f, (float)width / (float)height, 0.1f, 512.0f);
		camera.setTranslation(glm::vec3(0.5f, 0.0f, 0.0f));
		camera.movementSpeed = 5.0f;
	}

	~VulkanExample()
	{
		if (device) {
			vkDestroyPipeline(device, pipeline, nullptr);
			vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
			vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
			instanceBuffer.destroy();
			for (auto& buffer : uniformBuffers) {
				buffer.destroy();
			}
			for (auto& buffer : indirectDrawCountBuffers) {
				buffer.destroy();
			}
			for (auto& buffer : indirectCommandsBuffers) {
				buffer.destroy();
			}
			compute.lodLevelsBuffers.destroy();
			vkDestroyPipelineLayout(device, compute.pipelineLayout, nullptr);
			vkDestroyDescriptorSetLayout(device, compute.descriptorSetLayout, nullptr);
			vkDestroyPipeline(device, compute.pipeline, nullptr);
			vkDestroyCommandPool(device, compute.commandPool, nullptr);
			for (auto& fence : compute.fences) {
				vkDestroyFence(device, fence, nullptr);
			}
			for (auto& semaphore : compute.semaphores) {
				vkDestroySemaphore(device, semaphore.complete, nullptr);
				vkDestroySemaphore(device, semaphore.ready, nullptr);
			}

		}
	}

	virtual void getEnabledFeatures()
	{
		// Enable multi draw indirect if supported
		if (deviceFeatures.multiDrawIndirect) {
			enabledFeatures.multiDrawIndirect = VK_TRUE;
		}
		// This is required for for using firstInstance
		enabledFeatures.drawIndirectFirstInstance = VK_TRUE;
	}


	void loadAssets()
	{
		const uint32_t glTFLoadingFlags = vkglTF::FileLoadingFlags::PreTransformVertices | vkglTF::FileLoadingFlags::PreMultiplyVertexColors | vkglTF::FileLoadingFlags::FlipY;
		lodModel.loadFromFile(getAssetPath() + "models/suzanne_lods.gltf", vulkanDevice, queue, glTFLoadingFlags);
	}

	void prepareDescriptorPool()
	{
		// This is shared between graphics and compute
		std::vector<VkDescriptorPoolSize> poolSizes = {
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, maxConcurrentFrames * 2),
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, maxConcurrentFrames * 4)
		};
		VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, maxConcurrentFrames * 2);
		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));
	}

	void prepareGraphics()
	{
		// Layout
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
			// Binding 0: Vertex shader uniform buffer
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT,0),
		};
		VkDescriptorSetLayoutCreateInfo descriptorLayout = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayout));

		// Sets per frame, just like the buffers themselves
		for (auto i = 0; i < uniformBuffers.size(); i++) {
			VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);
			VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets[i]));
			std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
				// Binding 0: Vertex shader uniform buffer
				vks::initializers::writeDescriptorSet(descriptorSets[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffers[i].descriptor),
			};
			vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
		}

		// Pipeline layout
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(&descriptorSetLayout, 1);
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

		// This example uses two different input states, one for the instanced part and one for non-instanced rendering
		VkPipelineVertexInputStateCreateInfo inputState = vks::initializers::pipelineVertexInputStateCreateInfo();
		std::vector<VkVertexInputBindingDescription> bindingDescriptions;
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;

		// Vertex input bindings
		// The instancing pipeline uses a vertex input state with two bindings
		bindingDescriptions = {
		    // Binding point 0: Mesh vertex layout description at per-vertex rate
		    vks::initializers::vertexInputBindingDescription(0, sizeof(vkglTF::Vertex), VK_VERTEX_INPUT_RATE_VERTEX),
		    // Binding point 1: Instanced data at per-instance rate
		    vks::initializers::vertexInputBindingDescription(1, sizeof(InstanceData), VK_VERTEX_INPUT_RATE_INSTANCE)
		};

		// Vertex attribute bindings
		attributeDescriptions = {
		    // Per-vertex attributes
		    // These are advanced for each vertex fetched by the vertex shader
		    vks::initializers::vertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vkglTF::Vertex, pos)),	// Location 0: Position
		    vks::initializers::vertexInputAttributeDescription(0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vkglTF::Vertex, normal)),	// Location 1: Normal
		    vks::initializers::vertexInputAttributeDescription(0, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vkglTF::Vertex, color)),	// Location 2: Texture coordinates
		    // Per-Instance attributes
		    // These are fetched for each instance rendered
		    vks::initializers::vertexInputAttributeDescription(1, 3, VK_FORMAT_R32G32B32_SFLOAT, offsetof(InstanceData, pos)),	// Location 4: Position
		    vks::initializers::vertexInputAttributeDescription(1, 4, VK_FORMAT_R32_SFLOAT, offsetof(InstanceData, scale)),		// Location 5: Scale
		};
		inputState.pVertexBindingDescriptions = bindingDescriptions.data();
		inputState.pVertexAttributeDescriptions = attributeDescriptions.data();
		inputState.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
		inputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());

		// Indirect (and instanced) pipeline
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
		VkPipelineRasterizationStateCreateInfo rasterizationState = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
		VkPipelineColorBlendAttachmentState blendAttachmentState = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
		VkPipelineColorBlendStateCreateInfo colorBlendState = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
		VkPipelineDepthStencilStateCreateInfo depthStencilState = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
		VkPipelineViewportStateCreateInfo viewportState = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
		VkPipelineMultisampleStateCreateInfo multisampleState = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
		std::vector<VkDynamicState> dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
		VkPipelineDynamicStateCreateInfo dynamicState = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
		VkGraphicsPipelineCreateInfo pipelineCreateInfo = vks::initializers::pipelineCreateInfo(pipelineLayout, renderPass);
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = {
			loadShader(getShadersPath() + "computecullandlod/indirectdraw.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
			loadShader(getShadersPath() + "computecullandlod/indirectdraw.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT)
		};
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
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipeline));
	}

	void prepareBuffers()
	{
		objectCount = OBJECT_COUNT * OBJECT_COUNT * OBJECT_COUNT;

		vks::Buffer stagingBuffer;

		indirectCommands.resize(objectCount);

		// Indirect draw commands

		for (uint32_t x = 0; x < OBJECT_COUNT; x++) {
			for (uint32_t y = 0; y < OBJECT_COUNT; y++) {
				for (uint32_t z = 0; z < OBJECT_COUNT; z++) {
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

		for (auto& indirectCommandsBuffer : indirectCommandsBuffers) {
			VK_CHECK_RESULT(vulkanDevice->createBuffer(
				VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				&indirectCommandsBuffer,
				stagingBuffer.size));

			vulkanDevice->copyBuffer(&stagingBuffer, &indirectCommandsBuffer, queue);

			// Add an initial release barrier to the graphics queue,
			// so that when the compute command buffer executes for the first time
			// it doesn't complain about a lack of a corresponding "release" to its "acquire"
			VkCommandBuffer barrierCmd = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
			if (vulkanDevice->queueFamilyIndices.graphics != vulkanDevice->queueFamilyIndices.compute)
			{
				VkBufferMemoryBarrier buffer_barrier =
				{
					VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
					nullptr,
					VK_ACCESS_INDIRECT_COMMAND_READ_BIT,
					0,
					vulkanDevice->queueFamilyIndices.graphics,
					vulkanDevice->queueFamilyIndices.compute,
					indirectCommandsBuffer.buffer,
					0,
					indirectCommandsBuffer.descriptor.range
				};

				vkCmdPipelineBarrier(
					barrierCmd,
					VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
					VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
					0,
					0, nullptr,
					1, &buffer_barrier,
					0, nullptr);
			}
			vulkanDevice->flushCommandBuffer(barrierCmd, queue, true);
		}

		stagingBuffer.destroy();

		// Instance data
		std::vector<InstanceData> instanceData(objectCount);

		for (uint32_t x = 0; x < OBJECT_COUNT; x++) {
			for (uint32_t y = 0; y < OBJECT_COUNT; y++) {
				for (uint32_t z = 0; z < OBJECT_COUNT; z++) {
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

		// Draw count buffer for host side info readback
		for (auto& indirectDrawCountBuffer : indirectDrawCountBuffers) {

			VK_CHECK_RESULT(vulkanDevice->createBuffer(
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				&indirectDrawCountBuffer,
				sizeof(indirectStats)));

			// Map for host access
			VK_CHECK_RESULT(indirectDrawCountBuffer.map());
		}


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
			LOD lod{};
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
		for (auto& buffer : uniformBuffers) {
			VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &buffer, sizeof(UniformData)));
			VK_CHECK_RESULT(buffer.map());
		}
	}

	void prepareCompute()
	{
		// Get a compute capable device queue
		vkGetDeviceQueue(device, vulkanDevice->queueFamilyIndices.compute, 0, &compute.queue);

		// Create compute pipeline
		// Compute pipelines are created separate from graphics pipelines even if they use the same queue (family index)

		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
			// Binding 0: Instance input data buffer
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 0),
			// Binding 1: Indirect draw command output buffer (input)
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 1),
			// Binding 2: Uniform buffer with global matrices (input)
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 2),
			// Binding 3: Indirect draw stats (output)
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 3),
			// Binding 4: LOD info (input)
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT,4),
		};
		VkDescriptorSetLayoutCreateInfo descriptorLayout = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &compute.descriptorSetLayout));

		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(&compute.descriptorSetLayout, 1);
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &compute.pipelineLayout));

		for (auto i = 0; i < uniformBuffers.size(); i++) {
			VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &compute.descriptorSetLayout, 1);
			VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &compute.descriptorSets[i]));
			std::vector<VkWriteDescriptorSet> computeWriteDescriptorSets = {
				// Binding 0: Instance input data buffer
				vks::initializers::writeDescriptorSet(compute.descriptorSets[i], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, &instanceBuffer.descriptor),
				// Binding 1: Indirect draw command output buffer
				vks::initializers::writeDescriptorSet(compute.descriptorSets[i], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, &indirectCommandsBuffers[i].descriptor),
				// Binding 2: Uniform buffer with global matrices
				vks::initializers::writeDescriptorSet(compute.descriptorSets[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, &uniformBuffers[i].descriptor),
				// Binding 3: Atomic counter (written in shader)
				vks::initializers::writeDescriptorSet(compute.descriptorSets[i], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3, &indirectDrawCountBuffers[i].descriptor),
				// Binding 4: LOD info
				vks::initializers::writeDescriptorSet(compute.descriptorSets[i], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4, &compute.lodLevelsBuffers.descriptor)
			};
			vkUpdateDescriptorSets(device, static_cast<uint32_t>(computeWriteDescriptorSets.size()), computeWriteDescriptorSets.data(), 0, nullptr);
		}

		// Create pipeline
		VkComputePipelineCreateInfo computePipelineCreateInfo = vks::initializers::computePipelineCreateInfo(compute.pipelineLayout, 0);
		computePipelineCreateInfo.stage = loadShader(getShadersPath() + "computecullandlod/cull.comp.spv", VK_SHADER_STAGE_COMPUTE_BIT);

		// Use specialization constants to pass max. level of detail (determined by no. of meshes)
		VkSpecializationMapEntry specializationEntry{};
		specializationEntry.constantID = 0;
		specializationEntry.offset = 0;
		specializationEntry.size = sizeof(uint32_t);

		uint32_t specializationData = static_cast<uint32_t>(lodModel.nodes.size()) - 1;

		VkSpecializationInfo specializationInfo{};
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

		// Create command buffers for compute operations
		for (auto& cmdBuffer : compute.commandBuffers) {
			cmdBuffer = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, compute.commandPool);
		}

		// Fences to check for command buffer completion
		for (auto& fence : compute.fences) {
			VkFenceCreateInfo fenceCreateInfo = vks::initializers::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
			VK_CHECK_RESULT(vkCreateFence(device, &fenceCreateInfo, nullptr, &fence));
		}

		// Semaphores to order compute and graphics submissions
		for (auto& semaphore : compute.semaphores) {
			VkSemaphoreCreateInfo semaphoreInfo{ .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
			VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphore.complete));
			VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphore.ready));
		}
		// Signal first used ready semaphore
		VkSubmitInfo computeSubmitInfo = vks::initializers::submitInfo();
		computeSubmitInfo.signalSemaphoreCount = 1;
		computeSubmitInfo.pSignalSemaphores = &compute.semaphores[maxConcurrentFrames - 1].ready;
		VK_CHECK_RESULT(vkQueueSubmit(compute.queue, 1, &computeSubmitInfo, VK_NULL_HANDLE));
	}

	void updateUniformBuffer()
	{
		uniformData.projection = camera.matrices.perspective;
		uniformData.modelview = camera.matrices.view;
		if (!fixedFrustum) {
			uniformData.cameraPos = glm::vec4(camera.position, 1.0f) * -1.0f;
			frustum.update(uniformData.projection * uniformData.modelview);
			memcpy(uniformData.frustumPlanes, frustum.planes.data(), sizeof(glm::vec4) * 6);
		}
		memcpy(uniformBuffers[currentBuffer].mapped, &uniformData, sizeof(UniformData));
	}

	void prepare()
	{
		VulkanExampleBase::prepare();
		loadAssets();
		prepareBuffers();
		prepareDescriptorPool();
		prepareGraphics();
		prepareCompute();
		prepared = true;
	}

	void buildGraphicsCommandBuffer()
	{
		VkCommandBuffer cmdBuffer = drawCmdBuffers[currentBuffer];
		
		VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

		VkClearValue clearValues[2]{};
		clearValues[0].color = { { 0.18f, 0.27f, 0.5f, 0.0f } };
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
		renderPassBeginInfo.renderPass = renderPass;
		renderPassBeginInfo.renderArea.extent.width = width;
		renderPassBeginInfo.renderArea.extent.height = height;
		renderPassBeginInfo.clearValueCount = 2;
		renderPassBeginInfo.pClearValues = clearValues;
		renderPassBeginInfo.framebuffer = frameBuffers[currentImageIndex];

		VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));

		// Acquire barrier
		if (vulkanDevice->queueFamilyIndices.graphics != vulkanDevice->queueFamilyIndices.compute)
		{
			VkBufferMemoryBarrier buffer_barrier =
			{
				VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
				nullptr,
				0,
				VK_ACCESS_INDIRECT_COMMAND_READ_BIT,
				vulkanDevice->queueFamilyIndices.compute,
				vulkanDevice->queueFamilyIndices.graphics,
				indirectCommandsBuffers[currentBuffer].buffer,
				0,
				indirectCommandsBuffers[currentBuffer].descriptor.range
			};

			vkCmdPipelineBarrier(
				cmdBuffer,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
				0,
				0, nullptr,
				1, &buffer_barrier,
				0, nullptr);
		}

		vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
		vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

		VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);
		vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentBuffer], 0, nullptr);

		// Mesh containing the LODs
		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &lodModel.vertices.buffer, offsets);
		vkCmdBindVertexBuffers(cmdBuffer, 1, 1, &instanceBuffer.buffer, offsets);

		vkCmdBindIndexBuffer(cmdBuffer, lodModel.indices.buffer, 0, VK_INDEX_TYPE_UINT32);

		if (vulkanDevice->features.multiDrawIndirect)
		{
			vkCmdDrawIndexedIndirect(cmdBuffer, indirectCommandsBuffers[currentBuffer].buffer, 0, static_cast<uint32_t>(indirectCommands.size()), sizeof(VkDrawIndexedIndirectCommand));
		}
		else
		{
			// If multi draw is not available, we must issue separate draw commands
			for (auto j = 0; j < indirectCommands.size(); j++)
			{
				vkCmdDrawIndexedIndirect(cmdBuffer, indirectCommandsBuffers[currentBuffer].buffer, j * sizeof(VkDrawIndexedIndirectCommand), 1, sizeof(VkDrawIndexedIndirectCommand));
			}
		}

		drawUI(cmdBuffer);

		vkCmdEndRenderPass(cmdBuffer);

		// Release barrier
		if (vulkanDevice->queueFamilyIndices.graphics != vulkanDevice->queueFamilyIndices.compute)
		{
			VkBufferMemoryBarrier buffer_barrier =
			{
				VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
				nullptr,
				VK_ACCESS_INDIRECT_COMMAND_READ_BIT,
				0,
				vulkanDevice->queueFamilyIndices.graphics,
				vulkanDevice->queueFamilyIndices.compute,
				indirectCommandsBuffers[currentBuffer].buffer,
				0,
				indirectCommandsBuffers[currentBuffer].descriptor.range
			};

			vkCmdPipelineBarrier(
				cmdBuffer,
				VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
				VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
				0,
				0, nullptr,
				1, &buffer_barrier,
				0, nullptr);
		}

		VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffer));
	}


	void buildComputeCommandBuffer()
	{
		VkCommandBuffer cmdBuffer = compute.commandBuffers[currentBuffer];
		
		VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

		VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));

		// Acquire barrier
		// Add memory barrier to ensure that the indirect commands have been consumed before the compute shader updates them
		if (vulkanDevice->queueFamilyIndices.graphics != vulkanDevice->queueFamilyIndices.compute)
		{
			VkBufferMemoryBarrier buffer_barrier =
			{
				VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
				nullptr,
				0,
				VK_ACCESS_SHADER_WRITE_BIT,
				vulkanDevice->queueFamilyIndices.graphics,
				vulkanDevice->queueFamilyIndices.compute,
				indirectCommandsBuffers[currentBuffer].buffer,
				0,
				indirectCommandsBuffers[currentBuffer].descriptor.range
			};

			vkCmdPipelineBarrier(
				cmdBuffer,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_FLAGS_NONE,
				0, nullptr,
				1, &buffer_barrier,
				0, nullptr);
		}

		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute.pipeline);
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute.pipelineLayout, 0, 1, &compute.descriptorSets[currentBuffer], 0, nullptr);

		// Clear the buffer that the compute shader pass will write statistics and draw calls to
		vkCmdFillBuffer(cmdBuffer, indirectDrawCountBuffers[currentBuffer].buffer, 0, indirectDrawCountBuffers[currentBuffer].descriptor.range, 0);

		// This barrier ensures that the fill command is finished before the compute shader can start writing to the buffer
		VkMemoryBarrier memoryBarrier = vks::initializers::memoryBarrier();
		memoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(
			cmdBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_FLAGS_NONE,
			1, &memoryBarrier,
			0, nullptr,
			0, nullptr);

		// Dispatch the compute job
		// The compute shader will do the frustum culling and adjust the indirect draw calls depending on object visibility.
		// It also determines the lod to use depending on distance to the viewer.
		vkCmdDispatch(cmdBuffer, objectCount / 16, 1, 1);

		// Release barrier
		// Add memory barrier to ensure that the compute shader has finished writing the indirect command buffer before it's consumed
		if (vulkanDevice->queueFamilyIndices.graphics != vulkanDevice->queueFamilyIndices.compute)
		{
			VkBufferMemoryBarrier buffer_barrier =
			{
				VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
				nullptr,
				VK_ACCESS_SHADER_WRITE_BIT,
				0,
				vulkanDevice->queueFamilyIndices.compute,
				vulkanDevice->queueFamilyIndices.graphics,
				indirectCommandsBuffers[currentBuffer].buffer,
				0,
				indirectCommandsBuffers[currentBuffer].descriptor.range
			};

			vkCmdPipelineBarrier(
				cmdBuffer,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
				VK_FLAGS_NONE,
				0, nullptr,
				1, &buffer_barrier,
				0, nullptr);
		}

		vkEndCommandBuffer(cmdBuffer);
	}

	virtual void render()
	{
		if (!prepared)
			return;

		// Submit compute commands
		{
			VK_CHECK_RESULT(vkWaitForFences(device, 1, &compute.fences[currentBuffer], VK_TRUE, UINT64_MAX));
			VK_CHECK_RESULT(vkResetFences(device, 1, &compute.fences[currentBuffer]));

			// Get draw count from compute
			memcpy(&indirectStats, indirectDrawCountBuffers[currentBuffer].mapped, sizeof(indirectStats));
			buildComputeCommandBuffer();

			// Wait for rendering finished
			VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			VkSubmitInfo submitInfo = vks::initializers::submitInfo();
			submitInfo.waitSemaphoreCount = 1;
			submitInfo.pWaitSemaphores = &compute.semaphores[((int)currentBuffer - 1) % maxConcurrentFrames].ready;
			submitInfo.pWaitDstStageMask = &waitDstStageMask;
			submitInfo.signalSemaphoreCount = 1;
			submitInfo.pSignalSemaphores = &compute.semaphores[currentBuffer].complete;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &compute.commandBuffers[currentBuffer];
			VK_CHECK_RESULT(vkQueueSubmit(compute.queue, 1, &submitInfo, compute.fences[currentBuffer]));
		}

		// Submit graphics commands
		{
			VK_CHECK_RESULT(vkWaitForFences(device, 1, &waitFences[currentBuffer], VK_TRUE, UINT64_MAX));
			VK_CHECK_RESULT(vkResetFences(device, 1, &waitFences[currentBuffer]));
		
			VulkanExampleBase::prepareFrame(false);

			updateUniformBuffer();
			buildGraphicsCommandBuffer();

			VkPipelineStageFlags waitDstStageMask[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT };
			VkSemaphore waitSemaphores[2] = { presentCompleteSemaphores[currentBuffer], compute.semaphores[currentBuffer].complete };
			VkSemaphore signalSemaphores[2] = { renderCompleteSemaphores[currentImageIndex], compute.semaphores[currentBuffer].ready };

			// Submit graphics commands
			VkSubmitInfo submitInfo = vks::initializers::submitInfo();
			submitInfo.waitSemaphoreCount = 2;
			submitInfo.pWaitDstStageMask = waitDstStageMask;
			submitInfo.pWaitSemaphores = waitSemaphores;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];
			submitInfo.signalSemaphoreCount = 2;
			submitInfo.pSignalSemaphores = signalSemaphores;
			VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, waitFences[currentBuffer]));

			VulkanExampleBase::submitFrame(true);
		}
	}

	virtual void OnUpdateUIOverlay(vks::UIOverlay *overlay)
	{
		if (overlay->header("Settings")) {
			overlay->checkBox("Freeze frustum", &fixedFrustum);
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
