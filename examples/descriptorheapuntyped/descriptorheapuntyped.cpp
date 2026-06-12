/*
 * Vulkan Example - Using untyped descriptor heaps via VK_EXT_descriptor_heap
 * 
 * Descriptor heaps fundamentally rework how shader resources are bound. Descriptors are simply stored in buffers (heaps).
 * 
 * Note: This sample also uses uniform buffers and buffer device address to pass global data
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
	std::array<vks::Texture2D, 2> textures{};

	struct UniformData {
		glm::mat4 mvp{ glm::mat4(1.0f) };
		uint32_t samplerIndex{ 0 };
		uint32_t imageHeapIndexOffset{ 0 };
	} uniformData;
	std::array<vks::Buffer, maxConcurrentFrames> uniformBuffers;

	// This per-model data will be accessed via resource heaps
	struct ModelData {
		glm::vec4 pos;
		glm::vec4 color;
	};
	vks::Buffer modelDataBuffers[2];

	struct PushConstantBlock {
		VkDeviceAddress matrixReference;
	};

	int32_t selectedSampler{ 0 };
	vkglTF::Model model;

	VkPipeline pipeline{ nullptr };
	
	VkPhysicalDeviceVulkan12Features enabledDeviceVulkan12Features{};
	VkPhysicalDeviceDescriptorHeapFeaturesEXT enabledDeviceDescriptorHeapFeaturesEXT{};
	VkPhysicalDeviceSynchronization2Features enabledSynchronization2Features{};
	VkPhysicalDeviceShaderUntypedPointersFeaturesKHR enabledShaderUntypedPointersFeaturesKHR{};

	VkPhysicalDeviceDescriptorHeapPropertiesEXT descriptorHeapProperties{};

	PFN_vkWriteResourceDescriptorsEXT vkWriteResourceDescriptorsEXT{ nullptr };
	PFN_vkCmdBindResourceHeapEXT vkCmdBindResourceHeapEXT{ nullptr };
	PFN_vkCmdBindSamplerHeapEXT vkCmdBindSamplerHeapEXT{ nullptr };
	PFN_vkWriteSamplerDescriptorsEXT vkWriteSamplerDescriptorsEXT{ nullptr };
	PFN_vkCmdPushDataEXT vkCmdPushDataEXT{ nullptr };

	vks::Buffer descriptorHeapResources{};
	vks::Buffer descriptorHeapSamplers{};

	// Size and offset values for heap objects
	VkDeviceSize bufferHeapOffset{ 0 };
	VkDeviceSize bufferDescriptorSize{ 0 };
	VkDeviceSize imageHeapOffset{ 0 };
	VkDeviceSize imageDescriptorSize{ 0 };
	VkDeviceSize samplerHeapOffset{ 0 };
	VkDeviceSize samplerDescriptorSize{ 0 };

	std::vector<std::string> samplerNames{ "Linear", "Nearest" };

	// Descriptor heap makes heavy use of buffer device addresses
	uint64_t getBufferDeviceAddress(vks::Buffer &buffer)
	{
		VkBufferDeviceAddressInfoKHR bufferDeviceAI{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = buffer.buffer };
		buffer.deviceAddress = vkGetBufferDeviceAddress(device, &bufferDeviceAI);
		return buffer.deviceAddress;
	}

	VulkanExample() : VulkanExampleBase()
	{
		title = "Untyped descriptor heaps (VK_EXT_descriptor_heap)";
		useDynamicRendering = true;
		camera.type = Camera::CameraType::lookat;
		camera.setPerspective(60.0f, (float)width / (float)height, 0.1f, 512.0f);
		camera.setRotation(glm::vec3(0.0f, 0.0f, 0.0f));
		camera.setTranslation(glm::vec3(0.0f, 0.0f, -5.0f));

		// We use 1.3 as a baseline, so we can use the dynamic rendering functionality of the base class
		// Descriptor heaps do work with earlier versions though
		apiVersion = VK_API_VERSION_1_3;

		enabledDeviceExtensions.push_back(VK_EXT_DESCRIPTOR_HEAP_EXTENSION_NAME);
		enabledDeviceExtensions.push_back(VK_KHR_MAINTENANCE_5_EXTENSION_NAME);
		enabledDeviceExtensions.push_back(VK_KHR_SHADER_UNTYPED_POINTERS_EXTENSION_NAME);
		enabledDeviceExtensions.push_back(VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME);

		enabledDeviceVulkan12Features = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
			.pNext = &baseDynamicRenderingFeatures,
			.shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
			.shaderStorageBufferArrayNonUniformIndexing = VK_TRUE,
			.bufferDeviceAddress = VK_TRUE,
		};

		enabledSynchronization2Features = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
			.pNext = &enabledDeviceVulkan12Features,
			.synchronization2 = VK_TRUE
		};

		enabledDeviceDescriptorHeapFeaturesEXT = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_HEAP_FEATURES_EXT,
			.pNext = &enabledSynchronization2Features,
			.descriptorHeap = VK_TRUE,
		};

		enabledShaderUntypedPointersFeaturesKHR = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_UNTYPED_POINTERS_FEATURES_KHR,
			.pNext = &enabledDeviceDescriptorHeapFeaturesEXT,
			.shaderUntypedPointers = VK_TRUE,
		};
		deviceCreatepNextChain = &enabledShaderUntypedPointersFeaturesKHR;
	}

	~VulkanExample()
	{
		if (device) {
			for (auto& uniformBuffer : uniformBuffers) {
				uniformBuffer.destroy();
			}
			for (auto& modelDataBuffer : modelDataBuffers) {
				modelDataBuffer.destroy();
			}
			descriptorHeapResources.destroy();
			descriptorHeapSamplers.destroy();
			vkDestroyBuffer(device, descriptorHeapResources.buffer, nullptr);
			vkDestroyBuffer(device, descriptorHeapSamplers.buffer, nullptr);
			vkDestroyPipeline(device, pipeline, nullptr);
			for (auto& texture : textures) {
				texture.destroy();
			}
		}
	}

	virtual void getEnabledFeatures()
	{
		if (deviceFeatures.samplerAnisotropy) {
			enabledFeatures.samplerAnisotropy = VK_TRUE;
		};
	}

	void prepareUniformBuffers()
	{
		for (uint32_t i = 0; i < maxConcurrentFrames; i++) {
			VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &uniformBuffers[i], sizeof(UniformData)));
			VK_CHECK_RESULT(uniformBuffers[i].map());
			getBufferDeviceAddress(uniformBuffers[i]);
		}
	}

	void prepareDescriptorHeaps()
	{
		// Descriptor heaps have varying offset, size and alignment requirements, so we store it's properties for later user
		VkPhysicalDeviceProperties2 deviceProps2{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
		descriptorHeapProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_HEAP_PROPERTIES_EXT;
		deviceProps2.pNext = &descriptorHeapProperties;
		vkGetPhysicalDeviceProperties2(physicalDevice, &deviceProps2);

		// There are two descriptor heap types: One that can store resources (buffers, images) and one that can store samplers

		// Sampler heap
		// We need to calculate some aligned offsets, heaps and strides to make sure we properly accress the descriptors
		samplerDescriptorSize = vks::tools::alignedVkSize(descriptorHeapProperties.samplerDescriptorSize, descriptorHeapProperties.samplerDescriptorAlignment);

		// Size calculations for the heap also need to accomodate for the reserved range, used by the driver for internal bookkeeping
		const VkDeviceSize heapSizeSamplers = vks::tools::alignedVkSize(samplerDescriptorSize * 2 + descriptorHeapProperties.minSamplerHeapReservedRange, descriptorHeapProperties.samplerHeapAlignment);
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_DESCRIPTOR_HEAP_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&descriptorHeapSamplers,
			heapSizeSamplers));
		descriptorHeapSamplers.map();
		getBufferDeviceAddress(descriptorHeapSamplers);

		std::array<VkHostAddressRangeEXT, 2> hostAddressRangesSamplers{};

		// No need to create an actual VkSampler, we can simply pass the create info that describes the sampler
		std::array<VkSamplerCreateInfo, 2> samplerCreateInfos{
			VkSamplerCreateInfo{
				.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
				.magFilter = VK_FILTER_LINEAR,
				.minFilter = VK_FILTER_LINEAR,
				.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
				.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
				.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
				.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
				.maxAnisotropy = 16.0f,
				.maxLod = (float)textures[0].mipLevels,
			},
			VkSamplerCreateInfo{
				.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
				.magFilter = VK_FILTER_NEAREST,
				.minFilter = VK_FILTER_NEAREST,
				.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
				.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
				.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
				.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
				.maxAnisotropy = 16.0f,
				.maxLod = (float)textures[0].mipLevels,
			}
		};

		for (auto i = 0; i < samplerCreateInfos.size(); i++)
		{
			hostAddressRangesSamplers[i] = {
				.address = static_cast<uint8_t*>(descriptorHeapSamplers.mapped) + samplerDescriptorSize * i,
				.size = samplerDescriptorSize
			};
		}

		VK_CHECK_RESULT(vkWriteSamplerDescriptorsEXT(device, static_cast<uint32_t>(samplerCreateInfos.size()), samplerCreateInfos.data(), hostAddressRangesSamplers.data()));

		// Resource heap (buffers and images)

		bufferDescriptorSize = vks::tools::alignedVkSize(descriptorHeapProperties.bufferDescriptorSize, descriptorHeapProperties.bufferDescriptorAlignment);
		// Images are storted after the last buffer (aligned)
		imageHeapOffset = vks::tools::alignedVkSize(2 * bufferDescriptorSize, descriptorHeapProperties.imageDescriptorAlignment);
		imageDescriptorSize = vks::tools::alignedVkSize(descriptorHeapProperties.imageDescriptorSize, descriptorHeapProperties.imageDescriptorAlignment);

		// Size calculations for the heap also need to accomodate for the reserved range, used by the driver for internal bookkeeping
		const VkDeviceSize heapSizeResources = vks::tools::alignedVkSize(bufferDescriptorSize * 2 + imageDescriptorSize * 2 + descriptorHeapProperties.minResourceHeapReservedRange, descriptorHeapProperties.resourceHeapAlignment);
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_DESCRIPTOR_HEAP_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&descriptorHeapResources,
			heapSizeResources));
		descriptorHeapResources.map();
		getBufferDeviceAddress(descriptorHeapResources);

		std::vector<VkHostAddressRangeEXT> hostAddressRangesResources{};
		std::vector<VkResourceDescriptorInfoEXT> resourceDescriptorInfos{};
		
		// Buffer data
		std::array<VkDeviceAddressRangeEXT, 2> deviceAddressRangesModelData{};

		for (auto i = 0; i < 2; i++) {
			VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &modelDataBuffers[i], sizeof(ModelData)));
			VK_CHECK_RESULT(modelDataBuffers[i].map());
			getBufferDeviceAddress(modelDataBuffers[i]);
			const glm::vec4 positions[2] = { glm::vec4(-1.5f, 0.0f, 0.0f, 0.0f), glm::vec4(1.5f, 0.0f, 0.0f, 0.0f) };
			const glm::vec4 colors[2] = { glm::vec4(0.5f, 1.0f, 0.5f, 0.0f), glm::vec4(0.5f, 0.5f, 1.0f, 0.0f) };
			ModelData mdata{ .pos = positions[i], .color = colors[i] };
			memcpy(modelDataBuffers[i].mapped, &mdata, sizeof(ModelData));

			deviceAddressRangesModelData[i] = {.address = modelDataBuffers[i].deviceAddress, .size = modelDataBuffers[i].size};
			resourceDescriptorInfos.push_back({
				.sType = VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT,
				.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.data = {
					.pAddressRange = &deviceAddressRangesModelData[i],
				}
			});
			hostAddressRangesResources.push_back({
				.address = static_cast<uint8_t*>(descriptorHeapResources.mapped) + bufferDescriptorSize * i,
				.size = bufferDescriptorSize
			});
		}

		// Images
		std::array<VkImageViewCreateInfo, 2> imageViewCreateInfos{};
		std::array<VkImageDescriptorInfoEXT, 2> imageDescriptorInfo{};

		for (auto i = 0; i < 2; i++) {
			imageViewCreateInfos[i] = {
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.image = textures[i].image,
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.format = textures[i].format,
				.subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = textures[i].mipLevels, .baseArrayLayer = 0, .layerCount = 1},
			};

			imageDescriptorInfo[i] = {
				.sType = VK_STRUCTURE_TYPE_IMAGE_DESCRIPTOR_INFO_EXT,
				.pView = &imageViewCreateInfos[i],
				.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			};

			resourceDescriptorInfos.push_back({
				.sType = VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT,
				.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
				.data = {
					.pImage = &imageDescriptorInfo[i]
				}
			});

			hostAddressRangesResources.push_back({
				.address = static_cast<uint8_t*>(descriptorHeapResources.mapped) + imageHeapOffset + imageDescriptorSize * i,
				.size = imageDescriptorSize
			});
		}
		// With untyped pointers we need to manually offset into the resource heap as images are stored after the buffers
		// We calulcate this and pass it to the fragment shader to be used as an offset there
		uniformData.imageHeapIndexOffset = static_cast<uint32_t>(imageHeapOffset / imageDescriptorSize);

		VK_CHECK_RESULT(vkWriteResourceDescriptorsEXT(device, static_cast<uint32_t>(resourceDescriptorInfos.size()), resourceDescriptorInfos.data(), hostAddressRangesResources.data()));
	}

	void preparePipelines()
	{
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
			loadShader(getShadersPath() + "descriptorheapuntyped/cube.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
			loadShader(getShadersPath() + "descriptorheapuntyped/cube.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT)
		};

		VkGraphicsPipelineCreateInfo pipelineCI{
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.stageCount = static_cast<uint32_t>(shaderStages.size()),
			.pStages = shaderStages.data(),
			.pVertexInputState = vkglTF::Vertex::getPipelineVertexInputState({ vkglTF::VertexComponent::Position, vkglTF::VertexComponent::Normal, vkglTF::VertexComponent::UV, vkglTF::VertexComponent::Color }),
			.pInputAssemblyState = &inputAssemblyStateCI,
			.pViewportState = &viewportStateCI,
			.pRasterizationState = &rasterizationStateCI,
			.pMultisampleState = &multisampleStateCI,
			.pDepthStencilState = &depthStencilStateCI,
			.pColorBlendState = &colorBlendStateCI,
			.pDynamicState = &dynamicStateCI
		};
		VkPipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
			.colorAttachmentCount = 1,
			.pColorAttachmentFormats = &swapChain.colorFormat,
			.depthAttachmentFormat = depthFormat,
			.stencilAttachmentFormat = depthFormat
		};
		
		// With descriptor heaps we no longer need a pipeline layout
		// This struct must be chained into pipeline creation to enable the use of heaps (allowing us to leave pipelineLayout empty)
		VkPipelineCreateFlags2CreateInfo pipelineCreateFlags2CI{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_CREATE_FLAGS_2_CREATE_INFO,
			.pNext = &pipelineRenderingCreateInfo,
			.flags = VK_PIPELINE_CREATE_2_DESCRIPTOR_HEAP_BIT_EXT
		};
		pipelineCI.pNext = &pipelineCreateFlags2CI;

		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipeline));
	}

	void loadAssets()
	{
		const uint32_t glTFLoadingFlags = vkglTF::FileLoadingFlags::PreTransformVertices | vkglTF::FileLoadingFlags::PreMultiplyVertexColors | vkglTF::FileLoadingFlags::FlipY;
		model.loadFromFile(getAssetPath() + "models/cube.gltf", vulkanDevice, queue, glTFLoadingFlags);
		textures[0].loadFromFile(getAssetPath() + "textures/crate01_color_height_rgba.ktx", VK_FORMAT_R8G8B8A8_UNORM, vulkanDevice, queue);
		textures[1].loadFromFile(getAssetPath() + "textures/crate02_color_height_rgba.ktx", VK_FORMAT_R8G8B8A8_UNORM, vulkanDevice, queue);
	}

	void updateUniformBuffers()
	{		
		uniformData.mvp = camera.matrices.perspective* camera.matrices.view;
		uniformData.samplerIndex = selectedSampler;
		memcpy(uniformBuffers[currentBuffer].mapped, &uniformData, sizeof(UniformData));
	}

	void prepare()
	{
		VulkanExampleBase::prepare();

		// Using descriptor heaps requires some extensions, and with that functions to be loaded explicitly
		vkWriteResourceDescriptorsEXT = reinterpret_cast<PFN_vkWriteResourceDescriptorsEXT>(vkGetDeviceProcAddr(device, "vkWriteResourceDescriptorsEXT"));
		vkCmdBindResourceHeapEXT = reinterpret_cast<PFN_vkCmdBindResourceHeapEXT>(vkGetDeviceProcAddr(device, "vkCmdBindResourceHeapEXT"));
		vkCmdBindSamplerHeapEXT = reinterpret_cast<PFN_vkCmdBindSamplerHeapEXT>(vkGetDeviceProcAddr(device, "vkCmdBindSamplerHeapEXT"));
		vkWriteSamplerDescriptorsEXT = reinterpret_cast<PFN_vkWriteSamplerDescriptorsEXT>(vkGetDeviceProcAddr(device, "vkWriteSamplerDescriptorsEXT"));
		vkCmdPushDataEXT = reinterpret_cast<PFN_vkCmdPushDataEXT>(vkGetDeviceProcAddr(device, "vkCmdPushDataEXT"));

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

		VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));

		beginDynamicRendering(cmdBuffer);

		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		VkViewport viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
		vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
		VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);
		vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

		// Bind the heap containing resources (buffers and images)
		VkBindHeapInfoEXT bindHeapInfoRes{
			.sType = VK_STRUCTURE_TYPE_BIND_HEAP_INFO_EXT,
			.heapRange{
				.address = descriptorHeapResources.deviceAddress,
				.size = descriptorHeapResources.size
			},
			// Put the reserved range after our descriptors, simplifies some calculations
			.reservedRangeOffset = descriptorHeapResources.size - descriptorHeapProperties.minResourceHeapReservedRange,
			.reservedRangeSize = descriptorHeapProperties.minResourceHeapReservedRange,
		};
		vkCmdBindResourceHeapEXT(cmdBuffer, &bindHeapInfoRes);
		
		// Bind the heap containing samplers
		VkBindHeapInfoEXT bindHeapInfoSamplers{
			.sType = VK_STRUCTURE_TYPE_BIND_HEAP_INFO_EXT,
			.heapRange{
				.address = descriptorHeapSamplers.deviceAddress,
				.size = descriptorHeapSamplers.size
			},
			// Put the reserved range after our descriptors, simplifies some calculations
			.reservedRangeOffset = descriptorHeapSamplers.size - descriptorHeapProperties.minSamplerHeapReservedRange,
			.reservedRangeSize = descriptorHeapProperties.minSamplerHeapReservedRange
		};
		vkCmdBindSamplerHeapEXT(cmdBuffer, &bindHeapInfoSamplers);

		PushConstantBlock references{};
		// Pass pointer to the global matrix via a buffer device address
		references.matrixReference = uniformBuffers[currentBuffer].deviceAddress;
		VkPushDataInfoEXT pushDataInfo{
			.sType = VK_STRUCTURE_TYPE_PUSH_DATA_INFO_EXT,
			.data = {.address = &references, .size = sizeof(PushConstantBlock) }
		};
		vkCmdPushDataEXT(cmdBuffer, &pushDataInfo);

		model.bindBuffers(cmdBuffer);
		auto &primitive = model.nodeFromName("cube")->mesh[0].primitives[0];
		for (uint32_t j = 0; j < 2; j++) {
			vkCmdDrawIndexed(cmdBuffer, primitive->indexCount, 1, primitive->firstIndex, 0, j);
		}

		drawUI(cmdBuffer);
		endDynamicRendering(cmdBuffer);
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
		if (overlay->comboBox("Sampler", &selectedSampler, samplerNames)) {
			updateUniformBuffers();
		}
	}
};

VULKAN_EXAMPLE_MAIN()
