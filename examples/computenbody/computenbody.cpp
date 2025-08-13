/*
* Vulkan Example - Compute shader N-body simulation using two passes and shared compute shader memory
*
* This sample shows how to combine compute and graphics for doing N-body particle simulaton
* It calculates the particle system movement using two separate compute passes: calculating particle positions and integrating particles
* For that a shader storage buffer is used which is then used as a vertex buffer for drawing the particle system with a graphics pipeline
* To optimize performance, the compute shaders use shared memory
*
* Copyright (C) 2016-2025 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "vulkanexamplebase.h"

#if defined(__ANDROID__)
// Lower particle count on Android for performance reasons
constexpr auto PARTICLES_PER_ATTRACTOR = 3 * 1024;
#else
constexpr auto PARTICLES_PER_ATTRACTOR = 4 * 1024;
#endif

class VulkanExample : public VulkanExampleBase
{
public:
	struct Textures {
		vks::Texture2D particle;
		vks::Texture2D gradient;
	} textures{};

	// Particle Definition
	struct Particle {
		glm::vec4 pos;														// xyz = position, w = mass
		glm::vec4 vel;														// xyz = velocity, w = gradient texture position
	};
	uint32_t numParticles{ 0 };

	// We use a shader storage buffer object to store the particlces
	// This is updated by the compute pipeline and displayed as a vertex buffer by the graphics pipeline
	vks::Buffer storageBuffer;

	// Resources for the graphics part of the example
	struct Graphics {
		uint32_t queueFamilyIndex;											// Used to check if compute and graphics queue families differ and require additional barriers
		VkDescriptorSetLayout descriptorSetLayout;							// Particle system rendering shader binding layout
		std::array<VkDescriptorSet, maxConcurrentFrames> descriptorSets;	// Particle system rendering shader bindings
		VkPipelineLayout pipelineLayout;									// Layout of the graphics pipeline
		VkPipeline pipeline;												// Particle rendering pipeline
		struct UniformData {
			glm::mat4 projection;
			glm::mat4 view;
			glm::vec2 screenDim;
		} uniformData;
		std::array<vks::Buffer, maxConcurrentFrames> uniformBuffers;		// Contains scene matrices
	} graphics;

	// Resources for the compute part of the example
	struct Compute {
		uint32_t queueFamilyIndex;											// Used to check if compute and graphics queue families differ and require additional barriers
		VkQueue queue;														// Separate queue for compute commands (queue family may differ from the one used for graphics)
		VkCommandPool commandPool;											// Use a separate command pool (queue family may differ from the one used for graphics)
		std::array<VkCommandBuffer, maxConcurrentFrames> commandBuffers;	// Command buffer storing the dispatch commands and barriers
		VkDescriptorSetLayout descriptorSetLayout;							// Compute shader binding layout
		std::array<VkDescriptorSet, maxConcurrentFrames> descriptorSets;	// Compute shader bindings
		std::array<VkFence, maxConcurrentFrames> fences{};					// Fences to make sure command buffers are done
		struct ComputeSemaphores {
			VkSemaphore ready{ VK_NULL_HANDLE };
			VkSemaphore complete{ VK_NULL_HANDLE };
		};
		std::array<ComputeSemaphores, maxConcurrentFrames> semaphores{};	// Semaphores for submission ordering
		VkPipelineLayout pipelineLayout;									// Layout of the compute pipeline
		VkPipeline pipelineCalculate;										// Compute pipeline for N-Body velocity calculation (1st pass)
		VkPipeline pipelineIntegrate;										// Compute pipeline for euler integration (2nd pass)
		struct UniformData {												// Compute shader uniform block object
			float deltaT{ 0.0f };											// Frame delta time
			int32_t particleCount{ 0 };
			// Parameters used to control the behaviour of the particle system
			float gravity{ 0.002f };
			float power{ 0.75f };
			float soften{ 0.05f };
		} uniformData;
		std::array<vks::Buffer, maxConcurrentFrames> uniformBuffers;		// Uniform buffer object containing particle system parameters
	} compute;

	VulkanExample() : VulkanExampleBase()
	{
		title = "Compute shader N-body system";
		camera.type = Camera::CameraType::lookat;
		camera.setPerspective(60.0f, (float)width / (float)height, 0.1f, 512.0f);
		camera.setRotation(glm::vec3(-26.0f, 75.0f, 0.0f));
		camera.setTranslation(glm::vec3(0.0f, 0.0f, -14.0f));
		camera.movementSpeed = 2.5f;
	}

	~VulkanExample()
	{
		if (device) {
			// Graphics
			vkDestroyPipeline(device, graphics.pipeline, nullptr);
			vkDestroyPipelineLayout(device, graphics.pipelineLayout, nullptr);
			vkDestroyDescriptorSetLayout(device, graphics.descriptorSetLayout, nullptr);
			for (auto& buffer : graphics.uniformBuffers) {
				buffer.destroy();
			}

			// Compute
			vkDestroyPipelineLayout(device, compute.pipelineLayout, nullptr);
			vkDestroyDescriptorSetLayout(device, compute.descriptorSetLayout, nullptr);
			vkDestroyPipeline(device, compute.pipelineCalculate, nullptr);
			vkDestroyPipeline(device, compute.pipelineIntegrate, nullptr);
			vkDestroyCommandPool(device, compute.commandPool, nullptr);
			for (auto& buffer : compute.uniformBuffers) {
				buffer.destroy();
			}
			for (auto& fence : compute.fences) {
				vkDestroyFence(device, fence, nullptr);
			}
			for (auto& semaphore : compute.semaphores) {
				vkDestroySemaphore(device, semaphore.ready, nullptr);
				vkDestroySemaphore(device, semaphore.complete, nullptr);
			}

			storageBuffer.destroy();

			textures.particle.destroy();
			textures.gradient.destroy();
		}
	}

	void loadAssets()
	{
		textures.particle.loadFromFile(getAssetPath() + "textures/particle01_rgba.ktx", VK_FORMAT_R8G8B8A8_UNORM, vulkanDevice, queue);
		textures.gradient.loadFromFile(getAssetPath() + "textures/particle_gradient_rgba.ktx", VK_FORMAT_R8G8B8A8_UNORM, vulkanDevice, queue);
	}

	// Setup and fill the compute shader storage buffers containing the particles
	void prepareStorageBuffers()
	{
		// We mark a few particles as attractors that move along a given path, these will pull in the other particles
		std::vector<glm::vec3> attractors = {
			glm::vec3(5.0f, 0.0f, 0.0f),
			glm::vec3(-5.0f, 0.0f, 0.0f),
			glm::vec3(0.0f, 0.0f, 5.0f),
			glm::vec3(0.0f, 0.0f, -5.0f),
			glm::vec3(0.0f, 4.0f, 0.0f),
			glm::vec3(0.0f, -8.0f, 0.0f),
		};

		numParticles = static_cast<uint32_t>(attractors.size()) * PARTICLES_PER_ATTRACTOR;

		// Initial particle positions
		std::vector<Particle> particleBuffer(numParticles);

		std::default_random_engine rndEngine(benchmark.active ? 0 : (unsigned)time(nullptr));
		std::normal_distribution<float> rndDist(0.0f, 1.0f);

		for (uint32_t i = 0; i < static_cast<uint32_t>(attractors.size()); i++)
		{
			for (uint32_t j = 0; j < PARTICLES_PER_ATTRACTOR; j++)
			{
				Particle& particle = particleBuffer[i * PARTICLES_PER_ATTRACTOR + j];

				// First particle in group as heavy center of gravity
				if (j == 0)
				{
					particle.pos = glm::vec4(attractors[i] * 1.5f, 90000.0f);
					particle.vel = glm::vec4(glm::vec4(0.0f));
				}
				else
				{
					// Position
					glm::vec3 position(attractors[i] + glm::vec3(rndDist(rndEngine), rndDist(rndEngine), rndDist(rndEngine)) * 0.75f);
					float len = glm::length(glm::normalize(position - attractors[i]));
					position.y *= 2.0f - (len * len);

					// Velocity
					glm::vec3 angular = glm::vec3(0.5f, 1.5f, 0.5f) * (((i % 2) == 0) ? 1.0f : -1.0f);
					glm::vec3 velocity = glm::cross((position - attractors[i]), angular) + glm::vec3(rndDist(rndEngine), rndDist(rndEngine), rndDist(rndEngine) * 0.025f);

					float mass = (rndDist(rndEngine) * 0.5f + 0.5f) * 75.0f;
					particle.pos = glm::vec4(position, mass);
					particle.vel = glm::vec4(velocity, 0.0f);
				}

				// Color gradient offset
				particle.vel.w = (float)i * 1.0f / static_cast<uint32_t>(attractors.size());
			}
		}

		compute.uniformData.particleCount = numParticles;

		VkDeviceSize storageBufferSize = particleBuffer.size() * sizeof(Particle);

		// Staging
		// SSBO won't be changed on the host after upload so copy to device local memory

		vks::Buffer stagingBuffer;

		vulkanDevice->createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, storageBufferSize, particleBuffer.data());
		// The SSBO will be used as a storage buffer for the compute pipeline and as a vertex buffer in the graphics pipeline
		vulkanDevice->createBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &storageBuffer, storageBufferSize);

		// Copy from staging buffer to storage buffer
		VkCommandBuffer copyCmd = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
		VkBufferCopy copyRegion = {};
		copyRegion.size = storageBufferSize;
		vkCmdCopyBuffer(copyCmd, stagingBuffer.buffer, storageBuffer.buffer, 1, &copyRegion);
		// Execute a transfer barrier to the compute queue, if necessary
		if (graphics.queueFamilyIndex != compute.queueFamilyIndex)
		{
			VkBufferMemoryBarrier buffer_barrier =
			{
				VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
				nullptr,
				VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
				0,
				graphics.queueFamilyIndex,
				compute.queueFamilyIndex,
				storageBuffer.buffer,
				0,
				storageBuffer.size
			};

			vkCmdPipelineBarrier(
				copyCmd,
				VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
				VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
				0,
				0, nullptr,
				1, &buffer_barrier,
				0, nullptr);
		}
		vulkanDevice->flushCommandBuffer(copyCmd, queue, true);

		stagingBuffer.destroy();
	}

	void prepareDescriptorPool()
	{
		// This is shared between graphics and compute
		std::vector<VkDescriptorPoolSize> poolSizes = {
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, maxConcurrentFrames * 2),
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, maxConcurrentFrames * 1),
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, maxConcurrentFrames * 2)
		};
		VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, maxConcurrentFrames * 2);
		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));
	}

	void prepareGraphics()
	{
		// Vertex shader uniform buffer block
		for (auto& buffer : graphics.uniformBuffers) {
			vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &buffer, sizeof(Graphics::UniformData));
			VK_CHECK_RESULT(buffer.map());
		}

		// Descriptor layout
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings;
		setLayoutBindings = {
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 2),
		};

		VkDescriptorSetLayoutCreateInfo descriptorLayout = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &graphics.descriptorSetLayout));

		// Sets per frame, just like the buffers themselves
		for (auto i = 0; i < graphics.uniformBuffers.size(); i++) {
			VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &graphics.descriptorSetLayout, 1);
			VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &graphics.descriptorSets[i]));
			std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
			   vks::initializers::writeDescriptorSet(graphics.descriptorSets[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &textures.particle.descriptor),
			   vks::initializers::writeDescriptorSet(graphics.descriptorSets[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &textures.gradient.descriptor),
			   vks::initializers::writeDescriptorSet(graphics.descriptorSets[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, &graphics.uniformBuffers[i].descriptor),
			};
			vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
		}

		// Pipeline layout
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(&graphics.descriptorSetLayout, 1);
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &graphics.pipelineLayout));

		// Pipeline
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_POINT_LIST, 0, VK_FALSE);
		VkPipelineRasterizationStateCreateInfo rasterizationState = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
		VkPipelineColorBlendAttachmentState blendAttachmentState = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
		VkPipelineColorBlendStateCreateInfo colorBlendState = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
		VkPipelineDepthStencilStateCreateInfo depthStencilState = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_FALSE, VK_FALSE, VK_COMPARE_OP_ALWAYS);
		VkPipelineViewportStateCreateInfo viewportState = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
		VkPipelineMultisampleStateCreateInfo multisampleState = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
		std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicState = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};

		// Vertex Input state
		std::vector<VkVertexInputBindingDescription> inputBindings = {
			vks::initializers::vertexInputBindingDescription(0, sizeof(Particle), VK_VERTEX_INPUT_RATE_VERTEX)
		};
		std::vector<VkVertexInputAttributeDescription> inputAttributes = {
			// Location 0 : Position
			vks::initializers::vertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Particle, pos)),
			// Location 1 : Velocity (used for color gradient lookup)
			vks::initializers::vertexInputAttributeDescription(0, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Particle, vel)),
		};
		VkPipelineVertexInputStateCreateInfo vertexInputState = vks::initializers::pipelineVertexInputStateCreateInfo();
		vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(inputBindings.size());
		vertexInputState.pVertexBindingDescriptions = inputBindings.data();
		vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(inputAttributes.size());
		vertexInputState.pVertexAttributeDescriptions = inputAttributes.data();

		// Shaders
		shaderStages[0] = loadShader(getShadersPath() + "computenbody/particle.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getShadersPath() + "computenbody/particle.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

		VkGraphicsPipelineCreateInfo pipelineCreateInfo = vks::initializers::pipelineCreateInfo(graphics.pipelineLayout, renderPass, 0);
		pipelineCreateInfo.pVertexInputState = &vertexInputState;
		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
		pipelineCreateInfo.pRasterizationState = &rasterizationState;
		pipelineCreateInfo.pColorBlendState = &colorBlendState;
		pipelineCreateInfo.pMultisampleState = &multisampleState;
		pipelineCreateInfo.pViewportState = &viewportState;
		pipelineCreateInfo.pDepthStencilState = &depthStencilState;
		pipelineCreateInfo.pDynamicState = &dynamicState;
		pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineCreateInfo.pStages = shaderStages.data();
		pipelineCreateInfo.renderPass = renderPass;

		// Additive blending
		blendAttachmentState.colorWriteMask = 0xF;
		blendAttachmentState.blendEnable = VK_TRUE;
		blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
		blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
		blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
		blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;

		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &graphics.pipeline));
	}

	void prepareCompute()
	{
		// Create a compute capable device queue
		// The VulkanDevice::createLogicalDevice functions finds a compute capable queue and prefers queue families that only support compute
		// Depending on the implementation this may result in different queue family indices for graphics and computes,
		// requiring proper synchronization (see the memory barriers in buildComputeCommandBuffer)
		vkGetDeviceQueue(device, compute.queueFamilyIndex, 0, &compute.queue);

		// Compute shader uniform buffer block
		for (auto& buffer : compute.uniformBuffers) {
			vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &buffer, sizeof(Compute::UniformData));
			VK_CHECK_RESULT(buffer.map());
		}

		// Create compute pipeline
		// Compute pipelines are created separate from graphics pipelines even if they use the same queue (family index)

		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
			// Binding 0 : Particle position storage buffer
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 0),
			// Binding 1 : Uniform buffer
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 1),
		};
		VkDescriptorSetLayoutCreateInfo descriptorLayout = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &compute.descriptorSetLayout));

		for (auto i = 0; i < compute.uniformBuffers.size(); i++) {
			VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &compute.descriptorSetLayout, 1);
			VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &compute.descriptorSets[i]));
			std::vector<VkWriteDescriptorSet> computeWriteDescriptorSets = {
				// Binding 0 : Particle position storage buffer
				vks::initializers::writeDescriptorSet(compute.descriptorSets[i], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, &storageBuffer.descriptor),
				// Binding 1 : Uniform buffer
				vks::initializers::writeDescriptorSet(compute.descriptorSets[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, &compute.uniformBuffers[i].descriptor)
			};
			vkUpdateDescriptorSets(device, static_cast<uint32_t>(computeWriteDescriptorSets.size()), computeWriteDescriptorSets.data(), 0, nullptr);
		}

		// Create pipelines
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(&compute.descriptorSetLayout, 1);
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &compute.pipelineLayout));

		VkComputePipelineCreateInfo computePipelineCreateInfo = vks::initializers::computePipelineCreateInfo(compute.pipelineLayout, 0);

		// 1st pass
		computePipelineCreateInfo.stage = loadShader(getShadersPath() + "computenbody/particle_calculate.comp.spv", VK_SHADER_STAGE_COMPUTE_BIT);

		// We want to use as much shared memory for the compute shader invocations as available, so we calculate it based on the device limits and pass it to the shader via specialization constants
		uint32_t sharedDataSize = std::min((uint32_t)1024, (uint32_t)(vulkanDevice->properties.limits.maxComputeSharedMemorySize / sizeof(glm::vec4)));
		VkSpecializationMapEntry specializationMapEntry = vks::initializers::specializationMapEntry(0, 0, sizeof(uint32_t));
		VkSpecializationInfo specializationInfo = vks::initializers::specializationInfo(1, &specializationMapEntry, sizeof(int32_t), &sharedDataSize);
		computePipelineCreateInfo.stage.pSpecializationInfo = &specializationInfo;

		VK_CHECK_RESULT(vkCreateComputePipelines(device, pipelineCache, 1, &computePipelineCreateInfo, nullptr, &compute.pipelineCalculate));

		// 2nd pass
		computePipelineCreateInfo.stage = loadShader(getShadersPath() + "computenbody/particle_integrate.comp.spv", VK_SHADER_STAGE_COMPUTE_BIT);
		VK_CHECK_RESULT(vkCreateComputePipelines(device, pipelineCache, 1, &computePipelineCreateInfo, nullptr, &compute.pipelineIntegrate));

		// Separate command pool as queue family for compute may be different than graphics
		VkCommandPoolCreateInfo cmdPoolInfo = vks::initializers::commandPoolCreateInfo();
		cmdPoolInfo.queueFamilyIndex = compute.queueFamilyIndex;
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
			vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphore.ready);
			vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphore.complete);
		}
		// Signal first used ready semaphore
		VkSubmitInfo computeSubmitInfo = vks::initializers::submitInfo();
		computeSubmitInfo.signalSemaphoreCount = 1;
		computeSubmitInfo.pSignalSemaphores = &compute.semaphores[maxConcurrentFrames - 1].ready;
		VK_CHECK_RESULT(vkQueueSubmit(compute.queue, 1, &computeSubmitInfo, VK_NULL_HANDLE));
	}

	void updateComputeUniformBuffers()
	{
		compute.uniformData.deltaT = paused ? 0.0f : frameTimer * 0.05f;
		memcpy(compute.uniformBuffers[currentBuffer].mapped, &compute.uniformData, sizeof(Compute::UniformData));
	}

	void updateGraphicsUniformBuffers()
	{
		graphics.uniformData.projection = camera.matrices.perspective;
		graphics.uniformData.view = camera.matrices.view;
		graphics.uniformData.screenDim = glm::vec2((float)width, (float)height);
		memcpy(graphics.uniformBuffers[currentBuffer].mapped, &graphics.uniformData, sizeof(Graphics::UniformData));
	}

	void prepare()
	{
		VulkanExampleBase::prepare();
		// We will be using the queue family indices to check if graphics and compute queue families differ
		// If that's the case, we need additional barriers for acquiring and releasing resources
		graphics.queueFamilyIndex = vulkanDevice->queueFamilyIndices.graphics;
		compute.queueFamilyIndex = vulkanDevice->queueFamilyIndices.compute;
		loadAssets();
		prepareDescriptorPool();
		prepareStorageBuffers();
		prepareGraphics();
		prepareCompute();
		prepared = true;
	}

	void buildGraphicsCommandBuffer()
	{
		VkCommandBuffer cmdBuffer = drawCmdBuffers[currentBuffer];
		
		VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

		VkClearValue clearValues[2]{};
		clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
		renderPassBeginInfo.renderPass = renderPass;
		renderPassBeginInfo.renderArea.offset.x = 0;
		renderPassBeginInfo.renderArea.offset.y = 0;
		renderPassBeginInfo.renderArea.extent.width = width;
		renderPassBeginInfo.renderArea.extent.height = height;
		renderPassBeginInfo.clearValueCount = 2;
		renderPassBeginInfo.pClearValues = clearValues;
		renderPassBeginInfo.framebuffer = frameBuffers[currentImageIndex];

		VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));

		// Acquire barrier
		if (graphics.queueFamilyIndex != compute.queueFamilyIndex)
		{
			VkBufferMemoryBarrier buffer_barrier =
			{
				VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
				nullptr,
				0,
				VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
				compute.queueFamilyIndex,
				graphics.queueFamilyIndex,
				storageBuffer.buffer,
				0,
				storageBuffer.size
			};

			vkCmdPipelineBarrier(
				cmdBuffer,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
				0,
				0, nullptr,
				1, &buffer_barrier,
				0, nullptr);
		}

		// Draw the particle system using the update vertex buffer
		vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
		vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

		VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);
		vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics.pipeline);
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics.pipelineLayout, 0, 1, &graphics.descriptorSets[currentBuffer], 0, nullptr);

		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &storageBuffer.buffer, offsets);
		vkCmdDraw(cmdBuffer, numParticles, 1, 0, 0);

		drawUI(cmdBuffer);

		vkCmdEndRenderPass(cmdBuffer);

		// Release barrier
		if (graphics.queueFamilyIndex != compute.queueFamilyIndex)
		{
			VkBufferMemoryBarrier buffer_barrier =
			{
				VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
				nullptr,
				VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
				0,
				graphics.queueFamilyIndex,
				compute.queueFamilyIndex,
				storageBuffer.buffer,
				0,
				storageBuffer.size
			};

			vkCmdPipelineBarrier(
				cmdBuffer,
				VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
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
		if (graphics.queueFamilyIndex != compute.queueFamilyIndex)
		{
			VkBufferMemoryBarrier buffer_barrier =
			{
				VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
				nullptr,
				0,
				VK_ACCESS_SHADER_WRITE_BIT,
				graphics.queueFamilyIndex,
				compute.queueFamilyIndex,
				storageBuffer.buffer,
				0,
				storageBuffer.size
			};

			vkCmdPipelineBarrier(
				cmdBuffer,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				0,
				0, nullptr,
				1, &buffer_barrier,
				0, nullptr);
		}

		// First pass: Calculate particle movement
		// -------------------------------------------------------------------------------------------------------
		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute.pipelineCalculate);
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute.pipelineLayout, 0, 1, &compute.descriptorSets[currentBuffer], 0, nullptr);
		vkCmdDispatch(cmdBuffer, numParticles / 256, 1, 1);

		// Add memory barrier to ensure that the computer shader has finished writing to the buffer
		VkBufferMemoryBarrier bufferBarrier = vks::initializers::bufferMemoryBarrier();
		bufferBarrier.buffer = storageBuffer.buffer;
		bufferBarrier.size = storageBuffer.descriptor.range;
		bufferBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		bufferBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		// Transfer ownership if compute and graphics queue family indices differ
		bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		vkCmdPipelineBarrier(
			cmdBuffer,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_FLAGS_NONE,
			0, nullptr,
			1, &bufferBarrier,
			0, nullptr);

		// Second pass: Integrate particles
		// -------------------------------------------------------------------------------------------------------
		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute.pipelineIntegrate);
		vkCmdDispatch(cmdBuffer, numParticles / 256, 1, 1);

		// Release barrier
		if (graphics.queueFamilyIndex != compute.queueFamilyIndex)
		{
			VkBufferMemoryBarrier buffer_barrier =
			{
				VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
				nullptr,
				VK_ACCESS_SHADER_WRITE_BIT,
				0,
				compute.queueFamilyIndex,
				graphics.queueFamilyIndex,
				storageBuffer.buffer,
				0,
				storageBuffer.size
			};

			vkCmdPipelineBarrier(
				cmdBuffer,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
				0,
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

			updateComputeUniformBuffers();
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

			updateGraphicsUniformBuffers();
			buildGraphicsCommandBuffer();

			VkPipelineStageFlags waitDstStageMask[2] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT };
			VkSemaphore waitSemaphores[2] = { presentCompleteSemaphores[currentBuffer], compute.semaphores[currentBuffer].complete };
			VkSemaphore signalSemaphores[2] = { renderCompleteSemaphores[currentImageIndex], compute.semaphores[currentBuffer].ready };

			VkSubmitInfo submitInfo = vks::initializers::submitInfo();
			submitInfo.waitSemaphoreCount = 2;
			submitInfo.pWaitSemaphores = waitSemaphores;
			submitInfo.pWaitDstStageMask = waitDstStageMask;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];
			submitInfo.signalSemaphoreCount = 2;
			submitInfo.pSignalSemaphores = signalSemaphores;
			VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, waitFences[currentBuffer]));

			VulkanExampleBase::submitFrame(true);
		}
	}
};

VULKAN_EXAMPLE_MAIN()
