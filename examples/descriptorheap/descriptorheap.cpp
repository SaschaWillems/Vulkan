/*
 * Vulkan Example - Using descriptor heaps via VK_EXT_descriptor_heap
 * 
 * Descriptor heaps fundamentally rework how shader resources are bound. Descriptors are simply stored in buffers (heaps).
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
		vks::Texture2D texture;
		glm::vec3 rotation;
	};
	std::array<Cube, 2> cubes;

	struct UniformData {
		glm::mat4 projectionMatrix;
		glm::mat4 viewMatrix;
		glm::mat4 modelMatrix[2];
	} uniformData;

	int32_t selectedSampler{ 0 };
	vkglTF::Model model;

	VkPipeline pipeline{ nullptr };
	
	VkPhysicalDeviceDescriptorHeapFeaturesEXT enabledDeviceDescriptorHeapFeaturesEXT{};
	VkPhysicalDeviceBufferDeviceAddressFeatures enabledBufferDeviceAddressFeatures{};
	VkPhysicalDeviceSynchronization2Features enabledynchronization2Features{};

	VkPhysicalDeviceDescriptorHeapPropertiesEXT descriptorHeapProperties{};

	PFN_vkWriteResourceDescriptorsEXT vkWriteResourceDescriptorsEXT{ nullptr };
	PFN_vkCmdBindResourceHeapEXT vkCmdBindResourceHeapEXT{ nullptr };
	PFN_vkCmdBindSamplerHeapEXT vkCmdBindSamplerHeapEXT{ nullptr };
	PFN_vkWriteSamplerDescriptorsEXT vkWriteSamplerDescriptorsEXT{ nullptr };
	PFN_vkCmdPushDataEXT vkCmdPushDataEXT{ nullptr };
	PFN_vkGetPhysicalDeviceDescriptorSizeEXT vkGetPhysicalDeviceDescriptorSizeEXT{ nullptr };

	vks::Buffer descriptorHeapResources{};
	vks::Buffer descriptorHeapSamplers{};
	std::array<vks::Buffer, maxConcurrentFrames> uniformBuffers{};

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
		title = "Descriptor heaps (VK_EXT_descriptor_heap)";
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

		enabledynchronization2Features = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
			.pNext = &baseDynamicRenderingFeatures,
			.synchronization2 = VK_TRUE
		};

		enabledBufferDeviceAddressFeatures = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
			.pNext = &enabledynchronization2Features,
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
		if (device) {
			for (auto& uniformBuffer : uniformBuffers) {
				uniformBuffer.destroy();
			}
			descriptorHeapResources.destroy();
			descriptorHeapSamplers.destroy();
			vkDestroyBuffer(device, descriptorHeapResources.buffer, nullptr);
			vkDestroyBuffer(device, descriptorHeapSamplers.buffer, nullptr);
			vkDestroyPipeline(device, pipeline, nullptr);
			for (auto& cube : cubes) {
				cube.texture.destroy();
			}
		}
	}

	virtual void getEnabledFeatures()
	{
		if (deviceFeatures.samplerAnisotropy) {
			enabledFeatures.samplerAnisotropy = VK_TRUE;
		};
	}

	void prepareDescriptorHeaps()
	{
		// One buffer that per uniform buffers per frames-in-flight
		for (uint32_t i = 0; i < maxConcurrentFrames; i++) {
			VK_CHECK_RESULT(vulkanDevice->createBuffer(
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				&uniformBuffers[i],
				sizeof(UniformData)));
			uniformBuffers[i].map();
			getBufferDeviceAddress(uniformBuffers[i]);
		}

		// Descriptor heaps have varying offset, size and alignment requirements, so we store it's properties for later user
		VkPhysicalDeviceProperties2 deviceProps2{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
		descriptorHeapProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_HEAP_PROPERTIES_EXT;
		deviceProps2.pNext = &descriptorHeapProperties;
		vkGetPhysicalDeviceProperties2(physicalDevice, &deviceProps2);

		// There are two descriptor heap types: One that can store resources (buffers, images) and one that can store samplers
		// We create heaps with a fixed size that's guaranteed to fit in the few descriptors we use
		const VkDeviceSize heapbufferSize = vks::tools::alignedVkSize(2048 + descriptorHeapProperties.minResourceHeapReservedRange, descriptorHeapProperties.resourceHeapAlignment);
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_DESCRIPTOR_HEAP_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&descriptorHeapResources,
			heapbufferSize));
		descriptorHeapResources.map();
		getBufferDeviceAddress(descriptorHeapResources);

		const VkDeviceSize heapSizeSamplers = vks::tools::alignedVkSize(2048 + descriptorHeapProperties.minSamplerHeapReservedRange, descriptorHeapProperties.samplerHeapAlignment);
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_DESCRIPTOR_HEAP_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&descriptorHeapSamplers,
			heapSizeSamplers));
		descriptorHeapSamplers.map();
		getBufferDeviceAddress(descriptorHeapSamplers);

		// Sampler heap
		// We need to calculate some aligned offsets, heaps and strides to make sure we properly accress the descriptors
		samplerDescriptorSize = vks::tools::alignedVkSize(descriptorHeapProperties.samplerDescriptorSize, descriptorHeapProperties.samplerDescriptorAlignment);

		// No need to create an actual VkSampler, we can simply pass the create info that describes the sampler
		std::array<VkSamplerCreateInfo, 2> samplerCreateInfos{
			VkSamplerCreateInfo{
				.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
				.magFilter = VK_FILTER_LINEAR,
				.minFilter = VK_FILTER_LINEAR,
				.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
				.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
				.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
				.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
				.maxAnisotropy = 16.0f,
				.compareOp = VK_COMPARE_OP_NEVER,
				.maxLod = (float)cubes[0].texture.mipLevels,
			},
			VkSamplerCreateInfo{
				.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
				.magFilter = VK_FILTER_NEAREST,
				.minFilter = VK_FILTER_NEAREST,
				.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
				.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
				.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
				.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
				.maxAnisotropy = 16.0f,
				.compareOp = VK_COMPARE_OP_NEVER,
				.maxLod = (float)cubes[0].texture.mipLevels,
			}
		};

		VkHostAddressRangeEXT hostAddressRangeSamplers{
			.address = static_cast<uint8_t*>(descriptorHeapSamplers.mapped),
			.size = samplerDescriptorSize * static_cast<uint32_t>(samplerCreateInfos.size())
		};
		VK_CHECK_RESULT(vkWriteSamplerDescriptorsEXT(device, 1, samplerCreateInfos.data(), &hostAddressRangeSamplers));

		// Resource heap (buffers and images)
		bufferDescriptorSize = vks::tools::alignedVkSize(descriptorHeapProperties.bufferDescriptorSize, descriptorHeapProperties.bufferDescriptorAlignment);
		// Images are storted after the last buffer (aligned)
		imageHeapOffset = vks::tools::alignedVkSize(uniformBuffers.size() * bufferDescriptorSize, descriptorHeapProperties.imageDescriptorAlignment);
		imageDescriptorSize = vks::tools::alignedVkSize(descriptorHeapProperties.imageDescriptorSize, descriptorHeapProperties.imageDescriptorAlignment);

		auto vectorSize{ maxConcurrentFrames + cubes.size() };
		std::vector<VkHostAddressRangeEXT> hostAddressRangesResources(vectorSize);
		std::vector<VkResourceDescriptorInfoEXT> resourceDescriptorInfos(vectorSize);
		
		size_t heapResIndex{ 0 };

		// Buffer
		std::array<VkDeviceAddressRangeEXT, maxConcurrentFrames> deviceAddressRangesUniformBuffer{};
		for (auto i = 0; i < uniformBuffers.size(); i++) {
			deviceAddressRangesUniformBuffer[i] = { .address = uniformBuffers[i].deviceAddress, .size = uniformBuffers[i].size};
			resourceDescriptorInfos[heapResIndex] = {
				.sType = VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT,
				.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.data = {
					.pAddressRange = &deviceAddressRangesUniformBuffer[i]
				}
			};
			hostAddressRangesResources[heapResIndex] = {
				.address = static_cast<uint8_t*>(descriptorHeapResources.mapped) + bufferDescriptorSize * i,
				.size = bufferDescriptorSize
			};

			heapResIndex++;
		}

		// Images
		std::array<VkImageViewCreateInfo, 2> imageViewCreateInfos{};
		std::array<VkImageDescriptorInfoEXT, 2> imageDescriptorInfo{};

		// @offset
		for (auto i = 0; i < cubes.size(); i++) {
			imageViewCreateInfos[i] = {
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.image = cubes[i].texture.image,
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.format = cubes[i].texture.format,
				.subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = cubes[i].texture.mipLevels, .baseArrayLayer = 0, .layerCount = 1},
			};

			imageDescriptorInfo[i] = {
				.sType = VK_STRUCTURE_TYPE_IMAGE_DESCRIPTOR_INFO_EXT,
				.pView = &imageViewCreateInfos[i],
				.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			};

			resourceDescriptorInfos[heapResIndex] = {
				.sType = VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT,
				.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
				.data = {
					.pImage = &imageDescriptorInfo[i]
				}
			};

			hostAddressRangesResources[heapResIndex] = {
				.address = static_cast<uint8_t*>(descriptorHeapResources.mapped) + imageHeapOffset + imageDescriptorSize * i,
				.size = imageDescriptorSize
			};

			heapResIndex++;
		}

		VK_CHECK_RESULT(vkWriteResourceDescriptorsEXT(device, static_cast<uint32_t>(resourceDescriptorInfos.size()), resourceDescriptorInfos.data(), hostAddressRangesResources.data()));
	}

	void preparePipelines()
	{
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
			loadShader(getShadersPath() + "descriptorheap/cube.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
			loadShader(getShadersPath() + "descriptorheap/cube.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT)
		};

		// Descriptor heaps can be used without having to explicitly change the shaders
		// This is done by specifiying the bindings and their types at the shader stage level
		// As samplers require a different heap (than images), we can't use combined images

		std::array<VkDescriptorSetAndBindingMappingEXT, 3> setAndBindingMappings = {

			// Buffer binding
			VkDescriptorSetAndBindingMappingEXT{
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_AND_BINDING_MAPPING_EXT,
				.descriptorSet = 0,
				.firstBinding = 0,
				.bindingCount = 1,
				.resourceMask = VK_SPIRV_RESOURCE_TYPE_UNIFORM_BUFFER_BIT_EXT,
				.source = VK_DESCRIPTOR_MAPPING_SOURCE_HEAP_WITH_CONSTANT_OFFSET_EXT,
				.sourceData = {
					.constantOffset = {
						.heapArrayStride = static_cast<uint32_t>(bufferDescriptorSize)
					}
				}
			},

			// We are using multiple images, which requires us to set heapArrayStride to let the implementation know where image n+1 starts
			VkDescriptorSetAndBindingMappingEXT{
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_AND_BINDING_MAPPING_EXT,
				.descriptorSet = 1,
				.firstBinding = 0,
				.bindingCount = 1,
				.resourceMask = VK_SPIRV_RESOURCE_TYPE_SAMPLED_IMAGE_BIT_EXT,
				.source = VK_DESCRIPTOR_MAPPING_SOURCE_HEAP_WITH_CONSTANT_OFFSET_EXT,
				.sourceData = {
					.constantOffset = {
						.heapOffset = static_cast<uint32_t>(imageHeapOffset),
						.heapArrayStride = static_cast<uint32_t>(imageDescriptorSize)
					}
				}
			},

			// As samplers require a different heap (than images), we can't use combined images but split image and sampler
			VkDescriptorSetAndBindingMappingEXT{
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_AND_BINDING_MAPPING_EXT,
				.descriptorSet = 2,
				.firstBinding = 0,
				.bindingCount = 1,
				.resourceMask = VK_SPIRV_RESOURCE_TYPE_SAMPLER_BIT_EXT,
				.source = VK_DESCRIPTOR_MAPPING_SOURCE_HEAP_WITH_CONSTANT_OFFSET_EXT,
				.sourceData = {
					.constantOffset = {
						.heapOffset = static_cast<uint32_t>(samplerHeapOffset),
						.heapArrayStride = static_cast<uint32_t>(samplerDescriptorSize)
					}
				}
			}

		};

		VkShaderDescriptorSetAndBindingMappingInfoEXT descriptorSetAndBindingMappingInfo{
			.sType = VK_STRUCTURE_TYPE_SHADER_DESCRIPTOR_SET_AND_BINDING_MAPPING_INFO_EXT,
			.mappingCount = static_cast<uint32_t>(setAndBindingMappings.size()),
			.pMappings = setAndBindingMappings.data()
		};

		shaderStages[0].pNext = &descriptorSetAndBindingMappingInfo;
		shaderStages[1].pNext = &descriptorSetAndBindingMappingInfo;

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
		cubes[0].texture.loadFromFile(getAssetPath() + "textures/crate01_color_height_rgba.ktx", VK_FORMAT_R8G8B8A8_UNORM, vulkanDevice, queue);
		cubes[1].texture.loadFromFile(getAssetPath() + "textures/crate02_color_height_rgba.ktx", VK_FORMAT_R8G8B8A8_UNORM, vulkanDevice, queue);
	}

	void updateUniformBuffers()
	{		
		uniformData.projectionMatrix = camera.matrices.perspective;
		uniformData.viewMatrix = camera.matrices.view;
		std::array<glm::vec3, 2> positions = { glm::vec3(-2.0f, 0.0f, 0.0f), glm::vec3(1.5f, 0.5f, 0.0f) };
		for (auto i = 0; i < cubes.size(); i++) {
			glm::mat4 cubeMat = glm::translate(glm::mat4(1.0f), positions[i]);
			cubeMat = glm::rotate(cubeMat, glm::radians(cubes[i].rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
			cubeMat = glm::rotate(cubeMat, glm::radians(cubes[i].rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
			cubeMat = glm::rotate(cubeMat, glm::radians(cubes[i].rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
			cubeMat = glm::scale(cubeMat, glm::vec3(0.25f));
			uniformData.modelMatrix[i] = cubeMat;
		}
		memcpy(uniformBuffers[currentBuffer].mapped, &uniformData, sizeof(UniformData));
	}

	void prepare()
	{
		VulkanExampleBase::prepare();

		// Using descriptor heaps requires some extensions, and with that functions to be loaded explicitly
		vkWriteResourceDescriptorsEXT = reinterpret_cast<PFN_vkWriteResourceDescriptorsEXT>(vkGetDeviceProcAddr(device, "vkWriteResourceDescriptorsEXT"));
		vkCmdBindResourceHeapEXT = reinterpret_cast<PFN_vkCmdBindResourceHeapEXT>(vkGetDeviceProcAddr(device, "vkCmdBindResourceHeapEXT"));
		vkCmdBindSamplerHeapEXT = reinterpret_cast<PFN_vkCmdBindSamplerHeapEXT>(vkGetDeviceProcAddr(device, "vkCmdBindSamplerHeapEXT"));
		vkGetPhysicalDeviceDescriptorSizeEXT = reinterpret_cast<PFN_vkGetPhysicalDeviceDescriptorSizeEXT>(vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceDescriptorSizeEXT"));
		vkWriteSamplerDescriptorsEXT = reinterpret_cast<PFN_vkWriteSamplerDescriptorsEXT>(vkGetInstanceProcAddr(instance, "vkWriteSamplerDescriptorsEXT"));
		vkCmdPushDataEXT = reinterpret_cast<PFN_vkCmdPushDataEXT>(vkGetInstanceProcAddr(instance, "vkCmdPushDataEXT"));

		loadAssets();
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
		VkDeviceSize offsets[1] = { 0 };

		// Pass options as push data
		struct PushData {
			int32_t samplerIndex;
			int32_t frameIndex;
		} pushData = {
			.samplerIndex = selectedSampler,
			.frameIndex = static_cast<int32_t>(currentBuffer),
		};
		VkPushDataInfoEXT pushDataInfo{
			.sType = VK_STRUCTURE_TYPE_PUSH_DATA_INFO_EXT,
			.data = { .address = &pushData, .size = sizeof(PushData) }
		};
		vkCmdPushDataEXT(cmdBuffer, &pushDataInfo);

		// Bind the heap containing resources (buffers and images)
		VkBindHeapInfoEXT bindHeapInfoRes{
			.sType = VK_STRUCTURE_TYPE_BIND_HEAP_INFO_EXT,
			.heapRange{
				.address = descriptorHeapResources.deviceAddress,
				.size = descriptorHeapResources.size
			},
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
			.reservedRangeSize = descriptorHeapProperties.minSamplerHeapReservedRange
		};
		vkCmdBindSamplerHeapEXT(cmdBuffer, &bindHeapInfoSamplers);

		model.bindBuffers(cmdBuffer);
		auto &primitive = model.nodeFromName("cube")->mesh[0].primitives[0];
		for (uint32_t j = 0; j < static_cast<uint32_t>(cubes.size()); j++) {
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
		if (overlay->comboBox("Sampler", &selectedSampler, samplerNames)) {
			updateUniformBuffers();
		}
	}
};

VULKAN_EXAMPLE_MAIN()
