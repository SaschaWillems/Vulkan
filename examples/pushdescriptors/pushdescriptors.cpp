/*
* Vulkan Example - Push descriptors
*
* Note: Requires a device that supports the VK_KHR_push_descriptor extension
*
* Push descriptors apply the push constants concept to descriptor sets. So instead of creating
* per-model descriptor sets (along with a pool for each descriptor type) for rendering multiple objects,
* this example uses push descriptors to pass descriptor sets for per-model textures and matrices
* at command buffer creation time.
*
* Copyright (C) 2018-2023 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "vulkanexamplebase.h"
#include "VulkanglTFModel.h"

class VulkanExample : public VulkanExampleBase
{
public:
	bool animate = true;

	PFN_vkCmdPushDescriptorSetKHR vkCmdPushDescriptorSetKHR{ VK_NULL_HANDLE };
	VkPhysicalDevicePushDescriptorPropertiesKHR pushDescriptorProps{};

	struct Cube {
		vks::Texture2D texture;
		vks::Buffer uniformBuffer;
		glm::vec3 rotation;
		glm::mat4 modelMat;
	};
	std::array<Cube, 2> cubes;

	vkglTF::Model model;

	struct UniformData {
		glm::mat4 projection;
		glm::mat4 view;
	} uniformData;
	vks::Buffer uniformBuffer;

	VkPipeline pipeline{ VK_NULL_HANDLE };
	VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };
	VkDescriptorSetLayout descriptorSetLayout{ VK_NULL_HANDLE };

	VulkanExample() : VulkanExampleBase()
	{
		title = "Push descriptors";
		camera.type = Camera::CameraType::lookat;
		camera.setPerspective(60.0f, (float)width / (float)height, 0.1f, 512.0f);
		camera.setRotation(glm::vec3(0.0f, 0.0f, 0.0f));
		camera.setTranslation(glm::vec3(0.0f, 0.0f, -5.0f));
		// Enable extension required for push descriptors
		enabledInstanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
		enabledDeviceExtensions.push_back(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);
	}

	~VulkanExample()
	{
		if (device) {
			vkDestroyPipeline(device, pipeline, nullptr);
			vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
			vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
			for (auto cube : cubes) {
				cube.uniformBuffer.destroy();
				cube.texture.destroy();
			}
			uniformBuffer.destroy();
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

			model.bindBuffers(drawCmdBuffers[i]);

			// Render two cubes using different descriptor sets using push descriptors
			for (const auto& cube : cubes) {

				// Instead of preparing the descriptor sets up-front, using push descriptors we can set (push) them inside of a command buffer
				// This allows a more dynamic approach without the need to create descriptor sets for each model
				// Note: dstSet for each descriptor set write is left at zero as this is ignored when using push descriptors

				std::array<VkWriteDescriptorSet, 3> writeDescriptorSets{};

				// Scene matrices
				writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptorSets[0].dstSet = 0;
				writeDescriptorSets[0].dstBinding = 0;
				writeDescriptorSets[0].descriptorCount = 1;
				writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				writeDescriptorSets[0].pBufferInfo = &uniformBuffer.descriptor;

				// Model matrices
				writeDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptorSets[1].dstSet = 0;
				writeDescriptorSets[1].dstBinding = 1;
				writeDescriptorSets[1].descriptorCount = 1;
				writeDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				writeDescriptorSets[1].pBufferInfo = &cube.uniformBuffer.descriptor;

				// Texture
				writeDescriptorSets[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptorSets[2].dstSet = 0;
				writeDescriptorSets[2].dstBinding = 2;
				writeDescriptorSets[2].descriptorCount = 1;
				writeDescriptorSets[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				writeDescriptorSets[2].pImageInfo = &cube.texture.descriptor;

				vkCmdPushDescriptorSetKHR(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 3, writeDescriptorSets.data());

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
		cubes[0].texture.loadFromFile(getAssetPath() + "textures/crate01_color_height_rgba.ktx", VK_FORMAT_R8G8B8A8_UNORM, vulkanDevice, queue);
		cubes[1].texture.loadFromFile(getAssetPath() + "textures/crate02_color_height_rgba.ktx", VK_FORMAT_R8G8B8A8_UNORM, vulkanDevice, queue);
	}

	void setupDescriptorSetLayout()
	{
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1),
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2),
		};

		VkDescriptorSetLayoutCreateInfo descriptorLayoutCI{};
		descriptorLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		// Setting this flag tells the descriptor set layouts that no actual descriptor sets are allocated but instead pushed at command buffer creation time
		descriptorLayoutCI.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
		descriptorLayoutCI.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
		descriptorLayoutCI.pBindings = setLayoutBindings.data();
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayoutCI, nullptr, &descriptorSetLayout));

	}

	void preparePipelines()
	{
		// Layout
		VkPipelineLayoutCreateInfo pipelineLayoutCI = vks::initializers::pipelineLayoutCreateInfo(&descriptorSetLayout, 1);
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &pipelineLayout));

		// Pipeline
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
		VkPipelineRasterizationStateCreateInfo rasterizationStateCI = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
		VkPipelineColorBlendAttachmentState blendAttachmentState = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
		VkPipelineColorBlendStateCreateInfo colorBlendStateCI = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
		VkPipelineDepthStencilStateCreateInfo depthStencilStateCI = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
		VkPipelineViewportStateCreateInfo viewportStateCI = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
		VkPipelineMultisampleStateCreateInfo multisampleStateCI = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
		const std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicStateCI = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

		VkGraphicsPipelineCreateInfo pipelineCI  = vks::initializers::pipelineCreateInfo(pipelineLayout, renderPass, 0);
		pipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
		pipelineCI.pRasterizationState = &rasterizationStateCI;
		pipelineCI.pColorBlendState = &colorBlendStateCI;
		pipelineCI.pMultisampleState = &multisampleStateCI;
		pipelineCI.pViewportState = &viewportStateCI;
		pipelineCI.pDepthStencilState = &depthStencilStateCI;
		pipelineCI.pDynamicState = &dynamicStateCI;
		pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineCI.pStages = shaderStages.data();
		pipelineCI.pVertexInputState = vkglTF::Vertex::getPipelineVertexInputState({vkglTF::VertexComponent::Position, vkglTF::VertexComponent::Normal, vkglTF::VertexComponent::UV, vkglTF::VertexComponent::Color});

		shaderStages[0] = loadShader(getShadersPath() + "pushdescriptors/cube.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getShadersPath() + "pushdescriptors/cube.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipeline));
	}

	void prepareUniformBuffers()
	{
		// Vertex shader scene uniform buffer block
		VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &uniformBuffer, sizeof(UniformData)));
		VK_CHECK_RESULT(uniformBuffer.map());

		// Vertex shader cube model uniform buffer blocks
		for (auto& cube : cubes) {
			VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &cube.uniformBuffer, sizeof(glm::mat4)));
			VK_CHECK_RESULT(cube.uniformBuffer.map());
		}

		updateUniformBuffers();
		updateCubeUniformBuffers();
	}

	void updateUniformBuffers()
	{
		uniformData.projection = camera.matrices.perspective;
		uniformData.view = camera.matrices.view;
		memcpy(uniformBuffer.mapped, &uniformData, sizeof(uniformData));
	}

	void updateCubeUniformBuffers()
	{
		cubes[0].modelMat = glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, 0.0f, 0.0f));
		cubes[1].modelMat = glm::translate(glm::mat4(1.0f), glm::vec3( 1.5f, 0.5f, 0.0f));

		for (auto& cube : cubes) {
			cube.modelMat = glm::rotate(cube.modelMat, glm::radians(cube.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
			cube.modelMat = glm::rotate(cube.modelMat, glm::radians(cube.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
			cube.modelMat = glm::rotate(cube.modelMat, glm::radians(cube.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
			cube.modelMat = glm::scale(cube.modelMat, glm::vec3(0.25f));
			memcpy(cube.uniformBuffer.mapped, &cube.modelMat, sizeof(glm::mat4));
		}

		if (animate && !paused) {
			cubes[0].rotation.x += 2.5f * frameTimer;
			if (cubes[0].rotation.x > 360.0f)
				cubes[0].rotation.x -= 360.0f;
			cubes[1].rotation.y += 2.0f * frameTimer;
			if (cubes[1].rotation.y > 360.0f)
				cubes[1].rotation.y -= 360.0f;
		}
	}

	void prepare()
	{
		VulkanExampleBase::prepare();

		/*
			Extension specific functions
		*/

		// The push descriptor update function is part of an extension so it has to be manually loaded
		vkCmdPushDescriptorSetKHR = (PFN_vkCmdPushDescriptorSetKHR)vkGetDeviceProcAddr(device, "vkCmdPushDescriptorSetKHR");
		if (!vkCmdPushDescriptorSetKHR) {
			vks::tools::exitFatal("Could not get a valid function pointer for vkCmdPushDescriptorSetKHR", -1);
		}

		// Get device push descriptor properties (to display them)
		PFN_vkGetPhysicalDeviceProperties2KHR vkGetPhysicalDeviceProperties2KHR = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2KHR>(vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceProperties2KHR"));
		if (!vkGetPhysicalDeviceProperties2KHR) {
			vks::tools::exitFatal("Could not get a valid function pointer for vkGetPhysicalDeviceProperties2KHR", -1);
		}
		VkPhysicalDeviceProperties2KHR deviceProps2{};
		pushDescriptorProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PUSH_DESCRIPTOR_PROPERTIES_KHR;
		deviceProps2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;
		deviceProps2.pNext = &pushDescriptorProps;
		vkGetPhysicalDeviceProperties2KHR(physicalDevice, &deviceProps2);

		/*
			End of extension specific functions
		*/

		loadAssets();
		prepareUniformBuffers();
		setupDescriptorSetLayout();
		preparePipelines();
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
		if (!prepared)
			return;
		updateUniformBuffers();
		if (animate && !paused) {
			updateCubeUniformBuffers();
		}
		draw();
	}

	virtual void OnUpdateUIOverlay(vks::UIOverlay *overlay)
	{
		if (overlay->header("Settings")) {
			overlay->checkBox("Animate", &animate);
		}
		if (overlay->header("Device properties")) {
			overlay->text("maxPushDescriptors: %d", pushDescriptorProps.maxPushDescriptors);
		}
	}
};

VULKAN_EXAMPLE_MAIN()
