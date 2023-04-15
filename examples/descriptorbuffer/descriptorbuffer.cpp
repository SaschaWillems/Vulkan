/*
 * Vulkan Example - Using descriptor buffers via VK_EXT_descriptor_buffer
 *
 * Copyright (C) 2022-2023 by Sascha Willems - www.saschawillems.de
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */

#include "vulkanexamplebase.h"
#include "VulkanglTFModel.h"

#define ENABLE_VALIDATION false

class VulkanExample : public VulkanExampleBase
{
public:
	bool animate = true;

	struct Cube {
		glm::mat4 matrix;
		vks::Texture2D texture;
		vks::Buffer uniformBuffer;
		glm::vec3 rotation;
	};
	std::array<Cube, 2> cubes;

	vks::Buffer uniformBufferCamera;

	vkglTF::Model model;

	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;

	VkDescriptorSetLayout descriptorSetLayoutBuffers;
	VkDescriptorSetLayout descriptorSetLayoutImages;

	vks::Buffer resourceDescriptorBuffer;
	vks::Buffer imageDescriptorBuffer;
	VkDeviceOrHostAddressConstKHR resourceDescriptorBufferDeviceAddress;
	VkDeviceOrHostAddressConstKHR imageDescriptorBufferDeviceAddress;

	PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR;

	VkPhysicalDeviceDescriptorBufferFeaturesEXT enabledDeviceDescriptorBufferFeaturesEXT{};
	VkPhysicalDeviceBufferDeviceAddressFeatures enabledBufferDeviceAddresFeatures{};
	VkPhysicalDeviceDescriptorBufferPropertiesEXT descriptorBufferProperties{};

	PFN_vkGetDescriptorSetLayoutSizeEXT vkGetDescriptorSetLayoutSizeEXT;
	PFN_vkGetDescriptorSetLayoutBindingOffsetEXT vkGetDescriptorSetLayoutBindingOffsetEXT;
	PFN_vkCmdBindDescriptorBuffersEXT vkCmdBindDescriptorBuffersEXT;
	PFN_vkCmdSetDescriptorBufferOffsetsEXT vkCmdSetDescriptorBufferOffsetsEXT;
	PFN_vkGetDescriptorEXT vkGetDescriptorEXT;
	PFN_vkCmdBindDescriptorBufferEmbeddedSamplersEXT vkCmdBindDescriptorBufferEmbeddedSamplersEXT;

	VkDeviceSize uniformDescriptorOffset;
	VkDeviceSize imageDescriptorOffset;

	uint64_t getBufferDeviceAddress(VkBuffer buffer)
	{
		VkBufferDeviceAddressInfoKHR bufferDeviceAI{};
		bufferDeviceAI.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		bufferDeviceAI.buffer = buffer;
		return vkGetBufferDeviceAddressKHR(vulkanDevice->logicalDevice, &bufferDeviceAI);
	}

	VulkanExample() : VulkanExampleBase(ENABLE_VALIDATION)
	{
		title = "Descriptor buffers (VK_EXT_descriptor_buffer)";
		camera.type = Camera::CameraType::lookat;
		camera.setPerspective(60.0f, (float)width / (float)height, 0.1f, 512.0f);
		camera.setRotation(glm::vec3(0.0f, 0.0f, 0.0f));
		camera.setTranslation(glm::vec3(0.0f, 0.0f, -5.0f));

		apiVersion = VK_API_VERSION_1_1;

		enabledInstanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
		
		enabledDeviceExtensions.push_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
		enabledDeviceExtensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
		enabledDeviceExtensions.push_back(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);
		enabledDeviceExtensions.push_back(VK_KHR_MAINTENANCE3_EXTENSION_NAME);

		enabledDeviceExtensions.push_back(VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME);

		enabledBufferDeviceAddresFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
		enabledBufferDeviceAddresFeatures.bufferDeviceAddress = VK_TRUE;

		enabledDeviceDescriptorBufferFeaturesEXT.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT;
		enabledDeviceDescriptorBufferFeaturesEXT.descriptorBuffer = VK_TRUE;
		enabledDeviceDescriptorBufferFeaturesEXT.pNext = &enabledBufferDeviceAddresFeatures;
		
		deviceCreatepNextChain = &enabledDeviceDescriptorBufferFeaturesEXT;
	}

	~VulkanExample()
	{
		vkDestroyDescriptorSetLayout(device, descriptorSetLayoutBuffers, nullptr);
		vkDestroyDescriptorSetLayout(device, descriptorSetLayoutImages, nullptr);
		vkDestroyPipeline(device, pipeline, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		for (auto cube : cubes) {
			cube.uniformBuffer.destroy();
			cube.texture.destroy();
		}
		uniformBufferCamera.destroy();
		resourceDescriptorBuffer.destroy();
		imageDescriptorBuffer.destroy();
	}

	virtual void getEnabledFeatures()
	{
		if (deviceFeatures.samplerAnisotropy) {
			enabledFeatures.samplerAnisotropy = VK_TRUE;
		};
	}

	void setupDescriptors()
	{
		VkDescriptorSetLayoutCreateInfo descriptorLayoutCI{};
		descriptorLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorLayoutCI.bindingCount = 1;
		descriptorLayoutCI.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;

		VkDescriptorSetLayoutBinding setLayoutBinding = {};

		setLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		setLayoutBinding.binding = 0;
		setLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		setLayoutBinding.descriptorCount = 1;

		descriptorLayoutCI.pBindings = &setLayoutBinding;
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayoutCI, nullptr, &descriptorSetLayoutBuffers));

		setLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		setLayoutBinding.binding = 0;
		setLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		setLayoutBinding.descriptorCount = 1;

		descriptorLayoutCI.pBindings = &setLayoutBinding;
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayoutCI, nullptr, &descriptorSetLayoutImages));
	}

	void preparePipelines()
	{
		// Set 0 = Camera UBO
		// Set 1 = Model UBO
		// Set 2 = Model image
		const std::array<VkDescriptorSetLayout, 3> setLayouts = { descriptorSetLayoutBuffers, descriptorSetLayoutBuffers, descriptorSetLayoutImages };

		VkPipelineLayoutCreateInfo pipelineLayoutCI{};
		pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		// The pipeline layout is based on the descriptor set layout we created above
		pipelineLayoutCI.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
		pipelineLayoutCI.pSetLayouts = setLayouts.data();
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
		pipelineCI.flags = VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;

		shaderStages[0] = loadShader(getShadersPath() + "descriptorbuffer/cube.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getShadersPath() + "descriptorbuffer/cube.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipeline));
	}

	void prepareDescriptorBuffer()
	{
		std::array<VkDeviceSize, 2> descriptorLayoutSizes{};
		vkGetDescriptorSetLayoutSizeEXT(device, descriptorSetLayoutBuffers, &descriptorLayoutSizes[0]);
		vkGetDescriptorSetLayoutSizeEXT(device, descriptorSetLayoutImages, &descriptorLayoutSizes[1]);

		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&resourceDescriptorBuffer,
			3 * descriptorLayoutSizes[0]));
		resourceDescriptorBuffer.map();

		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, // Flags 1 & 2 are required for combined images
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&imageDescriptorBuffer,
			2 * descriptorLayoutSizes[1]));
		imageDescriptorBuffer.map();

		resourceDescriptorBufferDeviceAddress.deviceAddress = getBufferDeviceAddress(resourceDescriptorBuffer.buffer);
		imageDescriptorBufferDeviceAddress.deviceAddress = getBufferDeviceAddress(imageDescriptorBuffer.buffer);

		PFN_vkGetPhysicalDeviceProperties2KHR vkGetPhysicalDeviceProperties2KHR = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2KHR>(vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceProperties2KHR"));
		assert(vkGetPhysicalDeviceProperties2KHR);
		VkPhysicalDeviceProperties2KHR deviceProps2{};
		descriptorBufferProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT;
		deviceProps2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;
		deviceProps2.pNext = &descriptorBufferProperties;
		vkGetPhysicalDeviceProperties2KHR(physicalDevice, &deviceProps2);

		// We need buffer offsets in some places, which are implementation depenendend, so we get them from the decsriptor buffer properties structure
		uniformDescriptorOffset = vks::tools::alignedSize(descriptorBufferProperties.uniformBufferDescriptorSize, descriptorBufferProperties.descriptorBufferOffsetAlignment);
		imageDescriptorOffset = vks::tools::alignedSize(descriptorBufferProperties.combinedImageSamplerDescriptorSize, descriptorBufferProperties.descriptorBufferOffsetAlignment);

		VkDescriptorGetInfoEXT desc_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT };

		// Set descriptors for images
		char* imageDescriptorBufPtr = (char*)imageDescriptorBuffer.mapped;
		desc_info.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		for (uint32_t i = 0; i < static_cast<uint32_t>(cubes.size()); i++) {
			desc_info.data.pCombinedImageSampler = &cubes[i].texture.descriptor;
			vkGetDescriptorEXT(device, &desc_info, descriptorBufferProperties.combinedImageSamplerDescriptorSize, imageDescriptorBufPtr + i * imageDescriptorOffset);
		}

		// For uniform buffers we only need buffer device addresses
		// Global uniform buffer
		char* uniformDescriptorBufPtr = (char*)resourceDescriptorBuffer.mapped;

		VkDescriptorAddressInfoEXT descriptorAddressInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT };
		descriptorAddressInfo.address = getBufferDeviceAddress(uniformBufferCamera.buffer);
		descriptorAddressInfo.range = uniformBufferCamera.size;
		descriptorAddressInfo.format = VK_FORMAT_UNDEFINED;

		desc_info.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		desc_info.data.pCombinedImageSampler = nullptr;
		desc_info.data.pUniformBuffer = &descriptorAddressInfo;
		vkGetDescriptorEXT(device, &desc_info, descriptorBufferProperties.uniformBufferDescriptorSize, uniformDescriptorBufPtr);

		// Per-model uniform buffers
		for (uint32_t i = 0; i < static_cast<uint32_t>(cubes.size()); i++) {
			VkDescriptorAddressInfoEXT addr_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT };
			addr_info.address = getBufferDeviceAddress(cubes[i].uniformBuffer.buffer);
			addr_info.range = cubes[i].uniformBuffer.size;
			addr_info.format = VK_FORMAT_UNDEFINED;

			desc_info.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			desc_info.data.pCombinedImageSampler = nullptr;
			desc_info.data.pUniformBuffer = &addr_info;
			vkGetDescriptorEXT(device, &desc_info, descriptorBufferProperties.uniformBufferDescriptorSize, uniformDescriptorBufPtr + (i + 1) * uniformDescriptorOffset);
		}
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

			VkDeviceSize offsets[1] = { 0 };
			model.bindBuffers(drawCmdBuffers[i]);

			// Descriptor buffer bindings
			// Set 0 = uniform buffer
			VkDescriptorBufferBindingInfoEXT bindingInfos[2]{};
			bindingInfos[0].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT;
			bindingInfos[0].address = resourceDescriptorBufferDeviceAddress.deviceAddress;
			bindingInfos[0].usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;// | VK_BUFFER_USAGE_PUSH_DESCRIPTORS_DESCRIPTOR_BUFFER_BIT_EXT;
			// Set 1 = Image
			bindingInfos[1].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT;
			bindingInfos[1].pNext = nullptr;
			bindingInfos[1].address = imageDescriptorBufferDeviceAddress.deviceAddress;
			bindingInfos[1].usage = VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;
			vkCmdBindDescriptorBuffersEXT(drawCmdBuffers[i], 2, bindingInfos);

			uint32_t bufferIndexUbo = 0;
			VkDeviceSize bufferOffset = 0;

			// Global Matrices (set 0)
			bufferOffset = 0;
			vkCmdSetDescriptorBufferOffsetsEXT(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &bufferIndexUbo, &bufferOffset);

			// Set and offset into descriptor for each model
			for (uint32_t j = 0; j < static_cast<uint32_t>(cubes.size()); j++) {
				// Uniform buffer (set 1)
				// Model ubos start at offset * 1 (slot 0 is global matrices)
				bufferOffset = (j + 1) * uniformDescriptorOffset;
				vkCmdSetDescriptorBufferOffsetsEXT(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &bufferIndexUbo, &bufferOffset);
				// Image (set 2)
				uint32_t bufferIndexImage = 1;
				bufferOffset = j * imageDescriptorOffset;
				vkCmdSetDescriptorBufferOffsetsEXT(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 2, 1, &bufferIndexImage, &bufferOffset);
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

	void prepareUniformBuffers()
	{
		// UBO for camera matrices
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&uniformBufferCamera,
			sizeof(glm::mat4) * 2));
		VK_CHECK_RESULT(uniformBufferCamera.map());

		// UBOs for model matrices
		for (uint32_t i = 0; i < static_cast<uint32_t>(cubes.size()); i++) {
			VK_CHECK_RESULT(vulkanDevice->createBuffer(
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				&cubes[i].uniformBuffer,
				sizeof(glm::mat4)));
			VK_CHECK_RESULT(cubes[i].uniformBuffer.map());
		}
		updateUniformBuffers();
	}

	void updateUniformBuffers()
	{
		memcpy(uniformBufferCamera.mapped, &camera.matrices.perspective, sizeof(glm::mat4));
		memcpy((char*)uniformBufferCamera.mapped + sizeof(glm::mat4), &camera.matrices.view, sizeof(glm::mat4));

		cubes[0].matrix = glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, 0.0f, 0.0f));
		cubes[1].matrix = glm::translate(glm::mat4(1.0f), glm::vec3( 1.5f, 0.5f, 0.0f));

		for (auto& cube : cubes) {
			cube.matrix = glm::rotate(cube.matrix, glm::radians(cube.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
			cube.matrix = glm::rotate(cube.matrix, glm::radians(cube.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
			cube.matrix = glm::rotate(cube.matrix, glm::radians(cube.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
			cube.matrix = glm::scale(cube.matrix, glm::vec3(0.25f));
			memcpy(cube.uniformBuffer.mapped, &cube.matrix, sizeof(glm::mat4));
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

		vkGetBufferDeviceAddressKHR = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(vkGetDeviceProcAddr(device, "vkGetBufferDeviceAddressKHR"));

		vkGetDescriptorSetLayoutSizeEXT = reinterpret_cast<PFN_vkGetDescriptorSetLayoutSizeEXT>(vkGetDeviceProcAddr(device, "vkGetDescriptorSetLayoutSizeEXT"));
		vkGetDescriptorSetLayoutBindingOffsetEXT = reinterpret_cast<PFN_vkGetDescriptorSetLayoutBindingOffsetEXT>(vkGetDeviceProcAddr(device, "vkGetDescriptorSetLayoutBindingOffsetEXT"));
		vkCmdBindDescriptorBuffersEXT = reinterpret_cast<PFN_vkCmdBindDescriptorBuffersEXT>(vkGetDeviceProcAddr(device, "vkCmdBindDescriptorBuffersEXT"));
		vkGetDescriptorEXT = reinterpret_cast<PFN_vkGetDescriptorEXT>(vkGetDeviceProcAddr(device, "vkGetDescriptorEXT"));
		vkCmdBindDescriptorBufferEmbeddedSamplersEXT = reinterpret_cast<PFN_vkCmdBindDescriptorBufferEmbeddedSamplersEXT>(vkGetDeviceProcAddr(device, "vkCmdBindDescriptorBufferEmbeddedSamplersEXT"));
		vkCmdSetDescriptorBufferOffsetsEXT = reinterpret_cast<PFN_vkCmdSetDescriptorBufferOffsetsEXT>(vkGetDeviceProcAddr(device, "vkCmdSetDescriptorBufferOffsetsEXT"));

		loadAssets();
		prepareUniformBuffers();
		setupDescriptors();
		prepareDescriptorBuffer();
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
			if (cubes[1].rotation.y > 360.0f)
				cubes[1].rotation.y -= 360.0f;
		}
		if ((camera.updated) || (animate && !paused)) {
			updateUniformBuffers();
		}
	}

	virtual void OnUpdateUIOverlay(vks::UIOverlay *overlay)
	{
		if (overlay->header("Settings")) {
			overlay->checkBox("Animate", &animate);
		}
	}
};

VULKAN_EXAMPLE_MAIN()
