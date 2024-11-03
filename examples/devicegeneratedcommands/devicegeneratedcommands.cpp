/*
 * Vulkan Example - Device generated commands
 *
 * Copyright (C) 2024 by Sascha Willems - www.saschawillems.de
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */

#include "vulkanexamplebase.h"
#include "VulkanglTFModel.h"


class VulkanExample : public VulkanExampleBase
{
public:
	std::vector<vkglTF::Model> models{};

	// Per-instance data block
	struct InstanceData {
		glm::vec3 pos;
		glm::vec3 rot;
		float scale;
		uint32_t texIndex;
	};

	vks::Buffer deviceCommandsBuffer;

	struct UniformData {
		glm::mat4 projection;
		glm::mat4 model{ glm::mat4(1.0f) };
		glm::mat4 view;
	} uniformData;
	vks::Buffer uniformBuffer;

	VkPipeline pipeline{ VK_NULL_HANDLE };
	VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };
	VkDescriptorSet descriptorSet{ VK_NULL_HANDLE };
	VkDescriptorSetLayout descriptorSetLayout{ VK_NULL_HANDLE };

	VkSampler samplerRepeat{ VK_NULL_HANDLE };

	uint32_t objectCount = 0;

	VkPhysicalDeviceDeviceGeneratedCommandsFeaturesEXT enabledDeviceGeneratedCommandsFeatures{};

	PFN_vkCreateIndirectExecutionSetEXT vkCreateIndirectExecutionSetEXT{ nullptr };
	PFN_vkDestroyIndirectExecutionSetEXT vkDestroyIndirectExecutionSetEXT{ nullptr };
	PFN_vkCreateIndirectCommandsLayoutEXT vkCreateIndirectCommandsLayoutEXT{ nullptr };
	PFN_vkDestroyIndirectCommandsLayoutEXT vkDestroyIndirectCommandsLayoutEXT{ nullptr };

	// @todo: base on pipeline library sample?

	VulkanExample() : VulkanExampleBase()
	{
		title = "Device generated commands";
		camera.type = Camera::CameraType::firstperson;
		camera.setPerspective(60.0f, (float)width / (float)height, 0.1f, 512.0f);
		camera.setTranslation(glm::vec3(0.0f, 0.0f, -10.0f));
		camera.movementSpeed = 5.0f;

		// VK_EXT_device_generated_commands requires api version 1.1, buffer device address and maintenance5
		enabledDeviceExtensions.push_back(VK_EXT_DEVICE_GENERATED_COMMANDS_EXTENSION_NAME);

		apiVersion = VK_API_VERSION_1_1;

		// Required by buffer device address
		enabledInstanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
		enabledInstanceExtensions.push_back(VK_KHR_DEVICE_GROUP_CREATION_EXTENSION_NAME);
		enabledDeviceExtensions.push_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
		enabledDeviceExtensions.push_back(VK_KHR_DEVICE_GROUP_EXTENSION_NAME);
		enabledDeviceExtensions.push_back(VK_KHR_MAINTENANCE_5_EXTENSION_NAME);
		// Required by maintenance5
		enabledDeviceExtensions.push_back(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
		// Required by dynamic rendering
		enabledDeviceExtensions.push_back(VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME);
		enabledDeviceExtensions.push_back(VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME);

		enabledDeviceGeneratedCommandsFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_GENERATED_COMMANDS_FEATURES_EXT;
		enabledDeviceGeneratedCommandsFeatures.deviceGeneratedCommands = VK_TRUE;

		deviceCreatepNextChain = &enabledDeviceGeneratedCommandsFeatures;
	}

	~VulkanExample()
	{
		if (device) {
			vkDestroyPipeline(device, pipeline, nullptr);
			vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
			vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
			uniformBuffer.destroy();
		}
	}

	// Enable physical device features required for this example
	virtual void getEnabledFeatures()
	{
		// Enable anisotropic filtering if supported
		if (deviceFeatures.samplerAnisotropy) {
			enabledFeatures.samplerAnisotropy = VK_TRUE;
		}
	};

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

			// Skysphere
			vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
			models[2].draw(drawCmdBuffers[i]);

			drawUI(drawCmdBuffers[i]);

			vkCmdEndRenderPass(drawCmdBuffers[i]);

			VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
		}
	}

	void loadAssets()
	{
		const uint32_t glTFLoadingFlags = vkglTF::FileLoadingFlags::PreTransformVertices | vkglTF::FileLoadingFlags::PreMultiplyVertexColors | vkglTF::FileLoadingFlags::FlipY;
		models.resize(3);
		models[0].loadFromFile(getAssetPath() + "models/cube.gltf", vulkanDevice, queue, glTFLoadingFlags);
		models[1].loadFromFile(getAssetPath() + "models/plane.gltf", vulkanDevice, queue, glTFLoadingFlags);
		models[2].loadFromFile(getAssetPath() + "models/sphere.gltf", vulkanDevice, queue, glTFLoadingFlags);
	}

	void setupDescriptors()
	{
		// Pool
		std::vector<VkDescriptorPoolSize> poolSizes = {
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1),
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2),
		};
		VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, 2);
		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));

		// Layout
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
			// Binding 0: Vertex shader uniform buffer
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
		};
		VkDescriptorSetLayoutCreateInfo descriptorLayout = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayout));

		// Set
		VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));
		std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
			// Binding 0: Vertex shader uniform buffer
			vks::initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffer.descriptor),
		};
		vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
	}

	void preparePipelines()
	{
		// Layout
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(&descriptorSetLayout, 1);
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

		// Pipelines
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
		VkPipelineRasterizationStateCreateInfo rasterizationState = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
		VkPipelineColorBlendAttachmentState blendAttachmentState = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
		VkPipelineColorBlendStateCreateInfo colorBlendState = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
		VkPipelineDepthStencilStateCreateInfo depthStencilState = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
		VkPipelineViewportStateCreateInfo viewportState = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
		VkPipelineMultisampleStateCreateInfo multisampleState = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
		std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicState = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

		VkGraphicsPipelineCreateInfo pipelineCreateInfo = vks::initializers::pipelineCreateInfo(pipelineLayout, renderPass);
		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
		pipelineCreateInfo.pRasterizationState = &rasterizationState;
		pipelineCreateInfo.pColorBlendState = &colorBlendState;
		pipelineCreateInfo.pMultisampleState = &multisampleState;
		pipelineCreateInfo.pViewportState = &viewportState;
		pipelineCreateInfo.pDepthStencilState = &depthStencilState;
		pipelineCreateInfo.pDynamicState = &dynamicState;
		pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineCreateInfo.pStages = shaderStages.data();
		pipelineCreateInfo.pVertexInputState = vkglTF::Vertex::getPipelineVertexInputState({ vkglTF::VertexComponent::Position, vkglTF::VertexComponent::Color });

		VkPipelineCreateFlags2CreateInfoKHR pipelineCIFlags2CI{};
		pipelineCIFlags2CI.sType = VK_STRUCTURE_TYPE_PIPELINE_CREATE_FLAGS_2_CREATE_INFO_KHR;
		pipelineCIFlags2CI.flags = VK_PIPELINE_CREATE_2_INDIRECT_BINDABLE_BIT_EXT;

		pipelineCreateInfo.pNext = &pipelineCIFlags2CI;

		shaderStages[0] = loadShader(getShadersPath() + "triangle/triangle.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getShadersPath() + "triangle/triangle.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		//rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipeline));
	}

	void prepareUniformBuffers()
	{
		VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &uniformBuffer, sizeof(uniformData)));
		VK_CHECK_RESULT(uniformBuffer.map());
	}

	void updateUniformBuffer()
	{
		uniformData.projection = camera.matrices.perspective;
		uniformData.view = camera.matrices.view;
		memcpy(uniformBuffer.mapped, &uniformData, sizeof(uniformData));
	}

	void prepareIndirectExecutionBuffer()
	{
		// Execution sets

		VkIndirectExecutionSetPipelineInfoEXT iePipeInfo{};
		iePipeInfo.sType = VK_STRUCTURE_TYPE_INDIRECT_EXECUTION_SET_PIPELINE_INFO_EXT;
		iePipeInfo.initialPipeline = pipeline;
		iePipeInfo.maxPipelineCount = 1;

		VkIndirectExecutionSetInfoEXT ieSetInfo{};
		ieSetInfo.pPipelineInfo = &iePipeInfo;

		VkIndirectExecutionSetCreateInfoEXT indirectExecutionSetCI{};
		indirectExecutionSetCI.sType = VK_STRUCTURE_TYPE_INDIRECT_EXECUTION_SET_CREATE_INFO_EXT;
		indirectExecutionSetCI.type = VK_INDIRECT_EXECUTION_SET_INFO_TYPE_PIPELINES_EXT;
		indirectExecutionSetCI.info = ieSetInfo;

		VkIndirectExecutionSetEXT ieSet{};

		VK_CHECK_RESULT(vkCreateIndirectExecutionSetEXT(device, &indirectExecutionSetCI, nullptr, &ieSet));

		// Command tokens 

		VkBindIndexBufferIndirectCommandEXT ibic{};
		ibic.bufferAddress = 0; // @todo
		ibic.indexType = VK_INDEX_TYPE_UINT32;
		ibic.size = 0; // @todo

		VkBindVertexBufferIndirectCommandEXT vbic{};
		vbic.bufferAddress = 0; // @todo
		vbic.size = 0; // @todo
		vbic.stride = 0; // @todo

		// Command layouts
		// 
		// @todo: ib, vb, draw

		VkIndirectCommandsIndexBufferTokenEXT icib_token{};
		icib_token.mode = VK_INDIRECT_COMMANDS_INPUT_MODE_VULKAN_INDEX_BUFFER_EXT;

		VkIndirectCommandsTokenDataEXT ic_token_data_ib{};
		ic_token_data_ib.pIndexBuffer = &icib_token;

		VkIndirectCommandsVertexBufferTokenEXT icvbToken{};
		icvbToken.vertexBindingUnit = 0;

		VkIndirectCommandsTokenDataEXT tokenDataVertexBuffer{};
		tokenDataVertexBuffer.pVertexBuffer = &icvbToken;


		std::vector<VkIndirectCommandsLayoutTokenEXT> commandTokenLayouts(3);
		commandTokenLayouts[0] = {};
		commandTokenLayouts[0].sType = VK_STRUCTURE_TYPE_INDIRECT_COMMANDS_LAYOUT_TOKEN_EXT;
		commandTokenLayouts[0].offset = 0;
		commandTokenLayouts[0].type = VK_INDIRECT_COMMANDS_TOKEN_TYPE_INDEX_BUFFER_EXT;
		commandTokenLayouts[0].data = ic_token_data_ib;

		commandTokenLayouts[1] = {};
		commandTokenLayouts[1].sType = VK_STRUCTURE_TYPE_INDIRECT_COMMANDS_LAYOUT_TOKEN_EXT;
		commandTokenLayouts[1].offset = 0;
		commandTokenLayouts[1].type = VK_INDIRECT_COMMANDS_TOKEN_TYPE_VERTEX_BUFFER_EXT;
		commandTokenLayouts[1].data = tokenDataVertexBuffer;

		commandTokenLayouts[2] = {};
		commandTokenLayouts[2].sType = VK_STRUCTURE_TYPE_INDIRECT_COMMANDS_LAYOUT_TOKEN_EXT;
		commandTokenLayouts[2].offset = 0;
		commandTokenLayouts[2].type = VK_INDIRECT_COMMANDS_TOKEN_TYPE_DRAW_INDEXED_EXT;

		VkIndirectCommandsLayoutCreateInfoEXT icLayoutCI{};
		icLayoutCI.sType = VK_STRUCTURE_TYPE_INDIRECT_COMMANDS_LAYOUT_CREATE_INFO_EXT;
		icLayoutCI.flags = VK_INDIRECT_COMMANDS_LAYOUT_USAGE_EXPLICIT_PREPROCESS_BIT_EXT;
		icLayoutCI.shaderStages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		icLayoutCI.pipelineLayout = VK_NULL_HANDLE; // @todo: require when doing push constants or sequence index
		icLayoutCI.tokenCount = static_cast<uint32_t>(commandTokenLayouts.size());
		icLayoutCI.pTokens = commandTokenLayouts.data();

		VkIndirectCommandsLayoutEXT icLayout{};

		VK_CHECK_RESULT(vkCreateIndirectCommandsLayoutEXT(device, &icLayoutCI, nullptr, &icLayout));
	}

	void prepare()
	{
		VulkanExampleBase::prepare();

		vkCreateIndirectExecutionSetEXT = reinterpret_cast<PFN_vkCreateIndirectExecutionSetEXT>(vkGetDeviceProcAddr(device, "vkCreateIndirectExecutionSetEXT"));
		vkDestroyIndirectExecutionSetEXT = reinterpret_cast<PFN_vkDestroyIndirectExecutionSetEXT>(vkGetDeviceProcAddr(device, "vkDestroyIndirectExecutionSetEXT"));
		vkCreateIndirectCommandsLayoutEXT = reinterpret_cast<PFN_vkCreateIndirectCommandsLayoutEXT>(vkGetDeviceProcAddr(device, "vkCreateIndirectCommandsLayoutEXT"));
		vkDestroyIndirectCommandsLayoutEXT = reinterpret_cast<PFN_vkDestroyIndirectCommandsLayoutEXT>(vkGetDeviceProcAddr(device, "vkDestroyIndirectCommandsLayoutEXT"));

		loadAssets();
		prepareUniformBuffers();
		setupDescriptors();
		preparePipelines();
		prepareIndirectExecutionBuffer();
		buildCommandBuffers();
		prepared = true;
	}

	void draw()
	{
		VulkanExampleBase::prepareFrame();
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
		VulkanExampleBase::submitFrame();
	}

	virtual void render()
	{
		if (!prepared) {
			return;
		}
		updateUniformBuffer();
		draw();
	}

	virtual void OnUpdateUIOverlay(vks::UIOverlay *overlay)
	{
		// @todo
	}
};

VULKAN_EXAMPLE_MAIN()
