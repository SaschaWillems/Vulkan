/*
* Vulkan Example - Attraction based compute shader particle system
*
* Updated compute shader by Lukas Bergdoll (https://github.com/Voultapher)
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "vulkanexamplebase.h"

#define VERTEX_BUFFER_BIND_ID 0
#define ENABLE_VALIDATION false
#if defined(__ANDROID__)
// Lower particle count on Android for performance reasons
#define PARTICLE_COUNT 128 * 1024
#else
#define PARTICLE_COUNT 256 * 1024
#endif

class VulkanExample : public VulkanExampleBase
{
public:
	float timer = 0.0f;
	float animStart = 20.0f;
	bool attachToCursor = false;

	struct {
		vks::Texture2D particle;
		vks::Texture2D gradient;
	} textures;

	struct {
		VkPipelineVertexInputStateCreateInfo inputState;
		std::vector<VkVertexInputBindingDescription> bindingDescriptions;
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
	} vertices;

	// Resources for the graphics part of the example
	struct {
		uint32_t queueFamilyIndex;					// Used to check if compute and graphics queue families differ and require additional barriers
		VkDescriptorSetLayout descriptorSetLayout;	// Particle system rendering shader binding layout
		VkDescriptorSet descriptorSet;				// Particle system rendering shader bindings
		VkPipelineLayout pipelineLayout;			// Layout of the graphics pipeline
		VkPipeline pipeline;						// Particle rendering pipeline
		VkSemaphore semaphore;                      // Execution dependency between compute & graphic submission
	} graphics;

	// Resources for the compute part of the example
	struct {
		uint32_t queueFamilyIndex;					// Used to check if compute and graphics queue families differ and require additional barriers
		vks::Buffer storageBuffer;					// (Shader) storage buffer object containing the particles
		vks::Buffer uniformBuffer;					// Uniform buffer object containing particle system parameters
		VkQueue queue;								// Separate queue for compute commands (queue family may differ from the one used for graphics)
		VkCommandPool commandPool;					// Use a separate command pool (queue family may differ from the one used for graphics)
		VkCommandBuffer commandBuffer;				// Command buffer storing the dispatch commands and barriers
		VkSemaphore semaphore;                      // Execution dependency between compute & graphic submission
		VkDescriptorSetLayout descriptorSetLayout;	// Compute shader binding layout
		VkDescriptorSet descriptorSet;				// Compute shader bindings
		VkPipelineLayout pipelineLayout;			// Layout of the compute pipeline
		VkPipeline pipeline;						// Compute pipeline for updating particle positions
		struct computeUBO {							// Compute shader uniform block object
			float deltaT;							//		Frame delta time
			float destX;							//		x position of the attractor
			float destY;							//		y position of the attractor
			int32_t particleCount = PARTICLE_COUNT;
		} ubo;
	} compute;

	// SSBO particle declaration
	struct Particle {
		glm::vec2 pos;								// Particle position
		glm::vec2 vel;								// Particle velocity
		glm::vec4 gradientPos;						// Texture coordinates for the gradient ramp map
	};

	VulkanExample() : VulkanExampleBase(ENABLE_VALIDATION)
	{
		title = "Compute shader particle system";
	}

	~VulkanExample()
	{
		// Graphics
		vkDestroyPipeline(device, graphics.pipeline, nullptr);
		vkDestroyPipelineLayout(device, graphics.pipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, graphics.descriptorSetLayout, nullptr);

		// Compute
		compute.storageBuffer.destroy();
		compute.uniformBuffer.destroy();
		vkDestroyPipelineLayout(device, compute.pipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, compute.descriptorSetLayout, nullptr);
		vkDestroyPipeline(device, compute.pipeline, nullptr);
		vkDestroySemaphore(device, compute.semaphore, nullptr);
		vkDestroyCommandPool(device, compute.commandPool, nullptr);

		textures.particle.destroy();
		textures.gradient.destroy();
	}

	void loadAssets()
	{
		textures.particle.loadFromFile(getAssetPath() + "textures/particle01_rgba.ktx", VK_FORMAT_R8G8B8A8_UNORM, vulkanDevice, queue);
		textures.gradient.loadFromFile(getAssetPath() + "textures/particle_gradient_rgba.ktx", VK_FORMAT_R8G8B8A8_UNORM, vulkanDevice, queue);
	}

	void buildCommandBuffers()
	{
		VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

		VkClearValue clearValues[2];
		clearValues[0].color = defaultClearColor;
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
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
					compute.storageBuffer.buffer,
					0,
					compute.storageBuffer.size
				};

				vkCmdPipelineBarrier(
					drawCmdBuffers[i],
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
					0,
					0, nullptr,
					1, &buffer_barrier,
					0, nullptr);
			}

			// Draw the particle system using the update vertex buffer
			vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
			vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

			VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);
			vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

			vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics.pipeline);
			vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics.pipelineLayout, 0, 1, &graphics.descriptorSet, 0, NULL);

			glm::vec2 screendim = glm::vec2((float)width, (float)height);
			vkCmdPushConstants(
					drawCmdBuffers[i],
					graphics.pipelineLayout,
					VK_SHADER_STAGE_VERTEX_BIT,
					0,
					sizeof(glm::vec2),
					&screendim);

			VkDeviceSize offsets[1] = { 0 };
			vkCmdBindVertexBuffers(drawCmdBuffers[i], VERTEX_BUFFER_BIND_ID, 1, &compute.storageBuffer.buffer, offsets);
			vkCmdDraw(drawCmdBuffers[i], PARTICLE_COUNT, 1, 0, 0);

			drawUI(drawCmdBuffers[i]);

			vkCmdEndRenderPass(drawCmdBuffers[i]);

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
					compute.storageBuffer.buffer,
					0,
					compute.storageBuffer.size
				};

				vkCmdPipelineBarrier(
					drawCmdBuffers[i],
					VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					0,
					0, nullptr,
					1, &buffer_barrier,
					0, nullptr);
			}

			VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
		}

	}

	void buildComputeCommandBuffer()
	{
		VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

		VK_CHECK_RESULT(vkBeginCommandBuffer(compute.commandBuffer, &cmdBufInfo));

		// Compute particle movement

		// Add memory barrier to ensure that the (graphics) vertex shader has fetched attributes before compute starts to write to the buffer
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
				compute.storageBuffer.buffer,
				0,
				compute.storageBuffer.size
			};

			vkCmdPipelineBarrier(
				compute.commandBuffer,
				VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				0,
				0, nullptr,
				1, &buffer_barrier,
				0, nullptr);
		}

		// Dispatch the compute job
		vkCmdBindPipeline(compute.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute.pipeline);
		vkCmdBindDescriptorSets(compute.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute.pipelineLayout, 0, 1, &compute.descriptorSet, 0, 0);
		vkCmdDispatch(compute.commandBuffer, PARTICLE_COUNT / 256, 1, 1);

		// Add barrier to ensure that compute shader has finished writing to the buffer
		// Without this the (rendering) vertex shader may display incomplete results (partial data from last frame)
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
				compute.storageBuffer.buffer,
				0,
				compute.storageBuffer.size
			};

			vkCmdPipelineBarrier(
				compute.commandBuffer,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
				0,
				0, nullptr,
				1, &buffer_barrier,
				0, nullptr);
		}

		vkEndCommandBuffer(compute.commandBuffer);
	}

	// Setup and fill the compute shader storage buffers containing the particles
	void prepareStorageBuffers()
	{
		std::default_random_engine rndEngine(benchmark.active ? 0 : (unsigned)time(nullptr));
		std::uniform_real_distribution<float> rndDist(-1.0f, 1.0f);

		// Initial particle positions
		std::vector<Particle> particleBuffer(PARTICLE_COUNT);
		for (auto& particle : particleBuffer) {
			particle.pos = glm::vec2(rndDist(rndEngine), rndDist(rndEngine));
			particle.vel = glm::vec2(0.0f);
			particle.gradientPos.x = particle.pos.x / 2.0f;
		}

		VkDeviceSize storageBufferSize = particleBuffer.size() * sizeof(Particle);

		// Staging
		// SSBO won't be changed on the host after upload so copy to device local memory

		vks::Buffer stagingBuffer;

		vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&stagingBuffer,
			storageBufferSize,
			particleBuffer.data());

		vulkanDevice->createBuffer(
			// The SSBO will be used as a storage buffer for the compute pipeline and as a vertex buffer in the graphics pipeline
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			&compute.storageBuffer,
			storageBufferSize);

		// Copy from staging buffer to storage buffer
		VkCommandBuffer copyCmd = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
		VkBufferCopy copyRegion = {};
		copyRegion.size = storageBufferSize;
		vkCmdCopyBuffer(copyCmd, stagingBuffer.buffer, compute.storageBuffer.buffer, 1, &copyRegion);
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
				compute.storageBuffer.buffer,
				0,
				compute.storageBuffer.size
			};

			vkCmdPipelineBarrier(
				copyCmd,
				VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				0,
				0, nullptr,
				1, &buffer_barrier,
				0, nullptr);
		}
		vulkanDevice->flushCommandBuffer(copyCmd, queue, true);

		stagingBuffer.destroy();

		// Binding description
		vertices.bindingDescriptions.resize(1);
		vertices.bindingDescriptions[0] =
			vks::initializers::vertexInputBindingDescription(
				VERTEX_BUFFER_BIND_ID,
				sizeof(Particle),
				VK_VERTEX_INPUT_RATE_VERTEX);

		// Attribute descriptions
		// Describes memory layout and shader positions
		vertices.attributeDescriptions.resize(2);
		// Location 0 : Position
		vertices.attributeDescriptions[0] =
			vks::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				0,
				VK_FORMAT_R32G32_SFLOAT,
				offsetof(Particle, pos));
		// Location 1 : Gradient position
		vertices.attributeDescriptions[1] =
			vks::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				1,
				VK_FORMAT_R32G32B32A32_SFLOAT,
				offsetof(Particle, gradientPos));

		// Assign to vertex buffer
		vertices.inputState = vks::initializers::pipelineVertexInputStateCreateInfo();
		vertices.inputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertices.bindingDescriptions.size());
		vertices.inputState.pVertexBindingDescriptions = vertices.bindingDescriptions.data();
		vertices.inputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertices.attributeDescriptions.size());
		vertices.inputState.pVertexAttributeDescriptions = vertices.attributeDescriptions.data();
	}

	void setupDescriptorPool()
	{
		std::vector<VkDescriptorPoolSize> poolSizes =
		{
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1),
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1),
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2)
		};

		VkDescriptorPoolCreateInfo descriptorPoolInfo =
			vks::initializers::descriptorPoolCreateInfo(
				static_cast<uint32_t>(poolSizes.size()),
				poolSizes.data(),
				2);

		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));
	}

	void setupDescriptorSetLayout()
	{
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings;
		// Binding 0 : Particle color map
		setLayoutBindings.push_back(vks::initializers::descriptorSetLayoutBinding(
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			0));
		// Binding 1 : Particle gradient ramp
		setLayoutBindings.push_back(vks::initializers::descriptorSetLayoutBinding(
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			1));

		VkDescriptorSetLayoutCreateInfo descriptorLayout =
			vks::initializers::descriptorSetLayoutCreateInfo(
				setLayoutBindings.data(),
				static_cast<uint32_t>(setLayoutBindings.size()));

		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &graphics.descriptorSetLayout));

		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo =
			vks::initializers::pipelineLayoutCreateInfo(
				&graphics.descriptorSetLayout,
				1);

		VkPushConstantRange pushConstantRange =
			vks::initializers::pushConstantRange(
				VK_SHADER_STAGE_VERTEX_BIT,
				sizeof(glm::vec2),
				0);

		pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
		pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &graphics.pipelineLayout));
	}

	void setupDescriptorSet()
	{
		VkDescriptorSetAllocateInfo allocInfo =
			vks::initializers::descriptorSetAllocateInfo(
				descriptorPool,
				&graphics.descriptorSetLayout,
				1);

		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &graphics.descriptorSet));

		std::vector<VkWriteDescriptorSet> writeDescriptorSets;
		// Binding 0 : Particle color map
		writeDescriptorSets.push_back(vks::initializers::writeDescriptorSet(
			graphics.descriptorSet,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			0,
			&textures.particle.descriptor));
		// Binding 1 : Particle gradient ramp
		writeDescriptorSets.push_back(vks::initializers::writeDescriptorSet(
			graphics.descriptorSet,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			1,
			&textures.gradient.descriptor));

		vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, NULL);
	}

	void preparePipelines()
	{
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
			vks::initializers::pipelineInputAssemblyStateCreateInfo(
				VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
				0,
				VK_FALSE);

		VkPipelineRasterizationStateCreateInfo rasterizationState =
			vks::initializers::pipelineRasterizationStateCreateInfo(
				VK_POLYGON_MODE_FILL,
				VK_CULL_MODE_NONE,
				VK_FRONT_FACE_COUNTER_CLOCKWISE,
				0);

		VkPipelineColorBlendAttachmentState blendAttachmentState =
			vks::initializers::pipelineColorBlendAttachmentState(
				0xf,
				VK_FALSE);

		VkPipelineColorBlendStateCreateInfo colorBlendState =
			vks::initializers::pipelineColorBlendStateCreateInfo(
				1,
				&blendAttachmentState);

		VkPipelineDepthStencilStateCreateInfo depthStencilState =
			vks::initializers::pipelineDepthStencilStateCreateInfo(
				VK_FALSE,
				VK_FALSE,
				VK_COMPARE_OP_ALWAYS);

		VkPipelineViewportStateCreateInfo viewportState =
			vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);

		VkPipelineMultisampleStateCreateInfo multisampleState =
			vks::initializers::pipelineMultisampleStateCreateInfo(
				VK_SAMPLE_COUNT_1_BIT,
				0);

		std::vector<VkDynamicState> dynamicStateEnables = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};
		VkPipelineDynamicStateCreateInfo dynamicState =
			vks::initializers::pipelineDynamicStateCreateInfo(
				dynamicStateEnables.data(),
				static_cast<uint32_t>(dynamicStateEnables.size()),
				0);

		// Rendering pipeline
		// Load shaders
		std::array<VkPipelineShaderStageCreateInfo,2> shaderStages;

		shaderStages[0] = loadShader(getShadersPath() + "computeparticles/particle.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getShadersPath() + "computeparticles/particle.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

		VkGraphicsPipelineCreateInfo pipelineCreateInfo =
			vks::initializers::pipelineCreateInfo(
				graphics.pipelineLayout,
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

	void prepareGraphics()
	{
		prepareStorageBuffers();
		prepareUniformBuffers();
		setupDescriptorSetLayout();
		preparePipelines();
		setupDescriptorSet();

		// Semaphore for compute & graphics sync
		VkSemaphoreCreateInfo semaphoreCreateInfo = vks::initializers::semaphoreCreateInfo();
		VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &graphics.semaphore));
	}

	void prepareCompute()
	{
		// Create a compute capable device queue
		// The VulkanDevice::createLogicalDevice functions finds a compute capable queue and prefers queue families that only support compute
		// Depending on the implementation this may result in different queue family indices for graphics and computes,
		// requiring proper synchronization (see the memory and pipeline barriers)
		vkGetDeviceQueue(device, compute.queueFamilyIndex, 0, &compute.queue);

		// Create compute pipeline
		// Compute pipelines are created separate from graphics pipelines even if they use the same queue (family index)

		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
			// Binding 0 : Particle position storage buffer
			vks::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				VK_SHADER_STAGE_COMPUTE_BIT,
				0),
			// Binding 1 : Uniform buffer
			vks::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				VK_SHADER_STAGE_COMPUTE_BIT,
				1),
		};

		VkDescriptorSetLayoutCreateInfo descriptorLayout =
			vks::initializers::descriptorSetLayoutCreateInfo(
				setLayoutBindings.data(),
				static_cast<uint32_t>(setLayoutBindings.size()));

		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device,	&descriptorLayout, nullptr,	&compute.descriptorSetLayout));

		VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo =
			vks::initializers::pipelineLayoutCreateInfo(
				&compute.descriptorSetLayout,
				1);

		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr,	&compute.pipelineLayout));

		VkDescriptorSetAllocateInfo allocInfo =
			vks::initializers::descriptorSetAllocateInfo(
				descriptorPool,
				&compute.descriptorSetLayout,
				1);

		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &compute.descriptorSet));

		std::vector<VkWriteDescriptorSet> computeWriteDescriptorSets =
		{
			// Binding 0 : Particle position storage buffer
			vks::initializers::writeDescriptorSet(
				compute.descriptorSet,
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				0,
				&compute.storageBuffer.descriptor),
			// Binding 1 : Uniform buffer
			vks::initializers::writeDescriptorSet(
				compute.descriptorSet,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				1,
				&compute.uniformBuffer.descriptor)
		};

		vkUpdateDescriptorSets(device, static_cast<uint32_t>(computeWriteDescriptorSets.size()), computeWriteDescriptorSets.data(), 0, NULL);

		// Create pipeline
		VkComputePipelineCreateInfo computePipelineCreateInfo = vks::initializers::computePipelineCreateInfo(compute.pipelineLayout, 0);
		computePipelineCreateInfo.stage = loadShader(getShadersPath() + "computeparticles/particle.comp.spv", VK_SHADER_STAGE_COMPUTE_BIT);
		VK_CHECK_RESULT(vkCreateComputePipelines(device, pipelineCache, 1, &computePipelineCreateInfo, nullptr, &compute.pipeline));

		// Separate command pool as queue family for compute may be different than graphics
		VkCommandPoolCreateInfo cmdPoolInfo = {};
		cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolInfo.queueFamilyIndex = compute.queueFamilyIndex;
		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		VK_CHECK_RESULT(vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &compute.commandPool));

		// Create a command buffer for compute operations
		compute.commandBuffer = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, compute.commandPool);

		// Semaphore for compute & graphics sync
		VkSemaphoreCreateInfo semaphoreCreateInfo = vks::initializers::semaphoreCreateInfo();
		VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &compute.semaphore));

		// Signal the semaphore
		VkSubmitInfo submitInfo = vks::initializers::submitInfo();
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &compute.semaphore;
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
		VK_CHECK_RESULT(vkQueueWaitIdle(queue));

		// Build a single command buffer containing the compute dispatch commands
		buildComputeCommandBuffer();

		// If graphics and compute queue family indices differ, acquire and immediately release the storage buffer, so that the initial acquire from the graphics command buffers are matched up properly
		if (graphics.queueFamilyIndex != compute.queueFamilyIndex)
		{
			// Create a transient command buffer for setting up the initial buffer transfer state
			VkCommandBuffer transferCmd = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, compute.commandPool, true);

			VkBufferMemoryBarrier acquire_buffer_barrier =
			{
				VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
				nullptr,
				0,
				VK_ACCESS_SHADER_WRITE_BIT,
				graphics.queueFamilyIndex,
				compute.queueFamilyIndex,
				compute.storageBuffer.buffer,
				0,
				compute.storageBuffer.size
			};
			vkCmdPipelineBarrier(
				transferCmd,
				VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				0,
				0, nullptr,
				1, &acquire_buffer_barrier,
				0, nullptr);

			VkBufferMemoryBarrier release_buffer_barrier =
			{
				VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
				nullptr,
				VK_ACCESS_SHADER_WRITE_BIT,
				0,
				compute.queueFamilyIndex,
				graphics.queueFamilyIndex,
				compute.storageBuffer.buffer,
				0,
				compute.storageBuffer.size
			};
			vkCmdPipelineBarrier(
				transferCmd,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
				0,
				0, nullptr,
				1, &release_buffer_barrier,
				0, nullptr);

			vulkanDevice->flushCommandBuffer(transferCmd, compute.queue, compute.commandPool);
		}
	}

	// Prepare and initialize uniform buffer containing shader uniforms
	void prepareUniformBuffers()
	{
		// Compute shader uniform buffer block
		vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&compute.uniformBuffer,
			sizeof(compute.ubo));

		// Map for host access
		VK_CHECK_RESULT(compute.uniformBuffer.map());

		updateUniformBuffers();
	}

	void updateUniformBuffers()
	{
		compute.ubo.deltaT = frameTimer * 2.5f;
		if (!attachToCursor)
		{
			compute.ubo.destX = sin(glm::radians(timer * 360.0f)) * 0.75f;
			compute.ubo.destY = 0.0f;
		}
		else
		{
			float normalizedMx = (mousePos.x - static_cast<float>(width / 2)) / static_cast<float>(width / 2);
			float normalizedMy = (mousePos.y - static_cast<float>(height / 2)) / static_cast<float>(height / 2);
			compute.ubo.destX = normalizedMx;
			compute.ubo.destY = normalizedMy;
		}

		memcpy(compute.uniformBuffer.mapped, &compute.ubo, sizeof(compute.ubo));
	}

	void draw()
	{
		VulkanExampleBase::prepareFrame();

		VkPipelineStageFlags graphicsWaitStageMasks[] = { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		VkSemaphore graphicsWaitSemaphores[] = { compute.semaphore, semaphores.presentComplete };
		VkSemaphore graphicsSignalSemaphores[] = { graphics.semaphore, semaphores.renderComplete };

		// Submit graphics commands
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];
		submitInfo.waitSemaphoreCount = 2;
		submitInfo.pWaitSemaphores = graphicsWaitSemaphores;
		submitInfo.pWaitDstStageMask = graphicsWaitStageMasks;
		submitInfo.signalSemaphoreCount = 2;
		submitInfo.pSignalSemaphores = graphicsSignalSemaphores;
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

		VulkanExampleBase::submitFrame();

		// Wait for rendering finished
		VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

		// Submit compute commands
		VkSubmitInfo computeSubmitInfo = vks::initializers::submitInfo();
		computeSubmitInfo.commandBufferCount = 1;
		computeSubmitInfo.pCommandBuffers = &compute.commandBuffer;
		computeSubmitInfo.waitSemaphoreCount = 1;
		computeSubmitInfo.pWaitSemaphores = &graphics.semaphore;
		computeSubmitInfo.pWaitDstStageMask = &waitStageMask;
		computeSubmitInfo.signalSemaphoreCount = 1;
		computeSubmitInfo.pSignalSemaphores = &compute.semaphore;
		VK_CHECK_RESULT(vkQueueSubmit(compute.queue, 1, &computeSubmitInfo, VK_NULL_HANDLE));

	}

	void prepare()
	{
		VulkanExampleBase::prepare();
		// We will be using the queue family indices to check if graphics and compute queue families differ
		// If that's the case, we need additional barriers for acquiring and releasing resources
		graphics.queueFamilyIndex = vulkanDevice->queueFamilyIndices.graphics;
		compute.queueFamilyIndex = vulkanDevice->queueFamilyIndices.compute;
		loadAssets();
		setupDescriptorPool();
		prepareGraphics();
		prepareCompute();
		buildCommandBuffers();
		prepared = true;
	}

	virtual void render()
	{
		if (!prepared)
			return;
		draw();

		if (!attachToCursor)
		{
			if (animStart > 0.0f)
			{
				animStart -= frameTimer * 5.0f;
			}
			else if (animStart <= 0.0f)
			{
				timer += frameTimer * 0.04f;
				if (timer > 1.f)
					timer = 0.f;
			}
		}

		updateUniformBuffers();
	}

	virtual void OnUpdateUIOverlay(vks::UIOverlay *overlay)
	{
		if (overlay->header("Settings")) {
			overlay->checkBox("Attach attractor to cursor", &attachToCursor);
		}
	}
};

VULKAN_EXAMPLE_MAIN()
