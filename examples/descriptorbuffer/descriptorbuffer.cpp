/*
 * Vulkan Example - Using descriptor buffers via VK_EXT_descriptor_buffer
 *
 * Copyright (C) 2022-2025 by Sascha Willems - www.saschawillems.de
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */

#include "vulkanexamplebase.h"
#include "VulkanglTFModel.h"


class VulkanExample : public VulkanExampleBase
{
public:
	bool animate = true;

	struct Cube {
		glm::mat4 matrix;
		vks::Texture2D texture;
		std::array<vks::Buffer, maxConcurrentFrames> uniformBuffers;
		glm::vec3 rotation;
	};
	std::array<Cube, 2> cubes;

	std::array<vks::Buffer, maxConcurrentFrames> uniformBuffersCamera;

	vkglTF::Model model;

	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;

	VkPhysicalDeviceDescriptorBufferFeaturesEXT enabledDeviceDescriptorBufferFeaturesEXT{};
	VkPhysicalDeviceBufferDeviceAddressFeatures enabledBufferDeviceAddresFeatures{};
	VkPhysicalDeviceDescriptorBufferPropertiesEXT descriptorBufferProperties{};

	PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR;
	PFN_vkGetDescriptorSetLayoutSizeEXT vkGetDescriptorSetLayoutSizeEXT;
	PFN_vkGetDescriptorSetLayoutBindingOffsetEXT vkGetDescriptorSetLayoutBindingOffsetEXT;
	PFN_vkCmdBindDescriptorBuffersEXT vkCmdBindDescriptorBuffersEXT;
	PFN_vkCmdSetDescriptorBufferOffsetsEXT vkCmdSetDescriptorBufferOffsetsEXT;
	PFN_vkGetDescriptorEXT vkGetDescriptorEXT;
	PFN_vkCmdBindDescriptorBufferEmbeddedSamplersEXT vkCmdBindDescriptorBufferEmbeddedSamplersEXT;

	// Stores all values that are required to setup a descriptor buffer for a resource buffer
	struct DescriptorInfo {
		VkDeviceSize layoutOffset;
		VkDeviceSize layoutSize;
		VkDescriptorSetLayout setLayout;
	};
	struct BufferDescriptorInfo : DescriptorInfo {
		std::array<vks::Buffer, maxConcurrentFrames> buffers;
	};
	BufferDescriptorInfo uniformDescriptor{};

	struct ImageDescriptorInfo : DescriptorInfo {
		vks::Buffer buffer;
	} combinedImageDescriptor{};

	// Descriptor buffer makes heavy use of buffer device addresses
	uint64_t getBufferDeviceAddress(vks::Buffer &buffer)
	{
		VkBufferDeviceAddressInfoKHR bufferDeviceAI{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = buffer.buffer };
		buffer.deviceAddress = vkGetBufferDeviceAddressKHR(device, &bufferDeviceAI);
		return buffer.deviceAddress;
	}

	VulkanExample() : VulkanExampleBase()
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
		vkDestroyDescriptorSetLayout(device, uniformDescriptor.setLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, combinedImageDescriptor.setLayout, nullptr);
		vkDestroyPipeline(device, pipeline, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		for (auto& cube : cubes) {
			for (auto& buffer : cube.uniformBuffers) {
				buffer.destroy();
			}
			cube.texture.destroy();
		}
		for (auto& buffer : uniformBuffersCamera) {
			buffer.destroy();
		}
		for (auto& buffer : uniformDescriptor.buffers) {
			buffer.destroy();
		}
		combinedImageDescriptor.buffer.destroy();
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
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayoutCI, nullptr, &uniformDescriptor.setLayout));

		setLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		setLayoutBinding.binding = 0;
		setLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		setLayoutBinding.descriptorCount = 1;
		descriptorLayoutCI.pBindings = &setLayoutBinding;
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayoutCI, nullptr, &combinedImageDescriptor.setLayout));
	}

	void preparePipelines()
	{
		// Set 0 = Camera UBO
		// Set 1 = Model UBO
		// Set 2 = Model image
		const std::array<VkDescriptorSetLayout, 3> setLayouts = { uniformDescriptor.setLayout, uniformDescriptor.setLayout, combinedImageDescriptor.setLayout };

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
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = {
			loadShader(getShadersPath() + "descriptorbuffer/cube.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
			loadShader(getShadersPath() + "descriptorbuffer/cube.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT)
		};

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
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipeline));
	}

	void prepareDescriptorBuffer()
	{
		// We need to get sizes and offsets for the descriptor layouts

		// This is done using a new extension structures and features
		PFN_vkGetPhysicalDeviceProperties2KHR vkGetPhysicalDeviceProperties2KHR = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2KHR>(vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceProperties2KHR"));
		assert(vkGetPhysicalDeviceProperties2KHR);
		VkPhysicalDeviceProperties2KHR deviceProps2{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR };
		descriptorBufferProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT;
		deviceProps2.pNext = &descriptorBufferProperties;
		vkGetPhysicalDeviceProperties2KHR(physicalDevice, &deviceProps2);

		// Some devices have very low limits for the no. of max descriptor buffer bindings, so we need to check
		if (descriptorBufferProperties.maxResourceDescriptorBufferBindings < 2) {
			vks::tools::exitFatal("This sample requires at least 2 descriptor bindings to run, the selected device only supports " + std::to_string(descriptorBufferProperties.maxResourceDescriptorBufferBindings), - 1);
		}

		vkGetDescriptorSetLayoutSizeEXT(device, uniformDescriptor.setLayout, &uniformDescriptor.layoutSize);
		vkGetDescriptorSetLayoutSizeEXT(device, combinedImageDescriptor.setLayout, &combinedImageDescriptor.layoutSize);

		vkGetDescriptorSetLayoutBindingOffsetEXT(device, uniformDescriptor.setLayout, 0, &uniformDescriptor.layoutOffset);
		vkGetDescriptorSetLayoutBindingOffsetEXT(device, combinedImageDescriptor.setLayout, 0, &combinedImageDescriptor.layoutOffset);

		// In order to copy resource descriptors to the correct place, we need to calculate aligned sizes
		uniformDescriptor.layoutSize = vks::tools::alignedVkSize(uniformDescriptor.layoutSize, descriptorBufferProperties.descriptorBufferOffsetAlignment);
		combinedImageDescriptor.layoutSize = vks::tools::alignedVkSize(combinedImageDescriptor.layoutSize, descriptorBufferProperties.descriptorBufferOffsetAlignment);

		// This buffer will contain resource descriptors for all the uniform buffers of a single frame (global matrices and cubes)
		uint32_t bufferDescriptorSize = (static_cast<uint32_t>(cubes.size()) + 1) * uniformDescriptor.layoutSize;
		for (uint32_t i = 0; i < maxConcurrentFrames; i++) {
			VK_CHECK_RESULT(vulkanDevice->createBuffer(
				VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				&uniformDescriptor.buffers[i],
				bufferDescriptorSize));
			uniformDescriptor.buffers[i].map();
			getBufferDeviceAddress(uniformDescriptor.buffers[i]);
		}

		// This buffer contains resource descriptors for the combined images (one per cube)
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, // Flags 1 & 2 are required for combined images
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&combinedImageDescriptor.buffer,
			static_cast<uint32_t>(cubes.size()) * combinedImageDescriptor.layoutSize));
		combinedImageDescriptor.buffer.map();
		getBufferDeviceAddress(combinedImageDescriptor.buffer);

		VkDescriptorGetInfoEXT descriptorInfo{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT };

		// Put image descriptors into the corresponding resource buffer
		char* imageDescriptorBufPtr = (char*)combinedImageDescriptor.buffer.mapped;
		descriptorInfo.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		for (uint32_t i = 0; i < static_cast<uint32_t>(cubes.size()); i++) {
			descriptorInfo.data.pCombinedImageSampler = &cubes[i].texture.descriptor;
			vkGetDescriptorEXT(device, &descriptorInfo, descriptorBufferProperties.combinedImageSamplerDescriptorSize, imageDescriptorBufPtr + i * combinedImageDescriptor.layoutSize + combinedImageDescriptor.layoutOffset);
		}

		// For uniform buffers we only need buffer device addresses and store them in the descriptor buffer
		for (uint32_t i = 0; i < maxConcurrentFrames; i++) {
			// Offset for the frame to be updated
			char* uniformDescriptorBufPtr = (char*)uniformDescriptor.buffers[i].mapped;

			// Global uniform buffer
			VkDescriptorAddressInfoEXT descriptorAddressInfo = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT };
			descriptorAddressInfo.address = getBufferDeviceAddress(uniformBuffersCamera[i]);
			descriptorAddressInfo.range = uniformBuffersCamera[i].size;
			descriptorAddressInfo.format = VK_FORMAT_UNDEFINED;

			descriptorInfo.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorInfo.data.pCombinedImageSampler = nullptr;
			descriptorInfo.data.pUniformBuffer = &descriptorAddressInfo;
			vkGetDescriptorEXT(device, &descriptorInfo, descriptorBufferProperties.uniformBufferDescriptorSize, uniformDescriptorBufPtr);

			// Per-model uniform buffers
			for (uint32_t j = 0; j < static_cast<uint32_t>(cubes.size()); j++) {
				descriptorAddressInfo = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT };
				descriptorAddressInfo.address = getBufferDeviceAddress(cubes[j].uniformBuffers[i]);
				descriptorAddressInfo.range = cubes[j].uniformBuffers[i].size;
				descriptorAddressInfo.format = VK_FORMAT_UNDEFINED;

				descriptorInfo.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				descriptorInfo.data.pCombinedImageSampler = nullptr;
				descriptorInfo.data.pUniformBuffer = &descriptorAddressInfo;
				vkGetDescriptorEXT(device, &descriptorInfo, descriptorBufferProperties.uniformBufferDescriptorSize, uniformDescriptorBufPtr + (j + 1) * uniformDescriptor.layoutSize + uniformDescriptor.layoutOffset);
			}
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
		for (uint32_t i = 0; i < maxConcurrentFrames; i++) {
			// UBO for camera matrices
			VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &uniformBuffersCamera[i], sizeof(glm::mat4) * 2));
			VK_CHECK_RESULT(uniformBuffersCamera[i].map());
			// UBOs for model matrices
			for (auto& cube : cubes) {
				VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &cube.uniformBuffers[i], sizeof(glm::mat4)));
				VK_CHECK_RESULT(cube.uniformBuffers[i].map());
			}
		}
	}

	void updateUniformBuffers()
	{
		memcpy(uniformBuffersCamera[currentBuffer].mapped, &camera.matrices.perspective, sizeof(glm::mat4));
		memcpy((char*)uniformBuffersCamera[currentBuffer].mapped + sizeof(glm::mat4), &camera.matrices.view, sizeof(glm::mat4));

		cubes[0].matrix = glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, 0.0f, 0.0f));
		cubes[1].matrix = glm::translate(glm::mat4(1.0f), glm::vec3( 1.5f, 0.5f, 0.0f));
		for (auto& cube : cubes) {
			cube.matrix = glm::rotate(cube.matrix, glm::radians(cube.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
			cube.matrix = glm::rotate(cube.matrix, glm::radians(cube.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
			cube.matrix = glm::rotate(cube.matrix, glm::radians(cube.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
			cube.matrix = glm::scale(cube.matrix, glm::vec3(0.25f));
			memcpy(cube.uniformBuffers[currentBuffer].mapped, &cube.matrix, sizeof(glm::mat4));
		}
	}

	void prepare()
	{
		VulkanExampleBase::prepare();

		// Using descriptor buffers requires some extensions, and with that functions to be loaded explicitly
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
		prepared = true;
	}

	void buildCommandBuffer()
	{
		VkCommandBuffer cmdBuffer = drawCmdBuffers[currentBuffer];

		VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

		VkClearValue clearValues[2]{};
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
		renderPassBeginInfo.framebuffer = frameBuffers[currentImageIndex];

		VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));

		vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		VkViewport viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
		vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

		VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);
		vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

		VkDeviceSize offsets[1] = { 0 };
		model.bindBuffers(cmdBuffer);

		// Descriptor buffer bindings
		// Set 0 = uniform buffer
		VkDescriptorBufferBindingInfoEXT bindingInfos[2]{};
		bindingInfos[0].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT;
		bindingInfos[0].address = uniformDescriptor.buffers[currentBuffer].deviceAddress;
		bindingInfos[0].usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;
		// Set 1 = Image
		bindingInfos[1].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT;
		bindingInfos[1].pNext = nullptr;
		bindingInfos[1].address = combinedImageDescriptor.buffer.deviceAddress;
		bindingInfos[1].usage = VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;
		vkCmdBindDescriptorBuffersEXT(cmdBuffer, 2, bindingInfos);

		uint32_t bufferIndexUbo = 0;
		VkDeviceSize globalBufferOffset = 0;

		// Global Matrices (set 0)
		vkCmdSetDescriptorBufferOffsetsEXT(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &bufferIndexUbo, &globalBufferOffset);

		// Set and offset into descriptor for each model
		for (uint32_t j = 0; j < static_cast<uint32_t>(cubes.size()); j++) {
			// Uniform buffer (set 1)
			// Model ubos start at offset * 1 (slot 0 is global matrices)
			VkDeviceSize uniformbufferOffset = (j + 1) * uniformDescriptor.layoutSize;
			vkCmdSetDescriptorBufferOffsetsEXT(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &bufferIndexUbo, &uniformbufferOffset);
			// Image (set 2)
			uint32_t bufferIndexImage = 1;
			VkDeviceSize imageBufferOffset = j * combinedImageDescriptor.layoutSize;
			vkCmdSetDescriptorBufferOffsetsEXT(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 2, 1, &bufferIndexImage, &imageBufferOffset);
			model.draw(cmdBuffer);
		}

		drawUI(cmdBuffer);

		vkCmdEndRenderPass(cmdBuffer);

		VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffer));
	}

	virtual void render()
	{
		if (!prepared)
			return;
		if (animate && !paused) {
			cubes[0].rotation.x += 2.5f * frameTimer;
			if (cubes[0].rotation.x > 360.0f)
				cubes[0].rotation.x -= 360.0f;
			cubes[1].rotation.y += 2.0f * frameTimer;
			if (cubes[1].rotation.y > 360.0f)
				cubes[1].rotation.y -= 360.0f;
		}
		VulkanExampleBase::prepareFrame();
		updateUniformBuffers();
		buildCommandBuffer();
		VulkanExampleBase::submitFrame();
	}

	virtual void OnUpdateUIOverlay(vks::UIOverlay *overlay)
	{
		if (overlay->header("Settings")) {
			overlay->checkBox("Animate", &animate);
		}
	}
};

VULKAN_EXAMPLE_MAIN()
