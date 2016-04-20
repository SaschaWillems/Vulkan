/*
* Vulkan Example - Attraction based compute shader particle system
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
#define PARTICLE_COUNT 8 * 1024

class VulkanExample : public VulkanExampleBase
{
private:
	vkTools::VulkanTexture textureColorMap;
public:
	float timer = 0.0f;
	float animStart = 50.0f;
	bool animate = true;

	struct {
		VkPipelineVertexInputStateCreateInfo inputState;
		std::vector<VkVertexInputBindingDescription> bindingDescriptions;
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
	} vertices;

	struct {
		VkPipeline postCompute;
		// Compute pipelines are separated from 
		// graphics pipelines in Vulkan
		VkPipeline compute;
	} pipelines;

	VkQueue computeQueue;
	VkCommandBuffer computeCmdBuffer;
	VkPipelineLayout computePipelineLayout;
	VkDescriptorSet computeDescriptorSet;
	VkDescriptorSetLayout computeDescriptorSetLayout;

	vkTools::UniformData computeStorageBuffer;

	struct {
		float deltaT;
		float destX;
		float destY;
		int32_t particleCount = PARTICLE_COUNT;
	} computeUbo;

	struct {
		struct {
			vkTools::UniformData ubo;
		} computeShader;
	} uniformData;

	struct Particle {
		glm::vec4 pos;
		glm::vec4 col;
		glm::vec4 vel;
	};

	VkPipelineLayout pipelineLayout;
	VkDescriptorSet descriptorSetPostCompute;
	VkDescriptorSetLayout descriptorSetLayout;

	VulkanExample() : VulkanExampleBase(ENABLE_VALIDATION)
	{
		width = 1280;
		height = 720;
		zoom = -2.0f;
		title = "Vulkan Example - Compute shader particle system";
	}

	~VulkanExample()
	{
		// Clean up used Vulkan resources 
		// Note : Inherited destructor cleans up resources stored in base class

		vkDestroyPipeline(device, pipelines.postCompute, nullptr);

		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
		vkDestroyBuffer(device, computeStorageBuffer.buffer, nullptr);
		vkFreeMemory(device, computeStorageBuffer.memory, nullptr);

		vkTools::destroyUniformData(device, &uniformData.computeShader.ubo);

		vkFreeCommandBuffers(device, cmdPool, 1, &computeCmdBuffer);
		vkDestroyPipelineLayout(device, computePipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, computeDescriptorSetLayout, nullptr);
		vkDestroyPipeline(device, pipelines.compute, nullptr);

		textureLoader->destroyTexture(textureColorMap);
	}

	void loadTextures()
	{
		textureLoader->loadTexture(
			getAssetPath() + "textures/particle01_rgba.ktx", 
			VK_FORMAT_R8G8B8A8_UNORM, 
			&textureColorMap, 
			false);
	}

	void buildCommandBuffers()
	{
		// Destroy command buffers if already present
		if (!checkCommandBuffers())
		{
			destroyCommandBuffers();
			createCommandBuffers();
		}

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

		VkResult err;

		for (int32_t i = 0; i < drawCmdBuffers.size(); ++i)
		{
			// Set target frame buffer
			renderPassBeginInfo.framebuffer = frameBuffers[i];

			err = vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo);
			assert(!err);

			// Buffer memory barrier to make sure that compute shader
			// writes are finished before using the storage buffer 
			// in the vertex shader
			VkBufferMemoryBarrier bufferBarrier = vkTools::initializers::bufferMemoryBarrier();
			// Source access : Compute shader buffer write
			bufferBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
			// Dest access : Vertex shader access (attribute binding)
			bufferBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
			bufferBarrier.buffer = computeStorageBuffer.buffer;
			bufferBarrier.offset = 0;
			bufferBarrier.size = computeStorageBuffer.descriptor.range;
			bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

			vkCmdPipelineBarrier(
				drawCmdBuffers[i],
				VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_FLAGS_NONE, 
				0, nullptr,
				1, &bufferBarrier,
				0, nullptr);

			vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport viewport = vkTools::initializers::viewport(
				(float)width,
				(float)height,
				0.0f,
				1.0f
				);
			vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

			VkRect2D scissor = vkTools::initializers::rect2D(
				width,
				height,
				0,
				0
				);
			vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

			vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSetPostCompute, 0, NULL);
			vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.postCompute);

			VkDeviceSize offsets[1] = { 0 };
			vkCmdBindVertexBuffers(drawCmdBuffers[i], VERTEX_BUFFER_BIND_ID, 1, &computeStorageBuffer.buffer, offsets);
			vkCmdDraw(drawCmdBuffers[i], PARTICLE_COUNT, 1, 0, 0);

			vkCmdEndRenderPass(drawCmdBuffers[i]);

			err = vkEndCommandBuffer(drawCmdBuffers[i]);
			assert(!err);
		}

	}

	void buildComputeCommandBuffer()
	{
		VkCommandBufferBeginInfo cmdBufInfo = vkTools::initializers::commandBufferBeginInfo();;

		vkBeginCommandBuffer(computeCmdBuffer, &cmdBufInfo);

		vkCmdBindPipeline(computeCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelines.compute);
		vkCmdBindDescriptorSets(computeCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 0, 1, &computeDescriptorSet, 0, 0);

		vkCmdDispatch(computeCmdBuffer, PARTICLE_COUNT / 16, 1, 1);

		vkEndCommandBuffer(computeCmdBuffer);
	}

	void draw()
	{
		VkResult err;

		// Get next image in the swap chain (back/front buffer)
		err = swapChain.acquireNextImage(semaphores.presentComplete, &currentBuffer);
		assert(!err);

		submitPostPresentBarrier(swapChain.buffers[currentBuffer].image);

		// Command buffer to be sumitted to the queue
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];

		// Submit to queue
		err = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
		assert(!err);

		submitPrePresentBarrier(swapChain.buffers[currentBuffer].image);

		err = swapChain.queuePresent(queue, currentBuffer, semaphores.renderComplete);
		assert(!err);

		err = vkQueueWaitIdle(queue);
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

	// Setup and fill the compute shader storage buffers for
	// vertex positions and velocities
	void prepareStorageBuffers()
	{
		float destPosX = 0.0f;
		float destPosY = 0.0f;

		// Initial particle positions
		std::vector<Particle> particleBuffer;
		for (int i = 0; i < PARTICLE_COUNT; ++i)
		{
			// Position
			float aspectRatio = (float)height / (float)width;
			float rndVal = (float)rand() / (float)(RAND_MAX / (360.0f * 3.14f * 2.0f));
			float rndRad = (float)rand() / (float)(RAND_MAX) * 0.5f;
			Particle p;
			p.pos = glm::vec4(
				destPosX + cos(rndVal) * rndRad * aspectRatio,
				destPosY + sin(rndVal) * rndRad,
				0.0f,
				1.0f);
			p.col = glm::vec4(
				(float)(rand() % 255) / 255.0f,
				(float)(rand() % 255) / 255.0f,
				(float)(rand() % 255) / 255.0f,
				1.0f);
			p.vel = glm::vec4(0.0f);
			particleBuffer.push_back(p);
		}

		// Buffer size is the same for all storage buffers
		uint32_t storageBufferSize = particleBuffer.size() * sizeof(Particle);

		VkMemoryAllocateInfo memAlloc = vkTools::initializers::memoryAllocateInfo();
		VkMemoryRequirements memReqs;

		VkResult err;
		void *data;

		struct StagingBuffer {
			VkDeviceMemory memory;
			VkBuffer buffer;
		} stagingBuffer;

		// Allocate and fill host-visible staging storage buffer object

		// Allocate and fill storage buffer object
		VkBufferCreateInfo vBufferInfo = 
			vkTools::initializers::bufferCreateInfo(
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
				storageBufferSize);
		vkTools::checkResult(vkCreateBuffer(device, &vBufferInfo, nullptr, &stagingBuffer.buffer));
		vkGetBufferMemoryRequirements(device, stagingBuffer.buffer, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memAlloc.memoryTypeIndex);
		vkTools::checkResult(vkAllocateMemory(device, &memAlloc, nullptr, &stagingBuffer.memory));
		vkTools::checkResult(vkMapMemory(device, stagingBuffer.memory, 0, storageBufferSize, 0, &data));
		memcpy(data, particleBuffer.data(), storageBufferSize);
		vkUnmapMemory(device, stagingBuffer.memory);
		vkTools::checkResult(vkBindBufferMemory(device, stagingBuffer.buffer, stagingBuffer.memory, 0));

		// Allocate device local storage buffer ojbect
		vBufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		vkTools::checkResult(vkCreateBuffer(device, &vBufferInfo, nullptr, &computeStorageBuffer.buffer));
		vkGetBufferMemoryRequirements(device, computeStorageBuffer.buffer, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memAlloc.memoryTypeIndex);
		vkTools::checkResult(vkAllocateMemory(device, &memAlloc, nullptr, &computeStorageBuffer.memory));
		vkTools::checkResult(vkBindBufferMemory(device, computeStorageBuffer.buffer, computeStorageBuffer.memory, 0));

		// Copy from host to device
		createSetupCommandBuffer();

		VkBufferCopy copyRegion = {};
		copyRegion.size = storageBufferSize;
		vkCmdCopyBuffer(
			setupCmdBuffer,
			stagingBuffer.buffer,
			computeStorageBuffer.buffer,
			1,
			&copyRegion);

		flushSetupCommandBuffer();

		// Destroy staging buffer
		vkDestroyBuffer(device, stagingBuffer.buffer, nullptr);
		vkFreeMemory(device, stagingBuffer.memory, nullptr);

		computeStorageBuffer.descriptor.buffer = computeStorageBuffer.buffer;
		computeStorageBuffer.descriptor.offset = 0;
		computeStorageBuffer.descriptor.range = storageBufferSize;

		// Binding description
		vertices.bindingDescriptions.resize(1);
		vertices.bindingDescriptions[0] =
			vkTools::initializers::vertexInputBindingDescription(
				VERTEX_BUFFER_BIND_ID,
				sizeof(Particle),
				VK_VERTEX_INPUT_RATE_VERTEX);

		// Attribute descriptions
		// Describes memory layout and shader positions
		vertices.attributeDescriptions.resize(2);
		// Location 0 : Position
		vertices.attributeDescriptions[0] =
			vkTools::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				0,
				VK_FORMAT_R32G32B32A32_SFLOAT,
				0);
		// Location 1 : Color
		vertices.attributeDescriptions[1] =
			vkTools::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				1,
				VK_FORMAT_R32G32B32A32_SFLOAT,
				sizeof(float) * 4);

		// Assign to vertex buffer
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
			vkTools::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1),
			vkTools::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1),
			vkTools::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
		};

		VkDescriptorPoolCreateInfo descriptorPoolInfo =
			vkTools::initializers::descriptorPoolCreateInfo(
				poolSizes.size(),
				poolSizes.data(),
				3);

		VkResult vkRes = vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool);
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
				setLayoutBindings.size());

		VkResult err = vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayout);
		assert(!err);

		VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo =
			vkTools::initializers::pipelineLayoutCreateInfo(
				&descriptorSetLayout,
				1);

		err = vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &pipelineLayout);
		assert(!err);
	}

	void setupDescriptorSet()
	{
		VkDescriptorSetAllocateInfo allocInfo =
			vkTools::initializers::descriptorSetAllocateInfo(
				descriptorPool,
				&descriptorSetLayout,
				1);

		VkResult vkRes = vkAllocateDescriptorSets(device, &allocInfo, &descriptorSetPostCompute);
		assert(!vkRes);

		// Image descriptor for the color map texture
		VkDescriptorImageInfo texDescriptor =
			vkTools::initializers::descriptorImageInfo(
				textureColorMap.sampler,
				textureColorMap.view,
				VK_IMAGE_LAYOUT_GENERAL);

		std::vector<VkWriteDescriptorSet> writeDescriptorSets =
		{
			// Binding 1 : Fragment shader image sampler
			vkTools::initializers::writeDescriptorSet(
				descriptorSetPostCompute,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				0,
				&texDescriptor)
		};

		vkUpdateDescriptorSets(device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);
	}

	// Create a separate command buffer for compute commands
	void createComputeCommandBuffer()
	{
		VkCommandBufferAllocateInfo cmdBufAllocateInfo =
			vkTools::initializers::commandBufferAllocateInfo(
				cmdPool,
				VK_COMMAND_BUFFER_LEVEL_PRIMARY,
				1);

		VkResult vkRes = vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &computeCmdBuffer);
		assert(!vkRes);
	}

	void preparePipelines()
	{
		VkResult err;

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
			vkTools::initializers::pipelineInputAssemblyStateCreateInfo(
				VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
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

		// Rendering pipeline
		// Load shaders
		std::array<VkPipelineShaderStageCreateInfo,2> shaderStages;

		shaderStages[0] = loadShader(getAssetPath() + "shaders/computeparticles/particle.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getAssetPath() + "shaders/computeparticles/particle.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

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

		err = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.postCompute);
		assert(!err);
	}

	void prepareCompute()
	{
		// Create compute pipeline
		// Compute pipelines are created separate from graphics pipelines
		// even if they use the same queue

		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
			// Binding 0 : Particle position storage buffer
			vkTools::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				VK_SHADER_STAGE_COMPUTE_BIT,
				0),
			// Binding 1 : Uniform buffer
			vkTools::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				VK_SHADER_STAGE_COMPUTE_BIT,
				1),
		};

		VkDescriptorSetLayoutCreateInfo descriptorLayout =
			vkTools::initializers::descriptorSetLayoutCreateInfo(
				setLayoutBindings.data(),
				setLayoutBindings.size());

		VkResult err = vkCreateDescriptorSetLayout(
			device,
			&descriptorLayout,
			nullptr,
			&computeDescriptorSetLayout);
		assert(!err);


		VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo =
			vkTools::initializers::pipelineLayoutCreateInfo(
				&computeDescriptorSetLayout,
				1);

		err = vkCreatePipelineLayout(
			device,
			&pPipelineLayoutCreateInfo,
			nullptr,
			&computePipelineLayout);
		assert(!err);

		VkDescriptorSetAllocateInfo allocInfo =
			vkTools::initializers::descriptorSetAllocateInfo(
				descriptorPool,
				&computeDescriptorSetLayout,
				1);

		err = vkAllocateDescriptorSets(device, &allocInfo, &computeDescriptorSet);
		assert(!err);

		std::vector<VkWriteDescriptorSet> computeWriteDescriptorSets =
		{
			// Binding 0 : Particle position storage buffer
			vkTools::initializers::writeDescriptorSet(
				computeDescriptorSet,
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				0,
				&computeStorageBuffer.descriptor),
			// Binding 1 : Uniform buffer
			vkTools::initializers::writeDescriptorSet(
				computeDescriptorSet,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				1,
				&uniformData.computeShader.ubo.descriptor)
		};

		vkUpdateDescriptorSets(device, computeWriteDescriptorSets.size(), computeWriteDescriptorSets.data(), 0, NULL);

		// Create pipeline		
		VkComputePipelineCreateInfo computePipelineCreateInfo =
			vkTools::initializers::computePipelineCreateInfo(
				computePipelineLayout,
				0);
		computePipelineCreateInfo.stage = loadShader(getAssetPath() + "shaders/computeparticles/particle.comp.spv", VK_SHADER_STAGE_COMPUTE_BIT);
		err = vkCreateComputePipelines(device, pipelineCache, 1, &computePipelineCreateInfo, nullptr, &pipelines.compute);
		assert(!err);
	}

	// Prepare and initialize uniform buffer containing shader uniforms
	void prepareUniformBuffers()
	{
		// Compute shader uniform buffer block
		createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			sizeof(computeUbo),
			&computeUbo,
			&uniformData.computeShader.ubo.buffer,
			&uniformData.computeShader.ubo.memory,
			&uniformData.computeShader.ubo.descriptor);

		updateUniformBuffers();
	}

	void updateUniformBuffers()
	{
		computeUbo.deltaT = frameTimer * 5.0f;
		computeUbo.destX = sin(glm::radians(timer*360.0)) * 0.75f;
		computeUbo.destY = 0;
		uint8_t *pData;
		VkResult err = vkMapMemory(device, uniformData.computeShader.ubo.memory, 0, sizeof(computeUbo), 0, (void **)&pData);
		assert(!err);
		memcpy(pData, &computeUbo, sizeof(computeUbo));
		vkUnmapMemory(device, uniformData.computeShader.ubo.memory);
	}

	// Find and create a compute capable device queue
	void getComputeQueue()
	{
		uint32_t queueIndex = 0;
		uint32_t queueCount;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, NULL);
		assert(queueCount >= 1);

		std::vector<VkQueueFamilyProperties> queueProps;
		queueProps.resize(queueCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, queueProps.data());
		
		for (queueIndex = 0; queueIndex < queueCount; queueIndex++)
		{
			if (queueProps[queueIndex].queueFlags & VK_QUEUE_COMPUTE_BIT)
				break;
		}
		assert(queueIndex < queueCount);

		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.queueFamilyIndex = queueIndex;
		queueCreateInfo.queueCount = 1;
		vkGetDeviceQueue(device, queueIndex, 0, &computeQueue);
	}

	void prepare()
	{
		VulkanExampleBase::prepare();
		loadTextures();
		getComputeQueue();
		createComputeCommandBuffer();
		prepareStorageBuffers();
		prepareUniformBuffers();
		setupDescriptorSetLayout();
		preparePipelines();
		setupDescriptorPool();
		setupDescriptorSet();
		prepareCompute();
		buildCommandBuffers(); 
		buildComputeCommandBuffer();
		prepared = true;
	}

	virtual void render()
	{
		if (!prepared)
			return;
		vkDeviceWaitIdle(device);
		draw();
		vkDeviceWaitIdle(device);
		if (animStart > 0.0f)
		{
			animStart -= frameTimer * 5.0f;
		}
		if ((animate) & (animStart <= 0.0f))
		{
			timer += frameTimer * 0.1f;
			if (timer > 1.0)
			{
				timer -= 1.0f;
			}
		}
		updateUniformBuffers();
	}

	void toggleAnimation()
	{
		animate = !animate;
	}
};

VulkanExample *vulkanExample;

#if defined(_WIN32)
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (vulkanExample != NULL)
	{
		vulkanExample->handleMessages(hWnd, uMsg, wParam, lParam);
		if (uMsg == WM_KEYDOWN)
		{
			switch (wParam)
			{
			case 0x41:
				vulkanExample->toggleAnimation();
				break;
			}
		}
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