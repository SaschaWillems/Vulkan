/*
* Vulkan Example - Buffer devie address
*
* Copyright (C) 2024 by Sascha Willems - www.saschawillems.de
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
		vks::Buffer uniformBuffer;
		glm::vec3 rotation;
		VkDeviceAddress bufferDeviceAddress;
	};
	std::array<Cube, 2> cubes{};

	vks::Texture2D texture;
	vkglTF::Model model;

	// Global matrices
	struct UniformData {
		glm::mat4 projection;
		glm::mat4 view;
	} uniformData;
	vks::Buffer uniformBuffer;

	VkPipeline pipeline{ VK_NULL_HANDLE };
	VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };
	VkDescriptorSet descriptorSet{ VK_NULL_HANDLE };
	VkDescriptorSetLayout descriptorSetLayout{ VK_NULL_HANDLE };

	// @todo: comment
	PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR{ VK_NULL_HANDLE };

	VkPhysicalDeviceBufferDeviceAddressFeatures enabledBufferDeviceAddresFeatures{};

	VulkanExample() : VulkanExampleBase()
	{
		title = "Buffer device address";
		camera.type = Camera::CameraType::lookat;
		camera.setPerspective(60.0f, (float)width / (float)height, 0.1f, 512.0f);
		camera.setRotation(glm::vec3(0.0f, 0.0f, 0.0f));
		camera.setTranslation(glm::vec3(0.0f, 0.0f, -5.0f));

		enabledInstanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
		enabledInstanceExtensions.push_back(VK_KHR_DEVICE_GROUP_CREATION_EXTENSION_NAME);
		enabledDeviceExtensions.push_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
		enabledDeviceExtensions.push_back(VK_KHR_DEVICE_GROUP_EXTENSION_NAME);

		enabledBufferDeviceAddresFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
		enabledBufferDeviceAddresFeatures.bufferDeviceAddress = VK_TRUE;

		deviceCreatepNextChain = &enabledBufferDeviceAddresFeatures;
	}

	~VulkanExample()
	{
		if (device) {
			vkDestroyPipeline(device, pipeline, nullptr);
			vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
			vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
			texture.destroy();
			for (auto cube : cubes) {
				cube.uniformBuffer.destroy();
			}
		}
	}

	virtual void getEnabledFeatures()
	{
		if (deviceFeatures.samplerAnisotropy) {
			enabledFeatures.samplerAnisotropy = VK_TRUE;
		};
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

		for (int32_t i = 0; i < drawCmdBuffers.size(); ++i) {
			renderPassBeginInfo.framebuffer = frameBuffers[i];

			VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo));

			vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

			VkViewport viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
			vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

			VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);
			vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

			// @todo: comment all of this
			vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

			model.bindBuffers(drawCmdBuffers[i]);

			/*
				[POI] Render cubes with separate descriptor sets
			*/
			for (auto& cube : cubes) {
				// @todo: comment
				// Pass pointer to cube data buffer via BDA
				VkDeviceAddress pushConstant = cube.bufferDeviceAddress;
				vkCmdPushConstants(drawCmdBuffers[i], pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(VkDeviceAddress), &pushConstant);

				model.draw(drawCmdBuffers[i]);
			}

			drawUI(drawCmdBuffers[i]);

			vkCmdEndRenderPass(drawCmdBuffers[i]);

			VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
		}
	}

	void loadAssets()
	{
		const uint32_t glTFLoadingFlags = vkglTF::FileLoadingFlags::PreTransformVertices | vkglTF::FileLoadingFlags::PreMultiplyVertexColors | vkglTF::FileLoadingFlags::FlipY;
		model.loadFromFile(getAssetPath() + "models/cube.gltf", vulkanDevice, queue, glTFLoadingFlags);
		texture.loadFromFile(getAssetPath() + "textures/crate01_color_height_rgba.ktx", VK_FORMAT_R8G8B8A8_UNORM, vulkanDevice, queue);
	}

	/*
		[POI] Set up descriptor sets and set layout
	*/
	void setupDescriptors()
	{
		std::array<VkDescriptorSetLayoutBinding, 2> setLayoutBindings{};

		/*
			Binding 0: Uniform buffers (used to pass matrices)
		*/
		setLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		// Shader binding point
		setLayoutBindings[0].binding = 0;
		// Accessible from the vertex shader only (flags can be combined to make it accessible to multiple shader stages)
		setLayoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		// Binding contains one element (can be used for array bindings)
		setLayoutBindings[0].descriptorCount = 1;

		/*
			Binding 1: Combined image sampler (used to pass per object texture information)
		*/
		setLayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		setLayoutBindings[1].binding = 1;
		// Accessible from the fragment shader only
		setLayoutBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		setLayoutBindings[1].descriptorCount = 1;

		// Create the descriptor set layout
		VkDescriptorSetLayoutCreateInfo descriptorLayoutCI{};
		descriptorLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorLayoutCI.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
		descriptorLayoutCI.pBindings = setLayoutBindings.data();

		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayoutCI, nullptr, &descriptorSetLayout));

		std::array<VkDescriptorPoolSize, 2> descriptorPoolSizes{};

		// Uniform buffers : 1 per object
		descriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorPoolSizes[0].descriptorCount = static_cast<uint32_t>(cubes.size());

		// Combined image samples : 1 per object texture
		descriptorPoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorPoolSizes[1].descriptorCount = static_cast<uint32_t>(cubes.size());

		// Create the global descriptor pool
		VkDescriptorPoolCreateInfo descriptorPoolCI = {};
		descriptorPoolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolCI.poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size());
		descriptorPoolCI.pPoolSizes = descriptorPoolSizes.data();
		// Max. number of descriptor sets that can be allocated from this pool (one per object)
		descriptorPoolCI.maxSets = static_cast<uint32_t>(cubes.size());

		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolCI, nullptr, &descriptorPool));

		// Allocates an empty descriptor set without actual descriptors from the pool using the set layout
		VkDescriptorSetAllocateInfo allocateInfo{};
		allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocateInfo.descriptorPool = descriptorPool;
		allocateInfo.descriptorSetCount = 1;
		allocateInfo.pSetLayouts = &descriptorSetLayout;
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocateInfo, &descriptorSet));

		// Update the descriptor set with the actual descriptors matching shader bindings set in the layout

		std::array<VkWriteDescriptorSet, 2> writeDescriptorSets{};

		/*
			Binding 0: Object matrices Uniform buffer
		*/
		writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSets[0].dstSet = descriptorSet;
		writeDescriptorSets[0].dstBinding = 0;
		writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writeDescriptorSets[0].pBufferInfo = &uniformBuffer.descriptor;
		writeDescriptorSets[0].descriptorCount = 1;

		/*
			Binding 1: Object texture
		*/
		writeDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSets[1].dstSet = descriptorSet;
		writeDescriptorSets[1].dstBinding = 1;
		writeDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writeDescriptorSets[1].pImageInfo = &texture.descriptor;
		writeDescriptorSets[1].descriptorCount = 1;

		// Execute the writes to update descriptors for this set
		// Note that it's also possible to gather all writes and only run updates once, even for multiple sets
		// This is possible because each VkWriteDescriptorSet also contains the destination set to be updated
		// For simplicity we will update once per set instead

		vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
	}

	void preparePipelines()
	{
		VkPushConstantRange pushConstantRange{};
		// @todo: comment
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(VkDeviceAddress);

		VkPipelineLayoutCreateInfo pipelineLayoutCI = vks::initializers::pipelineLayoutCreateInfo();
		pipelineLayoutCI.pushConstantRangeCount = 1;
		pipelineLayoutCI.pPushConstantRanges = &pushConstantRange;
		pipelineLayoutCI.setLayoutCount = 1;
		pipelineLayoutCI.pSetLayouts = &descriptorSetLayout;
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
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

		VkGraphicsPipelineCreateInfo pipelineCI = vks::initializers::pipelineCreateInfo(pipelineLayout, renderPass, 0);
		pipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
		pipelineCI.pRasterizationState = &rasterizationStateCI;
		pipelineCI.pColorBlendState = &colorBlendStateCI;
		pipelineCI.pMultisampleState = &multisampleStateCI;
		pipelineCI.pViewportState = &viewportStateCI;
		pipelineCI.pDepthStencilState = &depthStencilStateCI;
		pipelineCI.pDynamicState = &dynamicStateCI;
		pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineCI.pStages = shaderStages.data();
		pipelineCI.pVertexInputState = vkglTF::Vertex::getPipelineVertexInputState({ vkglTF::VertexComponent::Position, vkglTF::VertexComponent::Normal, vkglTF::VertexComponent::UV, vkglTF::VertexComponent::Color });

		shaderStages[0] = loadShader(getShadersPath() + "bufferdeviceaddress/cube.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getShadersPath() + "bufferdeviceaddress/cube.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipeline));
	}

	void prepareUniformBuffers()
	{
		VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &uniformBuffer, sizeof(UniformData)));
		VK_CHECK_RESULT(uniformBuffer.map());

		// @todo: comment
		// @todo: ubo bit can be removed
		const VkBufferUsageFlags bufferUsageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
		for (auto& cube : cubes) {
			VK_CHECK_RESULT(vulkanDevice->createBuffer(bufferUsageFlags, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &cube.uniformBuffer, sizeof(glm::mat4)));
			VK_CHECK_RESULT(cube.uniformBuffer.map());

			// @todo: comment
			VkBufferDeviceAddressInfo bufferDeviceAdressInfo{};
			bufferDeviceAdressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
			bufferDeviceAdressInfo.buffer = cube.uniformBuffer.buffer;
			cube.bufferDeviceAddress = vkGetBufferDeviceAddressKHR(device, &bufferDeviceAdressInfo);
		}
		updateUniformBuffers();
	}

	void updateUniformBuffers()
	{
		uniformData.projection = camera.matrices.perspective;
		uniformData.view = camera.matrices.view;
		memcpy(uniformBuffer.mapped, &uniformData, sizeof(UniformData));

		cubes[0].modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, 0.0f, 0.0f));
		cubes[1].modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(1.5f, 0.5f, 0.0f));

		for (auto& cube : cubes) {
			cube.modelMatrix = glm::rotate(cube.modelMatrix, glm::radians(cube.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
			cube.modelMatrix = glm::rotate(cube.modelMatrix, glm::radians(cube.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
			cube.modelMatrix = glm::rotate(cube.modelMatrix, glm::radians(cube.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
			cube.modelMatrix = glm::scale(cube.modelMatrix, glm::vec3(0.25f));
			memcpy(cube.uniformBuffer.mapped, &cube.modelMatrix, sizeof(glm::mat4));
		}
	}

	void draw()
	{
		VulkanExampleBase::prepareFrame();
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
		VulkanExampleBase::submitFrame();
	}

	void prepare()
	{
		VulkanExampleBase::prepare();

		// @todo: comment
		vkGetBufferDeviceAddressKHR = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(vkGetDeviceProcAddr(device, "vkGetBufferDeviceAddressKHR"));

		loadAssets();
		prepareUniformBuffers();
		setupDescriptors();
		preparePipelines();
		buildCommandBuffers();
		prepared = true;
	}

	virtual void render()
	{
		if (!prepared)
			return;
		draw();
		if (animate && !paused) {
			cubes[0].rotation.x += 2.5f * frameTimer;
			if (cubes[0].rotation.x > 360.0f)
				cubes[0].rotation.x -= 360.0f;
			cubes[1].rotation.y += 2.0f * frameTimer;
			if (cubes[1].rotation.x > 360.0f)
				cubes[1].rotation.x -= 360.0f;
		}
		if ((camera.updated) || (animate && !paused)) {
			updateUniformBuffers();
		}
	}

	virtual void OnUpdateUIOverlay(vks::UIOverlay* overlay)
	{
		if (overlay->header("Settings")) {
			overlay->checkBox("Animate", &animate);
		}
	}
};

VULKAN_EXAMPLE_MAIN()
