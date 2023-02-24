/*
* Vulkan Example - Using dynamic state
*
* Copyright (C) 2022-2023 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "vulkanexamplebase.h"
#include "VulkanglTFModel.h"

#define ENABLE_VALIDATION false

class VulkanExample: public VulkanExampleBase
{
public:
	vkglTF::Model scene;

	vks::Buffer uniformBuffer;

	// Same uniform buffer layout as shader
	struct UBOVS {
		glm::mat4 projection;
		glm::mat4 modelView;
		glm::vec4 lightPos = glm::vec4(0.0f, 2.0f, 1.0f, 0.0f);
	} uboVS;

	float clearColor[4] = { 0.0f, 0.0f, 0.2f, 1.0f };

	VkPipelineLayout pipelineLayout;
	VkDescriptorSet descriptorSet;
	VkDescriptorSetLayout descriptorSetLayout;

	VkPipeline pipeline;

	// This sample demonstrates different dynamic states, so we check and store what extension is available
	bool hasDynamicState = false;
	bool hasDynamicState2 = false;
	bool hasDynamicState3 = false;
	bool hasDynamicVertexState = false;

	VkPhysicalDeviceExtendedDynamicStateFeaturesEXT extendedDynamicStateFeaturesEXT{};
	VkPhysicalDeviceExtendedDynamicState2FeaturesEXT extendedDynamicState2FeaturesEXT{};
	VkPhysicalDeviceExtendedDynamicState3FeaturesEXT extendedDynamicState3FeaturesEXT{};

	// Function pointers for dynamic states used in this sample
	// VK_EXT_dynamic_stte
	PFN_vkCmdSetCullModeEXT vkCmdSetCullModeEXT = nullptr;
	PFN_vkCmdSetFrontFaceEXT vkCmdSetFrontFaceEXT = nullptr;
	PFN_vkCmdSetDepthTestEnableEXT vkCmdSetDepthTestEnableEXT = nullptr;
	PFN_vkCmdSetDepthWriteEnableEXT vkCmdSetDepthWriteEnableEXT = nullptr;
	// VK_EXT_dynamic_state_2
	PFN_vkCmdSetRasterizerDiscardEnable vkCmdSetRasterizerDiscardEnableEXT = nullptr;
	// VK_EXT_dynamic_state_3
	PFN_vkCmdSetColorBlendEnableEXT vkCmdSetColorBlendEnableEXT = nullptr;
	PFN_vkCmdSetColorBlendEquationEXT vkCmdSetColorBlendEquationEXT = nullptr;

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

	VulkanExample() : VulkanExampleBase(ENABLE_VALIDATION)
	{
		title = "Dynamic state";
		camera.type = Camera::CameraType::lookat;
		camera.setPosition(glm::vec3(0.0f, 0.0f, -10.5f));
		camera.setRotation(glm::vec3(-25.0f, 15.0f, 0.0f));
		camera.setRotationSpeed(0.5f);
		camera.setPerspective(60.0f, (float)width / (float)height, 0.1f, 256.0f);
	}

	~VulkanExample()
	{
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

		uniformBuffer.destroy();
	}

	void buildCommandBuffers()
	{
		VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

		VkClearValue clearValues[2];
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

		for (int32_t i = 0; i < drawCmdBuffers.size(); ++i)
		{
			// Set target frame buffer
			renderPassBeginInfo.framebuffer = frameBuffers[i];

			VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo));

			vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
			vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

			VkRect2D scissor = vks::initializers::rect2D(width, height,	0, 0);
			vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

			// Apply dynamic states

			if (vkCmdSetCullModeEXT) {
				vkCmdSetCullModeEXT(drawCmdBuffers[i], VkCullModeFlagBits(dynamicState.cullMode));
			}
			if (vkCmdSetFrontFaceEXT) {
				vkCmdSetFrontFaceEXT(drawCmdBuffers[i], VkFrontFace(dynamicState.frontFace));
			}
			if (vkCmdSetDepthTestEnableEXT) {
				vkCmdSetDepthTestEnableEXT(drawCmdBuffers[i], VkFrontFace(dynamicState.depthTest));
			}
			if (vkCmdSetDepthWriteEnableEXT) {
				vkCmdSetDepthWriteEnableEXT(drawCmdBuffers[i], VkFrontFace(dynamicState.depthWrite));
			}

			if (vkCmdSetRasterizerDiscardEnableEXT) {
				vkCmdSetRasterizerDiscardEnableEXT(drawCmdBuffers[i], VkBool32(dynamicState2.rasterizerDiscardEnable));
			}

			if (vkCmdSetColorBlendEnableEXT) {
				const std::vector<VkBool32> blendEnables = { dynamicState3.colorBlendEnable };
				vkCmdSetColorBlendEnableEXT(drawCmdBuffers[i], 0, 1, blendEnables.data());

				VkColorBlendEquationEXT colorBlendEquation{};

				if (dynamicState3.colorBlendEnable) {
					colorBlendEquation.colorBlendOp = VK_BLEND_OP_ADD;
					colorBlendEquation.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
					colorBlendEquation.dstColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
					colorBlendEquation.alphaBlendOp = VK_BLEND_OP_ADD;
					colorBlendEquation.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
					colorBlendEquation.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
				}

				vkCmdSetColorBlendEquationEXT(drawCmdBuffers[i], 0, 1, &colorBlendEquation);
			}

			vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);
			scene.bindBuffers(drawCmdBuffers[i]);

			vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
			scene.draw(drawCmdBuffers[i]);

			drawUI(drawCmdBuffers[i]);

			vkCmdEndRenderPass(drawCmdBuffers[i]);

			VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
		}
	}

	void loadAssets()
	{
		const uint32_t glTFLoadingFlags = vkglTF::FileLoadingFlags::PreTransformVertices | vkglTF::FileLoadingFlags::PreMultiplyVertexColors | vkglTF::FileLoadingFlags::FlipY;
		scene.loadFromFile(getAssetPath() + "models/treasure_smooth.gltf", vulkanDevice, queue, glTFLoadingFlags);
	}

	void setupDescriptorPool()
	{
		std::vector<VkDescriptorPoolSize> poolSizes =
		{
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1)
		};

		VkDescriptorPoolCreateInfo descriptorPoolInfo =
			vks::initializers::descriptorPoolCreateInfo(
				poolSizes.size(),
				poolSizes.data(),
				2);

		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));
	}

	void setupDescriptorSetLayout()
	{
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings =
		{
			// Binding 0 : Vertex shader uniform buffer
			vks::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				VK_SHADER_STAGE_VERTEX_BIT,
				0)
		};

		VkDescriptorSetLayoutCreateInfo descriptorLayout =
			vks::initializers::descriptorSetLayoutCreateInfo(
				setLayoutBindings.data(),
				setLayoutBindings.size());

		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayout));

		VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo =
			vks::initializers::pipelineLayoutCreateInfo(
				&descriptorSetLayout,
				1);

		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &pipelineLayout));
	}

	void setupDescriptorSet()
	{
		VkDescriptorSetAllocateInfo allocInfo =
			vks::initializers::descriptorSetAllocateInfo(
				descriptorPool,
				&descriptorSetLayout,
				1);

		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));

		std::vector<VkWriteDescriptorSet> writeDescriptorSets =
		{
			// Binding 0 : Vertex shader uniform buffer
			vks::initializers::writeDescriptorSet(
				descriptorSet,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				0,
				&uniformBuffer.descriptor)
		};

		vkUpdateDescriptorSets(device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);
	}

	void preparePipelines()
	{
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
		VkPipelineRasterizationStateCreateInfo rasterizationState = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
		VkPipelineColorBlendAttachmentState blendAttachmentState = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
		VkPipelineColorBlendStateCreateInfo colorBlendState = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
		VkPipelineDepthStencilStateCreateInfo depthStencilState = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
		VkPipelineViewportStateCreateInfo viewportState = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
		VkPipelineMultisampleStateCreateInfo multisampleState = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

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
		pipelineCI.stageCount = shaderStages.size();
		pipelineCI.pStages = shaderStages.data();
		pipelineCI.pVertexInputState  = vkglTF::Vertex::getPipelineVertexInputState({vkglTF::VertexComponent::Position, vkglTF::VertexComponent::Normal, vkglTF::VertexComponent::Color});

		// Create the graphics pipeline state objects

		// Textured pipeline
		// Phong shading pipeline
		shaderStages[0] = loadShader(getShadersPath() + "pipelines/phong.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getShadersPath() + "pipelines/phong.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipeline));
	}

	// Prepare and initialize uniform buffer containing shader uniforms
	void prepareUniformBuffers()
	{
		// Create the vertex shader uniform buffer block
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&uniformBuffer,
			sizeof(uboVS)));

		// Map persistent
		VK_CHECK_RESULT(uniformBuffer.map());

		updateUniformBuffers();
	}

	void updateUniformBuffers()
	{
		uboVS.projection = camera.matrices.perspective;
		uboVS.modelView = camera.matrices.view;
		memcpy(uniformBuffer.mapped, &uboVS, sizeof(uboVS));
	}

	void draw()
	{
		VulkanExampleBase::prepareFrame();

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

		VulkanExampleBase::submitFrame();
	}

	void getEnabledExtensions()
	{
		// Check what dynamic states are supported by the current implementation
		hasDynamicState = vulkanDevice->extensionSupported(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME);
		hasDynamicState2 = vulkanDevice->extensionSupported(VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME);
		hasDynamicState3 = vulkanDevice->extensionSupported(VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME);
		hasDynamicVertexState = vulkanDevice->extensionSupported(VK_EXT_VERTEX_INPUT_DYNAMIC_STATE_EXTENSION_NAME);

		// Enable dynamic state extensions if present. This function is called after physical and before logical device creation, so we can enabled extensions based on a list of supported extensions
		if (vulkanDevice->extensionSupported(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME)) {
			enabledDeviceExtensions.push_back(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME);
			extendedDynamicStateFeaturesEXT.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;
			extendedDynamicStateFeaturesEXT.extendedDynamicState = VK_TRUE;
			deviceCreatepNextChain = &extendedDynamicStateFeaturesEXT;
		}
		if (vulkanDevice->extensionSupported(VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME)) {
			enabledDeviceExtensions.push_back(VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME);
			extendedDynamicState2FeaturesEXT.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_2_FEATURES_EXT;
			extendedDynamicState2FeaturesEXT.extendedDynamicState2 = VK_TRUE;
			if (hasDynamicState) {
				extendedDynamicStateFeaturesEXT.pNext = &extendedDynamicState2FeaturesEXT;
			} else {
				deviceCreatepNextChain = &extendedDynamicState2FeaturesEXT;
			}
		}
		if (vulkanDevice->extensionSupported(VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME)) {
			enabledDeviceExtensions.push_back(VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME);
			extendedDynamicState3FeaturesEXT.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT;
			extendedDynamicState3FeaturesEXT.extendedDynamicState3ColorBlendEnable = VK_TRUE;
			extendedDynamicState3FeaturesEXT.extendedDynamicState3ColorBlendEquation = VK_TRUE;
			if (hasDynamicState2) {
				extendedDynamicState2FeaturesEXT.pNext = &extendedDynamicState3FeaturesEXT;
			} else {
				deviceCreatepNextChain = &extendedDynamicState3FeaturesEXT;
			}

		}
		if (vulkanDevice->extensionSupported(VK_EXT_VERTEX_INPUT_DYNAMIC_STATE_EXTENSION_NAME)) {
			enabledDeviceExtensions.push_back(VK_EXT_VERTEX_INPUT_DYNAMIC_STATE_EXTENSION_NAME);
		}
	}

	void prepare()
	{
		VulkanExampleBase::prepare();

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
		setupDescriptorSetLayout();
		preparePipelines();
		setupDescriptorPool();
		setupDescriptorSet();
		buildCommandBuffers();
		prepared = true;
	}

	virtual void render()
	{
		if (!prepared)
			return;
		draw();
		if (camera.updated) {
			updateUniformBuffers();
		}
	}

	virtual void viewChanged()
	{
		updateUniformBuffers();
	}

	virtual void OnUpdateUIOverlay(vks::UIOverlay *overlay)
	{
		bool rebuildCB = false;		
		if (overlay->header("Dynamic state")) {
			if (hasDynamicState) {
				rebuildCB = overlay->comboBox("Cull mode", &dynamicState.cullMode, { "none", "front", "back" });
				rebuildCB |= overlay->comboBox("Front face", &dynamicState.frontFace, { "Counter clockwise", "Clockwise" });
				rebuildCB |= overlay->checkBox("Depth test", &dynamicState.depthTest);
				rebuildCB |= overlay->checkBox("Depth write", &dynamicState.depthWrite);
			} else {
				overlay->text("Extension not supported");
			}
		}
		if (overlay->header("Dynamic state 2")) {
			if (hasDynamicState2) {
				rebuildCB |= overlay->checkBox("Rasterizer discard", &dynamicState2.rasterizerDiscardEnable);
			}
			else {
				overlay->text("Extension not supported");
			}
		}
		if (overlay->header("Dynamic state 3")) {
			if (hasDynamicState3) {
				rebuildCB |= overlay->checkBox("Color blend", &dynamicState3.colorBlendEnable);
				rebuildCB |= overlay->colorPicker("Clear color", clearColor);
			}
			else {
				overlay->text("Extension not supported");
			}
		}
		if (rebuildCB) {
			buildCommandBuffers();
		}
	}
};

VULKAN_EXAMPLE_MAIN()
