/*
* Vulkan Example - Device address commands (VK_KHR_device_address_commands)
*
* todo
* 
* Copyright (C) 2026 by Sascha Willems - www.saschawillems.de
*
*/

#include "vulkanexamplebase.h"
#include "VulkanglTFModel.h"

class VulkanExample : public VulkanExampleBase
{
public:
	bool animate = true;

	struct Cube {
		glm::mat4 modelMatrix;
		glm::vec3 rotation{ };
		std::array<vks::Buffer, maxConcurrentFrames> buffers{};
		std::array<VkDeviceAddress, maxConcurrentFrames> bufferDeviceAddresses{};
	};
	std::array<Cube, 2> cubes{};

	vks::Texture2D texture;
	vkglTF::Model model;

	// Global matrices
	struct Scene {
		glm::mat4 mvp;
		std::array<vks::Buffer, maxConcurrentFrames> buffers;
		std::array<VkDeviceAddress, maxConcurrentFrames> bufferDeviceAddresses{};
	} scene;

	VkPipeline pipeline{ VK_NULL_HANDLE };
	VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };
	VkDescriptorSet descriptorSet{ VK_NULL_HANDLE };
	VkDescriptorSetLayout descriptorSetLayout{ VK_NULL_HANDLE };

	PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR{ VK_NULL_HANDLE };
	VkPhysicalDeviceBufferDeviceAddressFeatures enabledBufferDeviceAddresFeatures{};

	PFN_vkCmdFillMemoryKHR vkCmdFillMemoryKHR{ VK_NULL_HANDLE };
	PFN_vkCmdBindVertexBuffers3KHR vkCmdBindVertexBuffers3KHR{ VK_NULL_HANDLE };
	PFN_vkCmdBindIndexBuffer3KHR vkCmdBindIndexBuffer3KHR{ VK_NULL_HANDLE };
	PFN_vkCmdDrawIndexedIndirect2KHR vkCmdDrawIndexedIndirect2KHR{ VK_NULL_HANDLE };
	VkPhysicalDeviceDeviceAddressCommandsFeaturesKHR enabledDeviceDeviceAddressCommandsFeaturesKHR{};

	// This sample passes the buffer references ("pointer") using push constants, the shader then reads data from that buffer address
	struct PushConstantBlock {
		// Reference to the global matrices
		VkDeviceAddress sceneReference;
		// Reference to the per model matrices
		VkDeviceAddress modelReference;
	};

	struct Vertex {
		float position[3];
		float color[3];
	};

	vks::Buffer vertexBuffer;
	vks::Buffer indexBuffer;
	uint32_t indexCount{ 0 };

	vks::Buffer indirectDrawBuffer;

	VulkanExample() : VulkanExampleBase()
	{
		title = "Device address commands";
		camera.type = Camera::CameraType::lookat;
		camera.setPerspective(60.0f, (float)width / (float)height, 0.1f, 512.0f);
		camera.setRotation(glm::vec3(0.0f, -180.0f, 0.0f));
		camera.setTranslation(glm::vec3(0.0f, 0.0f, -2.5f));

		enabledInstanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
		enabledInstanceExtensions.push_back(VK_KHR_DEVICE_GROUP_CREATION_EXTENSION_NAME);
		enabledDeviceExtensions.push_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
		enabledDeviceExtensions.push_back(VK_KHR_DEVICE_GROUP_EXTENSION_NAME);
		enabledDeviceExtensions.push_back(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);
		enabledDeviceExtensions.push_back(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME);
		enabledDeviceExtensions.push_back(VK_KHR_DEVICE_ADDRESS_COMMANDS_EXTENSION_NAME);

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
			vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
			texture.destroy();
			for (auto& cube : cubes) {
				for (auto& buffer : cube.buffers) {
					buffer.destroy();
				}
			}
			for (auto& buffer : scene.buffers) {
				buffer.destroy();
			}
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

	void loadAssets()
	{
		const uint32_t glTFLoadingFlags = vkglTF::FileLoadingFlags::PreTransformVertices | vkglTF::FileLoadingFlags::PreMultiplyVertexColors | vkglTF::FileLoadingFlags::FlipY;
		model.loadFromFile(getAssetPath() + "models/cube.gltf", vulkanDevice, queue, glTFLoadingFlags);
		texture.loadFromFile(getAssetPath() + "textures/crate01_color_height_rgba.ktx", VK_FORMAT_R8G8B8A8_UNORM, vulkanDevice, queue);
		// @todo
		createVertexBuffer();
	}

	// Rename...
	void createVertexBuffer()
	{
		// @todo: create multiple triangles, maybe with texture mapping or at least different color		
		// @todo: then switch BDA (triangle) in shader based on draw index?
		// @todo: combine vtx and idx buffer into single buffer

		std::vector<Vertex> vertices = {
			{ {  1.0f,  1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
			{ { -1.0f,  1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
			{ {  0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } }
		};
		uint32_t vertexBufferSize = static_cast<uint32_t>(vertices.size()) * sizeof(Vertex);
		std::vector<uint32_t> indices = { 0, 1, 2 };
		indexCount = static_cast<uint32_t>(indices.size());
		uint32_t indexBufferSize = indexCount * sizeof(uint32_t);

		struct StagingBuffers {
			vks::Buffer vertexBuffer;
			vks::Buffer indexBuffer;
		} stagingBuffers;

		// Host visible source buffers (staging)
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&stagingBuffers.vertexBuffer,
			vertexBufferSize,
			vertices.data()));
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&stagingBuffers.indexBuffer,
			indexBufferSize,
			indices.data()));

		// Device local destination buffers
		// @todo: check if vtx/idx buf flags are still required
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			&vertexBuffer,
			vertexBufferSize));
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			&indexBuffer,
			indexBufferSize));

		// Copy from host do device
		vulkanDevice->copyBuffer(&stagingBuffers.vertexBuffer, &vertexBuffer, queue);
		vulkanDevice->copyBuffer(&stagingBuffers.indexBuffer, &indexBuffer, queue);

		// Clean up
		stagingBuffers.vertexBuffer.destroy();
		stagingBuffers.indexBuffer.destroy();

		// Indirect draw buffer
		// @todo
		uint32_t indirectDrawBufferSize = sizeof(VkDrawIndexedIndirectCommand);
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			&indirectDrawBuffer,
			vertexBufferSize));

		vertexBuffer.deviceAddress = getBufferDeviceAddress(vertexBuffer.buffer);
		indexBuffer.deviceAddress = getBufferDeviceAddress(indexBuffer.buffer);
		indirectDrawBuffer.deviceAddress = getBufferDeviceAddress(indirectDrawBuffer.buffer);
	}

	void setupDescriptors()
	{
		// Pool
		std::vector<VkDescriptorPoolSize> descriptorPoolSizes = {
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
		};
		VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(descriptorPoolSizes, 2);
		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));
		// Layout
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0)
		};
		VkDescriptorSetLayoutCreateInfo descriptorLayout = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayout));
		// Set
		VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));
		std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
			vks::initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &texture.descriptor)
		};
		vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
	}

	void preparePipelines()
	{
		// The buffer addresses will be passed to the shader using push constants
		// That way it's very easy to do a draw call, change the reference to another buffer (or part of that buffer) and do the next draw call using different data
		VkPushConstantRange pushConstantRange{
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.offset = 0,
			.size = sizeof(PushConstantBlock)
		};

		VkPipelineLayoutCreateInfo pipelineLayoutCI{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = 1,
			.pSetLayouts = &descriptorSetLayout,
			.pushConstantRangeCount = 1,
			.pPushConstantRanges = &pushConstantRange
		};
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &pipelineLayout));

		const std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
		VkPipelineRasterizationStateCreateInfo rasterizationStateCI = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
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

			// Get the device of this buffer that is later on passed to the shader (aka "reference")
			VkBufferDeviceAddressInfo bufferDeviceAdressInfo{
				.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
				.buffer = scene.buffers[i].buffer
			};
			scene.bufferDeviceAddresses[i] = vkGetBufferDeviceAddressKHR(device, &bufferDeviceAdressInfo);

			for (auto& cube : cubes) {
				// Note that we don't use this buffer for uniforms but rather pass it's address as a reference to the shader, so isntead of the uniform buffer usage we use a different flag
				VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &cube.buffers[i], sizeof(glm::mat4)));
				VK_CHECK_RESULT(cube.buffers[i].map());

				// Get the device of this buffer that is later on passed to the shader (aka "reference")
				bufferDeviceAdressInfo.buffer = cube.buffers[i].buffer;
				cube.bufferDeviceAddresses[i] = vkGetBufferDeviceAddressKHR(device, &bufferDeviceAdressInfo);
			}
		}
	}

	void updateUniformBuffers()
	{
		scene.mvp = camera.matrices.perspective * camera.matrices.view;
		memcpy(scene.buffers[currentBuffer].mapped, &scene, sizeof(glm::mat4));

		cubes[0].modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
		cubes[1].modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(1.5f, 0.5f, 0.0f));

		for (auto& cube : cubes) {
			//cube.modelMatrix = glm::rotate(cube.modelMatrix, glm::radians(cube.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
			//cube.modelMatrix = glm::rotate(cube.modelMatrix, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			//cube.modelMatrix = glm::rotate(cube.modelMatrix, glm::radians(cube.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
			//cube.modelMatrix = glm::scale(cube.modelMatrix, glm::vec3(0.25f));
			memcpy(cube.buffers[currentBuffer].mapped, &cube.modelMatrix, sizeof(glm::mat4));
		}
	}

	void prepare()
	{
		VulkanExampleBase::prepare();

		vkGetBufferDeviceAddressKHR = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(vkGetDeviceProcAddr(device, "vkGetBufferDeviceAddressKHR"));
		vkCmdFillMemoryKHR = reinterpret_cast<PFN_vkCmdFillMemoryKHR>(vkGetDeviceProcAddr(device, "vkCmdFillMemoryKHR"));
		vkCmdBindVertexBuffers3KHR = reinterpret_cast<PFN_vkCmdBindVertexBuffers3KHR>(vkGetDeviceProcAddr(device, "vkCmdBindVertexBuffers3KHR"));
		vkCmdBindIndexBuffer3KHR = reinterpret_cast<PFN_vkCmdBindIndexBuffer3KHR>(vkGetDeviceProcAddr(device, "vkCmdBindIndexBuffer3KHR"));
		vkCmdDrawIndexedIndirect2KHR = reinterpret_cast<PFN_vkCmdDrawIndexedIndirect2KHR>(vkGetDeviceProcAddr(device, "vkCmdDrawIndexedIndirect2KHR"));

		loadAssets();
		prepareUniformBuffers();
		setupDescriptors();
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

		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

		// @todo
		VkBindVertexBuffer3InfoKHR vertexBindingInfo = {
			.sType = VK_STRUCTURE_TYPE_BIND_VERTEX_BUFFER_3_INFO_KHR,
			.addressRange = {vertexBuffer.deviceAddress, vertexBuffer.size},
			.addressFlags = VK_ADDRESS_COMMAND_FULLY_BOUND_BIT_KHR,
		};

		VkBindIndexBuffer3InfoKHR indexBindingInfo = {
			.sType = VK_STRUCTURE_TYPE_BIND_INDEX_BUFFER_3_INFO_KHR,
			.addressRange = {indexBuffer.deviceAddress, indexBuffer.size},
			.addressFlags = VK_ADDRESS_COMMAND_FULLY_BOUND_BIT_KHR,
			.indexType = VK_INDEX_TYPE_UINT32,
		};

		VkDrawIndirect2InfoKHR indirectInfo = {
			.sType = VK_STRUCTURE_TYPE_DRAW_INDIRECT_2_INFO_KHR,
			.addressRange = {indirectDrawBuffer.deviceAddress, indirectDrawBuffer.size, sizeof(VkDrawIndexedIndirectCommand)},
			.addressFlags = VK_ADDRESS_COMMAND_FULLY_BOUND_BIT_KHR,
			.drawCount = 1
		};

		VkDeviceAddressRangeKHR range = {
			.address = indirectDrawBuffer.deviceAddress,
			.size = sizeof(uint32_t)
			//.size = indirectDrawBuffer.size
		};

		VkDrawIndexedIndirectCommand idc = {
			.indexCount = 6,
			.instanceCount = 1,
			.firstIndex = 0
		};
		vkCmdFillMemoryKHR(cmdBuffer, &range, VK_ADDRESS_COMMAND_FULLY_BOUND_BIT_KHR, idc.indexCount);
		range.address += sizeof(uint32_t);
		vkCmdFillMemoryKHR(cmdBuffer, &range, VK_ADDRESS_COMMAND_FULLY_BOUND_BIT_KHR, idc.instanceCount);
		range.address += sizeof(uint32_t);
		vkCmdFillMemoryKHR(cmdBuffer, &range, VK_ADDRESS_COMMAND_FULLY_BOUND_BIT_KHR, idc.firstIndex);

		vkCmdBindVertexBuffers3KHR(cmdBuffer, 0, 1, &vertexBindingInfo);
		vkCmdBindIndexBuffer3KHR(cmdBuffer, &indexBindingInfo);
		
		//model.bindBuffers(cmdBuffer);

		// Instead of using descriptors to pass global and per-model matrices to the shader, we can now simply pass buffer references via push constants
		// The shader then simply reads data from the address of that reference
		PushConstantBlock references{};
		// Pass pointer to the global matrix via a buffer device address
		references.sceneReference = scene.bufferDeviceAddresses[currentBuffer];
		references.modelReference = cubes[0].bufferDeviceAddresses[currentBuffer];
		vkCmdPushConstants(cmdBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantBlock), &references);

		vkCmdDrawIndexedIndirect2KHR(cmdBuffer, &indirectInfo);

		/*
		for (auto& cube : cubes) {
			// Pass pointer to this cube's data buffer via a buffer device address
			// So instead of having to bind different descriptors, we only pass a different device address
			// This doesn't have to be an address from a different buffer, but could very well be just another address in the same buffer
			references.modelReference = cube.bufferDeviceAddresses[currentBuffer];
			vkCmdPushConstants(cmdBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantBlock), &references);

			model.draw(cmdBuffer);
		}
		*/

		//drawUI(cmdBuffer);

		vkCmdEndRenderPass(cmdBuffer);

		VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffer));
	}

	virtual void render()
	{
		if (!prepared)
			return;
		VulkanExampleBase::prepareFrame();
		/*
		if (animate && !paused) {
			cubes[0].rotation.x += 2.5f * frameTimer;
			if (cubes[0].rotation.x > 360.0f)
				cubes[0].rotation.x -= 360.0f;
			cubes[1].rotation.y += 2.0f * frameTimer;
			if (cubes[1].rotation.x > 360.0f)
				cubes[1].rotation.x -= 360.0f;
		}
		*/
		updateUniformBuffers();
		buildCommandBuffer();
		VulkanExampleBase::submitFrame();
	}

	virtual void OnUpdateUIOverlay(vks::UIOverlay* overlay)
	{
		if (overlay->header("Settings")) {
			overlay->checkBox("Animate", &animate);
		}
	}
};

VULKAN_EXAMPLE_MAIN()
