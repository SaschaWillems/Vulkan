/*
* Vulkan Example - Device address commands
*
* This sample shows how to use buffer device addresses for additional functionality via the VK_KHR_device_address_commands extension.
* The extension builts upon VK_KHR_buffer_device_address and lets you use handles instead of buffer objects for functionality
* like vertex, index and indirect draw buffers
*
* Copyright (C) 2026 by Sascha Willems - www.saschawillems.de
*
*/

#include "vulkanexamplebase.h"

class VulkanExample : public VulkanExampleBase
{
public:
	struct Scene {
		struct Matrices {
			glm::mat4 camera;
			glm::mat4 model[2];
		} matrices;
		std::array<vks::Buffer, maxConcurrentFrames> buffers;
	} scene;

	VkPipeline pipeline{ VK_NULL_HANDLE };
	VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };

	PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR{ VK_NULL_HANDLE };
	VkPhysicalDeviceBufferDeviceAddressFeatures enabledBufferDeviceAddresFeatures{};

	PFN_vkCmdUpdateMemoryKHR vkCmdUpdateMemoryKHR{ VK_NULL_HANDLE };
	PFN_vkCmdBindVertexBuffers3KHR vkCmdBindVertexBuffers3KHR{ VK_NULL_HANDLE };
	PFN_vkCmdBindIndexBuffer3KHR vkCmdBindIndexBuffer3KHR{ VK_NULL_HANDLE };
	PFN_vkCmdDrawIndexedIndirect2KHR vkCmdDrawIndexedIndirect2KHR{ VK_NULL_HANDLE };
	VkPhysicalDeviceDeviceAddressCommandsFeaturesKHR enabledDeviceDeviceAddressCommandsFeaturesKHR{};

	struct Vertex {
		float position[3];
		float color[3];
	};

	vks::Buffer vertexBuffer;
	uint32_t vertexBufferSize{ 0 };
	uint32_t indexBufferSize{ 0 };
	uint32_t indirectDrawBufferSize{ 0 };

	vks::Buffer indirectDrawBuffer;

	VulkanExample() : VulkanExampleBase()
	{
		title = "Device address commands";
		camera.type = Camera::CameraType::lookat;
		camera.setPerspective(60.0f, (float)width / (float)height, 0.1f, 512.0f);
		camera.setRotation(glm::vec3(0.0f, 0.0f, 0.0f));
		camera.setTranslation(glm::vec3(0.0f, 0.0f, -2.5f));

		enabledInstanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
		enabledInstanceExtensions.push_back(VK_KHR_DEVICE_GROUP_CREATION_EXTENSION_NAME);
		enabledDeviceExtensions.push_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
		enabledDeviceExtensions.push_back(VK_KHR_DEVICE_GROUP_EXTENSION_NAME);
		enabledDeviceExtensions.push_back(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);
		enabledDeviceExtensions.push_back(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME);
		enabledDeviceExtensions.push_back(VK_KHR_DEVICE_ADDRESS_COMMANDS_EXTENSION_NAME);

		enabledFeatures.multiDrawIndirect = VK_TRUE;
		
		enabledBufferDeviceAddresFeatures = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
			.bufferDeviceAddress = VK_TRUE
		};

		enabledDeviceDeviceAddressCommandsFeaturesKHR = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_ADDRESS_COMMANDS_FEATURES_KHR,
			.pNext = &enabledBufferDeviceAddresFeatures,
			.deviceAddressCommands = VK_TRUE
		};

		deviceCreatepNextChain = &enabledDeviceDeviceAddressCommandsFeaturesKHR;

	}

	~VulkanExample()
	{
		if (device) {
			vkDestroyPipeline(device, pipeline, nullptr);
			vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
			for (auto& buffer : scene.buffers) {
				buffer.destroy();
			}
			indirectDrawBuffer.destroy();
			vertexBuffer.destroy();
		}
	}

	virtual void getEnabledFeatures()
	{
		if (deviceFeatures.samplerAnisotropy) {
			enabledFeatures.samplerAnisotropy = VK_TRUE;
		};
	}

	uint64_t getBufferDeviceAddress(VkBuffer buffer)
	{
		VkBufferDeviceAddressInfoKHR bufferDeviceAI{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = buffer};
		return vkGetBufferDeviceAddressKHR(vulkanDevice->logicalDevice, &bufferDeviceAI);
	}

	void createBuffers()
	{
		std::vector<Vertex> vertices = {
			// First triangle
			{ {  1.0f,  1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
			{ { -1.0f,  1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
			{ {  0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
			// Second triangle
			{ {  1.0f, -1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
			{ { -1.0f, -1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
			{ {  0.0f,  1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } }
		};
		std::vector<uint32_t> indices = { 2, 1, 0, 5, 4, 3 };
		vertexBufferSize = static_cast<uint32_t>(vertices.size()) * sizeof(Vertex);
		indexBufferSize = static_cast<uint32_t>(indices.size()) * sizeof(uint32_t);

		vks::Buffer stagingBuffer;

		// Host visible source buffers (staging)
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&stagingBuffer,
			vertexBufferSize + indexBufferSize));
		stagingBuffer.map();
		// Vertex and index buffer are combined into one buffer (vertices first, then indices)
		memcpy(stagingBuffer.mapped, vertices.data(), vertexBufferSize);
		memcpy((char*)stagingBuffer.mapped + vertexBufferSize, indices.data(), indexBufferSize);

		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			&vertexBuffer,
			vertexBufferSize + indexBufferSize));

		vulkanDevice->copyBuffer(&stagingBuffer, &vertexBuffer, queue);
		stagingBuffer.destroy();

		// Indirect draw buffer for two draw calls (one per triangle)
		indirectDrawBufferSize = 2 * sizeof(VkDrawIndexedIndirectCommand);
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			&indirectDrawBuffer,
			vertexBufferSize));

		// With VK_KHR_device_address_commands we can access vertex/index and indirect draw buffers by their device address
		vertexBuffer.deviceAddress = getBufferDeviceAddress(vertexBuffer.buffer);
		indirectDrawBuffer.deviceAddress = getBufferDeviceAddress(indirectDrawBuffer.buffer);
	}

	void preparePipelines()
	{
		// The buffer addresses will be passed to the shader using push constants
		VkPushConstantRange pushConstantRange{
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.offset = 0,
			.size = sizeof(VkDeviceAddress)
		};

		VkPipelineLayoutCreateInfo pipelineLayoutCI{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.pushConstantRangeCount = 1,
			.pPushConstantRanges = &pushConstantRange
		};
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &pipelineLayout));

		const std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
		VkPipelineRasterizationStateCreateInfo rasterizationStateCI = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
		VkPipelineColorBlendAttachmentState blendAttachmentState = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
		VkPipelineColorBlendStateCreateInfo colorBlendStateCI = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
		VkPipelineDepthStencilStateCreateInfo depthStencilStateCI = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
		VkPipelineViewportStateCreateInfo viewportStateCI = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
		VkPipelineMultisampleStateCreateInfo multisampleStateCI = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
		VkPipelineDynamicStateCreateInfo dynamicStateCI = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables.data(), static_cast<uint32_t>(dynamicStateEnables.size()), 0);
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = {
			loadShader(getShadersPath() + "deviceaddresscommands/cube.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
			loadShader(getShadersPath() + "deviceaddresscommands/cube.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT)
		};

		// Vertex bindings and attributes
		std::vector<VkVertexInputBindingDescription> vertexInputBindings{
			vks::initializers::vertexInputBindingDescription(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX),
		};
		std::vector<VkVertexInputAttributeDescription> vertexInputAttributes{
			vks::initializers::vertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)),
			vks::initializers::vertexInputAttributeDescription(0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)),
		};
		VkPipelineVertexInputStateCreateInfo vertexInputState{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindings.size()),
			.pVertexBindingDescriptions = vertexInputBindings.data(),
			.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size()),
			.pVertexAttributeDescriptions = vertexInputAttributes.data()
		};

		VkGraphicsPipelineCreateInfo pipelineCI{
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.stageCount = static_cast<uint32_t>(shaderStages.size()),
			.pStages = shaderStages.data(),
			.pVertexInputState = &vertexInputState,
			.pInputAssemblyState = &inputAssemblyStateCI,
			.pViewportState = &viewportStateCI,
			.pRasterizationState = &rasterizationStateCI,
			.pMultisampleState = &multisampleStateCI,
			.pDepthStencilState = &depthStencilStateCI,
			.pColorBlendState = &colorBlendStateCI,
			.pDynamicState = &dynamicStateCI,
			.layout = pipelineLayout,
			.renderPass = renderPass
		};
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipeline));
	}

	void prepareUniformBuffers()
	{
		for (uint32_t i = 0; i < maxConcurrentFrames; i++) {
			// Note that we don't use this buffer for uniforms but rather pass it's address as a reference to the shader, so isntead of the uniform buffer usage we use a different flag
			VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &scene.buffers[i], sizeof(glm::mat4)));
			VK_CHECK_RESULT(scene.buffers[i].map());
			scene.buffers[i].deviceAddress = getBufferDeviceAddress(scene.buffers[i].buffer);
		}
	}

	void updateUniformBuffers()
	{
		scene.matrices.camera = camera.matrices.perspective * camera.matrices.view;
		scene.matrices.model[0] = glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, 0.0f, 0.0f));
		scene.matrices.model[1] = glm::translate(glm::mat4(1.0f), glm::vec3( 1.0f, 0.0f, 0.0f));
		memcpy(scene.buffers[currentBuffer].mapped, &scene.matrices, sizeof(scene.matrices));
	}

	void prepare()
	{
		VulkanExampleBase::prepare();

		vkGetBufferDeviceAddressKHR = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(vkGetDeviceProcAddr(device, "vkGetBufferDeviceAddressKHR"));
		vkCmdUpdateMemoryKHR = reinterpret_cast<PFN_vkCmdUpdateMemoryKHR>(vkGetDeviceProcAddr(device, "vkCmdUpdateMemoryKHR"));
		vkCmdBindVertexBuffers3KHR = reinterpret_cast<PFN_vkCmdBindVertexBuffers3KHR>(vkGetDeviceProcAddr(device, "vkCmdBindVertexBuffers3KHR"));
		vkCmdBindIndexBuffer3KHR = reinterpret_cast<PFN_vkCmdBindIndexBuffer3KHR>(vkGetDeviceProcAddr(device, "vkCmdBindIndexBuffer3KHR"));
		vkCmdDrawIndexedIndirect2KHR = reinterpret_cast<PFN_vkCmdDrawIndexedIndirect2KHR>(vkGetDeviceProcAddr(device, "vkCmdDrawIndexedIndirect2KHR"));

		createBuffers();
		prepareUniformBuffers();
		preparePipelines();
		prepared = true;
	}

	void buildCommandBuffer()
	{
		VkCommandBuffer cmdBuffer = drawCmdBuffers[currentBuffer];
		
		VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

		VkClearValue clearValues[2]{};
		clearValues[0].color = defaultClearColor;
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderPassBeginInfo{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = renderPass,
			.framebuffer = frameBuffers[currentImageIndex],
			.renderArea = {.offset = {.x = 0, .y = 0 }, .extent = {.width = width, .height = height } },
			.clearValueCount = 2,
			.pClearValues = clearValues,
		};

		VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));

		vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		VkViewport viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
		vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

		VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);
		vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

		// Update indirect draw commands
		// Note that this is an artificial example, as the draw calls never actually change
		VkDrawIndirect2InfoKHR indirectInfo{
			.sType = VK_STRUCTURE_TYPE_DRAW_INDIRECT_2_INFO_KHR,
			.addressRange = {indirectDrawBuffer.deviceAddress, indirectDrawBuffer.size, sizeof(VkDrawIndexedIndirectCommand)},
			.addressFlags = VK_ADDRESS_COMMAND_FULLY_BOUND_BIT_KHR,
			.drawCount = 2
		};

		VkDeviceAddressRangeKHR indirectRange{
			.address = indirectDrawBuffer.deviceAddress,
			.size = indirectDrawBuffer.size
		};

		std::array<VkDrawIndexedIndirectCommand, 2> indirectDrawCommands{
			VkDrawIndexedIndirectCommand{.indexCount = 3, .instanceCount = 1},
			VkDrawIndexedIndirectCommand{.indexCount = 3, .instanceCount = 1, .vertexOffset = 3, .firstInstance = 1 },
		};

		vkCmdUpdateMemoryKHR(cmdBuffer, &indirectRange, 0, indirectDrawBufferSize, indirectDrawCommands.data());

		// Vertex and index buffers can now be bound via their buffer device address instead
		VkBindVertexBuffer3InfoKHR vertexBindingInfo{
			.sType = VK_STRUCTURE_TYPE_BIND_VERTEX_BUFFER_3_INFO_KHR,
			.addressRange = {vertexBuffer.deviceAddress, vertexBuffer.size},
			.addressFlags = VK_ADDRESS_COMMAND_FULLY_BOUND_BIT_KHR,
		};

		VkBindIndexBuffer3InfoKHR indexBindingInfo{
			.sType = VK_STRUCTURE_TYPE_BIND_INDEX_BUFFER_3_INFO_KHR,
			.addressRange = {vertexBuffer.deviceAddress + vertexBufferSize, indexBufferSize},
			.addressFlags = VK_ADDRESS_COMMAND_FULLY_BOUND_BIT_KHR,
			.indexType = VK_INDEX_TYPE_UINT32,
		};

		vkCmdBindVertexBuffers3KHR(cmdBuffer, 0, 1, &vertexBindingInfo);
		vkCmdBindIndexBuffer3KHR(cmdBuffer, &indexBindingInfo);

		vkCmdPushConstants(cmdBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(VkDeviceAddress), &scene.buffers[currentBuffer].deviceAddress);

		vkCmdDrawIndexedIndirect2KHR(cmdBuffer, &indirectInfo);

		drawUI(cmdBuffer);

		vkCmdEndRenderPass(cmdBuffer);

		VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffer));
	}

	virtual void render()
	{
		if (!prepared)
			return;
		VulkanExampleBase::prepareFrame();

		updateUniformBuffers();
		buildCommandBuffer();
		VulkanExampleBase::submitFrame();
	}
};

VULKAN_EXAMPLE_MAIN()
