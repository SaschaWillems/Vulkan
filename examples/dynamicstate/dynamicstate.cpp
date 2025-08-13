/*
* Vulkan Example - Using dynamic state
* 
* This sample demonstrates the use of some of the VK_EXT_dynamic_state extensions
* These allow an application to set some pipeline related state dynamically at drawtime
* instead of having to pre-bake the state into a pipeline
* This can help reduce the number of pipelines required
*
* Copyright (C) 2022-2025 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "vulkanexamplebase.h"
#include "VulkanglTFModel.h"

class VulkanExample: public VulkanExampleBase
{
public:
	vkglTF::Model scene;

	struct UniformData {
		glm::mat4 projection;
		glm::mat4 modelView;
		glm::vec4 lightPos{ 0.0f, 2.0f, 1.0f, 0.0f };
	} uniformData;
	std::array<vks::Buffer, maxConcurrentFrames> uniformBuffers;

	float clearColor[4] = { 0.0f, 0.0f, 0.2f, 1.0f };

	VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };
	VkPipeline pipeline{ VK_NULL_HANDLE };
	VkDescriptorSetLayout descriptorSetLayout{ VK_NULL_HANDLE };
	std::array<VkDescriptorSet, maxConcurrentFrames> descriptorSets{};

	// This sample demonstrates different dynamic states, so we check and store what extension is available
	bool hasDynamicState{ false };
	bool hasDynamicState2{ false };
	bool hasDynamicState3{ false };
	bool hasDynamicVertexState{ false };

	VkPhysicalDeviceExtendedDynamicStateFeaturesEXT extendedDynamicStateFeaturesEXT{};
	VkPhysicalDeviceExtendedDynamicState2FeaturesEXT extendedDynamicState2FeaturesEXT{};
	VkPhysicalDeviceExtendedDynamicState3FeaturesEXT extendedDynamicState3FeaturesEXT{};

	// Function pointers for dynamic states used in this sample
	// VK_EXT_dynamic_stte
	PFN_vkCmdSetCullModeEXT vkCmdSetCullModeEXT{ nullptr };
	PFN_vkCmdSetFrontFaceEXT vkCmdSetFrontFaceEXT{ nullptr };
	PFN_vkCmdSetDepthTestEnableEXT vkCmdSetDepthTestEnableEXT{ nullptr };
	PFN_vkCmdSetDepthWriteEnableEXT vkCmdSetDepthWriteEnableEXT{ nullptr };
	// VK_EXT_dynamic_state_2
	PFN_vkCmdSetRasterizerDiscardEnable vkCmdSetRasterizerDiscardEnableEXT{ nullptr };
	// VK_EXT_dynamic_state_3
	PFN_vkCmdSetColorBlendEnableEXT vkCmdSetColorBlendEnableEXT{ nullptr };
	PFN_vkCmdSetColorBlendEquationEXT vkCmdSetColorBlendEquationEXT{ nullptr };

	// Dynamic state UI toggles
	struct DynamicState {
		int32_t cullMode = VK_CULL_MODE_BACK_BIT;
		int32_t frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		bool depthTest = true;
		bool depthWrite = true;
	} dynamicState;
	struct DynamicState2 {
		bool rasterizerDiscardEnable = false;
	} dynamicState2;
	struct DynamicState3 {
		bool colorBlendEnable = false;
	} dynamicState3;

	VulkanExample() : VulkanExampleBase()
	{
		title = "Dynamic state";
		camera.type = Camera::CameraType::lookat;
		camera.setPosition(glm::vec3(0.0f, 0.0f, -10.5f));
		camera.setRotation(glm::vec3(-25.0f, 15.0f, 0.0f));
		camera.setRotationSpeed(0.5f);
		camera.setPerspective(60.0f, (float)width / (float)height, 0.1f, 256.0f);
		
		// Note: We enable the dynamic state extensions dynamically, based on which ones the device supports see getEnabledExtensions
		enabledInstanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	}

	~VulkanExample()
	{
		if (device) {
			vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
			vkDestroyPipeline(device, pipeline, nullptr);
			vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
			for (auto& buffer : uniformBuffers) {
				buffer.destroy();
			}
		}
	}

	void getEnabledExtensions()
	{
		// Get the full list of extended dynamic state features supported by the device
		extendedDynamicStateFeaturesEXT.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;
		extendedDynamicStateFeaturesEXT.pNext = &extendedDynamicState2FeaturesEXT;
		extendedDynamicState2FeaturesEXT.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_2_FEATURES_EXT;
		extendedDynamicState2FeaturesEXT.pNext = &extendedDynamicState3FeaturesEXT;
		extendedDynamicState3FeaturesEXT.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT;
		extendedDynamicState3FeaturesEXT.pNext = nullptr;

		VkPhysicalDeviceFeatures2 physicalDeviceFeatures2;
		physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		physicalDeviceFeatures2.pNext = &extendedDynamicStateFeaturesEXT;
		vkGetPhysicalDeviceFeatures2(physicalDevice, &physicalDeviceFeatures2);

		// Check what dynamic states are supported by the current implementation
		// Checking for available features is probably sufficient, but retained redundant extension checks for clarity and consistency
		hasDynamicState = vulkanDevice->extensionSupported(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME) && extendedDynamicStateFeaturesEXT.extendedDynamicState;
		hasDynamicState2 = vulkanDevice->extensionSupported(VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME) && extendedDynamicState2FeaturesEXT.extendedDynamicState2;
		hasDynamicState3 = vulkanDevice->extensionSupported(VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME) && extendedDynamicState3FeaturesEXT.extendedDynamicState3ColorBlendEnable && extendedDynamicState3FeaturesEXT.extendedDynamicState3ColorBlendEquation;
		hasDynamicVertexState = vulkanDevice->extensionSupported(VK_EXT_VERTEX_INPUT_DYNAMIC_STATE_EXTENSION_NAME);

		// Enable dynamic state extensions if present. This function is called after physical and before logical device creation, so we can enabled extensions based on a list of supported extensions
		if (hasDynamicState) {
			enabledDeviceExtensions.push_back(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME);
			extendedDynamicStateFeaturesEXT.pNext = nullptr;
			deviceCreatepNextChain = &extendedDynamicStateFeaturesEXT;
		}
		if (hasDynamicState2) {
			enabledDeviceExtensions.push_back(VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME);
			extendedDynamicState2FeaturesEXT.pNext = nullptr;
			if (hasDynamicState) {
				extendedDynamicStateFeaturesEXT.pNext = &extendedDynamicState2FeaturesEXT;
			}
			else {
				deviceCreatepNextChain = &extendedDynamicState2FeaturesEXT;
			}
		}
		if (hasDynamicState3) {
			enabledDeviceExtensions.push_back(VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME);
			if (hasDynamicState2) {
				extendedDynamicState2FeaturesEXT.pNext = &extendedDynamicState3FeaturesEXT;
			}
			else {
				deviceCreatepNextChain = &extendedDynamicState3FeaturesEXT;
			}

		}
		if (hasDynamicVertexState) {
			enabledDeviceExtensions.push_back(VK_EXT_VERTEX_INPUT_DYNAMIC_STATE_EXTENSION_NAME);
		}
	}

	void loadAssets()
	{
		const uint32_t glTFLoadingFlags = vkglTF::FileLoadingFlags::PreTransformVertices | vkglTF::FileLoadingFlags::PreMultiplyVertexColors | vkglTF::FileLoadingFlags::FlipY;
		scene.loadFromFile(getAssetPath() + "models/treasure_smooth.gltf", vulkanDevice, queue, glTFLoadingFlags);
	}

	void setupDescriptors()
	{
		// Pool
		std::vector<VkDescriptorPoolSize> poolSizes = {
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, maxConcurrentFrames)
		};
		VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, maxConcurrentFrames);
		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));

		// Layout
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
			// Binding 0 : Vertex shader uniform buffer
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0)
		};
		VkDescriptorSetLayoutCreateInfo descriptorLayout = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayout));

		// Sets per frame, just like the buffers themselves
		VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);
		for (auto i = 0; i < uniformBuffers.size(); i++) {
			VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets[i]));
			std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
				vks::initializers::writeDescriptorSet(descriptorSets[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffers[i].descriptor),
			};
			vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
		}
	}

	void preparePipelines()
	{
		// Layout
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(&descriptorSetLayout, 1);
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

		// Pipeline
		// Instead of having to create a pipeline for each state combination, we only create one pipeline and toggle the new dynamic states during command buffer creation
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
		VkPipelineRasterizationStateCreateInfo rasterizationState = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
		VkPipelineColorBlendAttachmentState blendAttachmentState = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
		VkPipelineColorBlendStateCreateInfo colorBlendState = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
		VkPipelineDepthStencilStateCreateInfo depthStencilState = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
		VkPipelineViewportStateCreateInfo viewportState = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
		VkPipelineMultisampleStateCreateInfo multisampleState = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

		// All dynamic states we want to use need to be enabled at pipeline creation
		std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_LINE_WIDTH, };
		if (hasDynamicState) {
			dynamicStateEnables.push_back(VK_DYNAMIC_STATE_CULL_MODE_EXT);
			dynamicStateEnables.push_back(VK_DYNAMIC_STATE_FRONT_FACE_EXT);
			dynamicStateEnables.push_back(VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE_EXT);
			dynamicStateEnables.push_back(VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE_EXT);
		}
		if (hasDynamicState2) {
			dynamicStateEnables.push_back(VK_DYNAMIC_STATE_RASTERIZER_DISCARD_ENABLE_EXT);
		}
		if (hasDynamicState3) {
			dynamicStateEnables.push_back(VK_DYNAMIC_STATE_COLOR_BLEND_ENABLE_EXT);
			dynamicStateEnables.push_back(VK_DYNAMIC_STATE_COLOR_BLEND_EQUATION_EXT);
		}

		VkPipelineDynamicStateCreateInfo dynamicState = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);

		VkGraphicsPipelineCreateInfo pipelineCI = vks::initializers::pipelineCreateInfo(pipelineLayout, renderPass);
		pipelineCI.pInputAssemblyState = &inputAssemblyState;
		pipelineCI.pRasterizationState = &rasterizationState;
		pipelineCI.pColorBlendState = &colorBlendState;
		pipelineCI.pMultisampleState = &multisampleState;
		pipelineCI.pViewportState = &viewportState;
		pipelineCI.pDepthStencilState = &depthStencilState;
		pipelineCI.pDynamicState = &dynamicState;
		pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineCI.pStages = shaderStages.data();
		pipelineCI.pVertexInputState  = vkglTF::Vertex::getPipelineVertexInputState({vkglTF::VertexComponent::Position, vkglTF::VertexComponent::Normal, vkglTF::VertexComponent::Color});

		// Create the graphics pipeline state objects

		shaderStages[0] = loadShader(getShadersPath() + "pipelines/phong.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getShadersPath() + "pipelines/phong.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipeline));
	}

	// Prepare and initialize uniform buffer containing shader uniforms
	void prepareUniformBuffers()
	{
		for (auto& buffer : uniformBuffers) {
			VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &buffer, sizeof(UniformData), &uniformData));
			VK_CHECK_RESULT(buffer.map());
		}
	}

	void updateUniformBuffers()
	{
		uniformData.projection = camera.matrices.perspective;
		uniformData.modelView = camera.matrices.view;
		memcpy(uniformBuffers[currentBuffer].mapped, &uniformData, sizeof(uniformData));
	}

	void prepare()
	{
		VulkanExampleBase::prepare();

		// Dynamic states are set with vkCmd* calls in the command buffer, so we need to load the function pointers depending on extension supports
		if (hasDynamicState) {
			vkCmdSetCullModeEXT = reinterpret_cast<PFN_vkCmdSetCullModeEXT>(vkGetDeviceProcAddr(device, "vkCmdSetCullModeEXT"));
			vkCmdSetFrontFaceEXT = reinterpret_cast<PFN_vkCmdSetFrontFaceEXT>(vkGetDeviceProcAddr(device, "vkCmdSetFrontFaceEXT"));
			vkCmdSetDepthWriteEnableEXT = reinterpret_cast<PFN_vkCmdSetDepthWriteEnableEXT>(vkGetDeviceProcAddr(device, "vkCmdSetDepthWriteEnableEXT"));
			vkCmdSetDepthTestEnableEXT = reinterpret_cast<PFN_vkCmdSetDepthTestEnable>(vkGetDeviceProcAddr(device, "vkCmdSetDepthTestEnableEXT"));
		}

		if (hasDynamicState2) {
			vkCmdSetRasterizerDiscardEnableEXT = reinterpret_cast<PFN_vkCmdSetRasterizerDiscardEnableEXT>(vkGetDeviceProcAddr(device, "vkCmdSetRasterizerDiscardEnableEXT"));
		}

		if (hasDynamicState3) {
			vkCmdSetColorBlendEnableEXT = reinterpret_cast<PFN_vkCmdSetColorBlendEnableEXT>(vkGetDeviceProcAddr(device, "vkCmdSetColorBlendEnableEXT"));
			vkCmdSetColorBlendEquationEXT = reinterpret_cast<PFN_vkCmdSetColorBlendEquationEXT>(vkGetDeviceProcAddr(device, "vkCmdSetColorBlendEquationEXT"));
		}

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
		clearValues[0].color = { { clearColor[0], clearColor[1], clearColor[2], clearColor[3] } };
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

		vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
		vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

		VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);
		vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

		// Apply dynamic states

		if (vkCmdSetCullModeEXT) {
			vkCmdSetCullModeEXT(cmdBuffer, VkCullModeFlagBits(dynamicState.cullMode));
		}
		if (vkCmdSetFrontFaceEXT) {
			vkCmdSetFrontFaceEXT(cmdBuffer, VkFrontFace(dynamicState.frontFace));
		}
		if (vkCmdSetDepthTestEnableEXT) {
			vkCmdSetDepthTestEnableEXT(cmdBuffer, VkFrontFace(dynamicState.depthTest));
		}
		if (vkCmdSetDepthWriteEnableEXT) {
			vkCmdSetDepthWriteEnableEXT(cmdBuffer, VkFrontFace(dynamicState.depthWrite));
		}

		if (vkCmdSetRasterizerDiscardEnableEXT) {
			vkCmdSetRasterizerDiscardEnableEXT(cmdBuffer, VkBool32(dynamicState2.rasterizerDiscardEnable));
		}

		if (vkCmdSetColorBlendEnableEXT) {
			const std::vector<VkBool32> blendEnables = { dynamicState3.colorBlendEnable };
			vkCmdSetColorBlendEnableEXT(cmdBuffer, 0, 1, blendEnables.data());

			VkColorBlendEquationEXT colorBlendEquation{};

			if (dynamicState3.colorBlendEnable) {
				colorBlendEquation.colorBlendOp = VK_BLEND_OP_ADD;
				colorBlendEquation.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
				colorBlendEquation.dstColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
				colorBlendEquation.alphaBlendOp = VK_BLEND_OP_ADD;
				colorBlendEquation.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
				colorBlendEquation.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			}

			vkCmdSetColorBlendEquationEXT(cmdBuffer, 0, 1, &colorBlendEquation);
		}

		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentBuffer], 0, nullptr);
		scene.bindBuffers(cmdBuffer);

		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		scene.draw(cmdBuffer);

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

	virtual void OnUpdateUIOverlay(vks::UIOverlay *overlay)
	{
		if (overlay->header("Dynamic state")) {
			if (hasDynamicState) {
				overlay->comboBox("Cull mode", &dynamicState.cullMode, { "none", "front", "back" });
				overlay->comboBox("Front face", &dynamicState.frontFace, { "Counter clockwise", "Clockwise" });
				overlay->checkBox("Depth test", &dynamicState.depthTest);
				overlay->checkBox("Depth write", &dynamicState.depthWrite);
			} else {
				overlay->text("Extension or features not supported");
			}
		}
		if (overlay->header("Dynamic state 2")) {
			if (hasDynamicState2) {
				overlay->checkBox("Rasterizer discard", &dynamicState2.rasterizerDiscardEnable);
			}
			else {
				overlay->text("Extension or features not supported");
			}
		}
		if (overlay->header("Dynamic state 3")) {
			if (hasDynamicState3) {
				overlay->checkBox("Color blend", &dynamicState3.colorBlendEnable);
				overlay->colorPicker("Clear color", clearColor);
			}
			else {
				overlay->text("Extension or features not supported");
			}
		}
	}
};

VULKAN_EXAMPLE_MAIN()
