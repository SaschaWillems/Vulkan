/*
 * Vulkan Example - Using descriptor heaps via VK_EXT_descriptor_heap
 *
 * Copyright (C) 2026 by Sascha Willems - www.saschawillems.de
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
	
	VkPhysicalDeviceDescriptorHeapFeaturesEXT enabledDeviceDescriptorHeapFeaturesEXT{};
	VkPhysicalDeviceBufferDeviceAddressFeatures enabledBufferDeviceAddressFeatures{};

	VkPhysicalDeviceDescriptorHeapPropertiesEXT descriptorHeapProperties{};

	PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR{ nullptr };
	PFN_vkWriteResourceDescriptorsEXT vkWriteResourceDescriptorsEXT{ nullptr };
	PFN_vkCmdBindResourceHeapEXT vkCmdBindResourceHeapEXT{ nullptr };
	PFN_vkCmdBindSamplerHeapEXT vkCmdBindSamplerHeapEXT{ nullptr };
	PFN_vkGetPhysicalDeviceDescriptorSizeEXT vkGetPhysicalDeviceDescriptorSizeEXT{ nullptr };

	// Stores all values that are required to setup a descriptor buffer for a resource buffer
	struct DescriptorInfo {
		VkDeviceSize layoutOffset;
		VkDeviceSize layoutSize;
		VkDescriptorSetLayout setLayout;
	};
	
	vks::Buffer descriptorHeap;
	vks::Buffer descriptorHeapSamplers;
	vks::Buffer uniformBuffers{};

	struct ImageDescriptorInfo : DescriptorInfo {
		vks::Buffer buffer;
	} combinedImageDescriptor{};

	// Descriptor heap makes heavy use of buffer device addresses
	uint64_t getBufferDeviceAddress(vks::Buffer &buffer)
	{
		VkBufferDeviceAddressInfoKHR bufferDeviceAI{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = buffer.buffer };
		buffer.deviceAddress = vkGetBufferDeviceAddressKHR(device, &bufferDeviceAI);
		return buffer.deviceAddress;
	}

	VulkanExample() : VulkanExampleBase()
	{
		title = "Descriptor heaps (VK_EXT_descriptor_heap)";
		camera.type = Camera::CameraType::lookat;
		camera.setPerspective(60.0f, (float)width / (float)height, 0.1f, 512.0f);
		camera.setRotation(glm::vec3(0.0f, 0.0f, 0.0f));
		camera.setTranslation(glm::vec3(0.0f, 0.0f, -5.0f));

		apiVersion = VK_API_VERSION_1_2;

		enabledInstanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
		enabledDeviceExtensions.push_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
		enabledDeviceExtensions.push_back(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
		enabledDeviceExtensions.push_back(VK_KHR_MAINTENANCE_5_EXTENSION_NAME);
		enabledDeviceExtensions.push_back(VK_EXT_DESCRIPTOR_HEAP_EXTENSION_NAME);

		enabledBufferDeviceAddressFeatures = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
			.bufferDeviceAddress = VK_TRUE
		};

		enabledDeviceDescriptorHeapFeaturesEXT = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_HEAP_FEATURES_EXT,
			.pNext = &enabledBufferDeviceAddressFeatures,
			.descriptorHeap = VK_TRUE 
		};

		deviceCreatepNextChain = &enabledDeviceDescriptorHeapFeaturesEXT;
	}

	~VulkanExample()
	{
		//vkDestroyDescriptorSetLayout(device, uniformDescriptor.setLayout, nullptr);
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
		//for (auto& buffer : uniformDescriptor.buffers) {
		//	buffer.destroy();
		//}
		combinedImageDescriptor.buffer.destroy();
	}

	virtual void getEnabledFeatures()
	{
		if (deviceFeatures.samplerAnisotropy) {
			enabledFeatures.samplerAnisotropy = VK_TRUE;
		};
	}

	void preparePipelines()
	{
		VkPipelineLayoutCreateInfo pipelineLayoutCI{};
		pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
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
			loadShader(getShadersPath() + "descriptorheap/cube.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
			loadShader(getShadersPath() + "descriptorheap/cube.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT)
		};

		VkDescriptorSetAndBindingMappingEXT setAndBindingMappingBuffers{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_AND_BINDING_MAPPING_EXT,
			.descriptorSet = 0,
			.firstBinding = 0,
			.bindingCount = 1,
			.resourceMask = VK_SPIRV_RESOURCE_TYPE_UNIFORM_BUFFER_BIT_EXT,
			.source = VK_DESCRIPTOR_MAPPING_SOURCE_HEAP_WITH_CONSTANT_OFFSET_EXT,
		};

		VkDescriptorSetAndBindingMappingEXT setAndBindingMappingImages{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_AND_BINDING_MAPPING_EXT,
			.descriptorSet = 2,
			.firstBinding = 0,
			.bindingCount = 1,
			.resourceMask = VK_SPIRV_RESOURCE_TYPE_COMBINED_SAMPLED_IMAGE_BIT_EXT,
			.source = VK_DESCRIPTOR_MAPPING_SOURCE_HEAP_WITH_CONSTANT_OFFSET_EXT,
		};

		std::array<VkDescriptorSetAndBindingMappingEXT, 3> sabms = { setAndBindingMappingBuffers, setAndBindingMappingImages };

		VkShaderDescriptorSetAndBindingMappingInfoEXT descriptorSetAndBindingMappingInfo{
			.sType = VK_STRUCTURE_TYPE_SHADER_DESCRIPTOR_SET_AND_BINDING_MAPPING_INFO_EXT,
			.mappingCount = static_cast<uint32_t>(sabms.size()),
			.pMappings = sabms.data()
		};

		shaderStages[0].pNext = &descriptorSetAndBindingMappingInfo;
		shaderStages[1].pNext = &descriptorSetAndBindingMappingInfo;

		VkPipelineCreateFlags2CreateInfo pcf2ci{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_CREATE_FLAGS_2_CREATE_INFO,
			.flags = VK_PIPELINE_CREATE_2_DESCRIPTOR_HEAP_BIT_EXT
		};

		VkGraphicsPipelineCreateInfo pipelineCI = vks::initializers::pipelineCreateInfo();
		pipelineCI.renderPass = renderPass;
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
		pipelineCI.pNext = &pcf2ci;
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipeline));
	}

	void prepareDescriptorHeaps()
	{
		PFN_vkGetPhysicalDeviceProperties2KHR vkGetPhysicalDeviceProperties2KHR = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2KHR>(vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceProperties2KHR"));
		assert(vkGetPhysicalDeviceProperties2KHR);
		VkPhysicalDeviceProperties2KHR deviceProps2{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR };
		descriptorHeapProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_HEAP_PROPERTIES_EXT;
		deviceProps2.pNext = &descriptorHeapProperties;
		vkGetPhysicalDeviceProperties2KHR(physicalDevice, &deviceProps2);		

		const VkDeviceSize uBufSize = sizeof(glm::mat4) * 4;
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&uniformBuffers,
			uBufSize));
		uniformBuffers.map();
		getBufferDeviceAddress(uniformBuffers);

		const VkDeviceSize heapSizeBuf = vks::tools::alignedVkSize(512 + descriptorHeapProperties.minResourceHeapReservedRange, descriptorHeapProperties.resourceHeapAlignment);
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_DESCRIPTOR_HEAP_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&descriptorHeap,
			heapSizeBuf));
		descriptorHeap.map();
		getBufferDeviceAddress(descriptorHeap);

		const VkDeviceSize heapSizeImg = vks::tools::alignedVkSize(512 + descriptorHeapProperties.minResourceHeapReservedRange, descriptorHeapProperties.resourceHeapAlignment);
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_DESCRIPTOR_HEAP_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&descriptorHeapSamplers,
			heapSizeImg));
		descriptorHeapSamplers.map();
		getBufferDeviceAddress(descriptorHeapSamplers);

		for (uint32_t i = 0; i < static_cast<uint32_t>(cubes.size()); i++) {
			VkImageViewCreateInfo vci{
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.image = cubes[i].texture.image,
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.format = cubes[i].texture.format,
				.subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = cubes[i].texture.mipLevels, .baseArrayLayer = 0, .layerCount = 1},
			};
			VkImageDescriptorInfoEXT idi{
				.sType = VK_STRUCTURE_TYPE_IMAGE_DESCRIPTOR_INFO_EXT,
				.pView = &vci,
				.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			};
			VkResourceDescriptorInfoEXT rdiImg{
				.sType = VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT,
				.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
				.data = {
					.pImage = &idi
				}
			};
			const VkDeviceSize imgDescSize = vkGetPhysicalDeviceDescriptorSizeEXT(physicalDevice, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
			auto imgDescData = std::make_unique<uint8_t[]>(static_cast<size_t>(imgDescSize));
			VkHostAddressRangeEXT vharImg{
				.address = imgDescData.get(),
				.size = static_cast<size_t>(imgDescSize)
			};
			vkWriteResourceDescriptorsEXT(device, 1, &rdiImg, &vharImg);
		}

		VkHostAddressRangeEXT vharBuf{
			.address = static_cast<uint8_t*>(descriptorHeap.mapped),
			.size = descriptorHeap.size
		};

		VkDeviceAddressRangeEXT input_address_range = { uniformBuffers.deviceAddress, uniformBuffers.size };
		VkResourceDescriptorInfoEXT in_descriptor_info = {
			.sType = VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT,
			.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.data = {
				.pAddressRange = &input_address_range
			}
		};
		vkWriteResourceDescriptorsEXT(device, 1, &in_descriptor_info, &vharBuf);
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
		char* bufDataPtr = (char*)uniformBuffers.mapped;
		size_t sInc = sizeof(glm::mat4);
		memcpy(bufDataPtr, &camera.matrices.perspective, sizeof(glm::mat4));
		bufDataPtr += sInc;
		memcpy(bufDataPtr, &camera.matrices.view, sizeof(glm::mat4));

		cubes[0].matrix = glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, 0.0f, 0.0f));
		cubes[1].matrix = glm::translate(glm::mat4(1.0f), glm::vec3( 1.5f, 0.5f, 0.0f));
		for (auto& cube : cubes) {
			bufDataPtr += sInc;
			cube.matrix = glm::rotate(cube.matrix, glm::radians(cube.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
			cube.matrix = glm::rotate(cube.matrix, glm::radians(cube.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
			cube.matrix = glm::rotate(cube.matrix, glm::radians(cube.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
			cube.matrix = glm::scale(cube.matrix, glm::vec3(0.25f));
			memcpy(bufDataPtr, &cube.matrix, sizeof(glm::mat4));
		}

	}

	void prepare()
	{
		VulkanExampleBase::prepare();

		// Using descriptor heaps requires some extensions, and with that functions to be loaded explicitly
		vkGetBufferDeviceAddressKHR = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(vkGetDeviceProcAddr(device, "vkGetBufferDeviceAddressKHR"));
		vkWriteResourceDescriptorsEXT = reinterpret_cast<PFN_vkWriteResourceDescriptorsEXT>(vkGetDeviceProcAddr(device, "vkWriteResourceDescriptorsEXT"));
		vkCmdBindResourceHeapEXT = reinterpret_cast<PFN_vkCmdBindResourceHeapEXT>(vkGetDeviceProcAddr(device, "vkCmdBindResourceHeapEXT"));
		vkCmdBindSamplerHeapEXT = reinterpret_cast<PFN_vkCmdBindSamplerHeapEXT>(vkGetDeviceProcAddr(device, "vkCmdBindSamplerHeapEXT"));
		vkGetPhysicalDeviceDescriptorSizeEXT = reinterpret_cast<PFN_vkGetPhysicalDeviceDescriptorSizeEXT>(vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceDescriptorSizeEXT"));

		loadAssets();
		prepareUniformBuffers();
		prepareDescriptorHeaps();
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

		const VkDeviceSize resStrideBuf = std::max(descriptorHeapProperties.resourceHeapAlignment, descriptorHeapProperties.bufferDescriptorAlignment);
		const VkDeviceSize resStrideImg = std::max(descriptorHeapProperties.resourceHeapAlignment, descriptorHeapProperties.imageDescriptorAlignment);

		// @todo
		VkBindHeapInfoEXT bindHeapInfoBuffers{
			.sType = VK_STRUCTURE_TYPE_BIND_HEAP_INFO_EXT,
			.pNext = nullptr,
			.heapRange{
				.address = descriptorHeap.deviceAddress,
				.size = descriptorHeap.size
			},
			.reservedRangeSize = descriptorHeapProperties.minResourceHeapReservedRange
		};
		vkCmdBindResourceHeapEXT(cmdBuffer, &bindHeapInfoBuffers);

		uint32_t bufferIndexUbo = 0;
		VkDeviceSize globalBufferOffset = 0;

		model.bindBuffers(cmdBuffer);
		auto &primitive = model.nodeFromName("cube")->mesh[0].primitives[0];
		for (uint32_t j = 0; j < static_cast<uint32_t>(cubes.size()); j++) {
			uint32_t bufferIndexImage = 1;
			VkDeviceSize imageBufferOffset = j * combinedImageDescriptor.layoutSize;
			vkCmdDrawIndexed(cmdBuffer, primitive->indexCount, 1, primitive->firstIndex, 0, j);			
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
