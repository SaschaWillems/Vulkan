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

// todo:
// FiF
// Dynamic rendering
// Cleanup

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
		int32_t selectedSampler{ 0 };
	} uniformData;

	vkglTF::Model model;

	VkPipeline pipeline{ nullptr };
	
	VkPhysicalDeviceDescriptorHeapFeaturesEXT enabledDeviceDescriptorHeapFeaturesEXT{};
	VkPhysicalDeviceBufferDeviceAddressFeatures enabledBufferDeviceAddressFeatures{};

	VkPhysicalDeviceDescriptorHeapPropertiesEXT descriptorHeapProperties{};

	PFN_vkGetPhysicalDeviceProperties2KHR vkGetPhysicalDeviceProperties2KHR{ nullptr };
	PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR{ nullptr };
	PFN_vkWriteResourceDescriptorsEXT vkWriteResourceDescriptorsEXT{ nullptr };
	PFN_vkCmdBindResourceHeapEXT vkCmdBindResourceHeapEXT{ nullptr };
	PFN_vkCmdBindSamplerHeapEXT vkCmdBindSamplerHeapEXT{ nullptr };
	PFN_vkWriteSamplerDescriptorsEXT vkWriteSamplerDescriptorsEXT{ nullptr };
	PFN_vkGetPhysicalDeviceDescriptorSizeEXT vkGetPhysicalDeviceDescriptorSizeEXT{ nullptr };

	vks::Buffer descriptorHeapResources{ nullptr };
	vks::Buffer descriptorHeapSamplers{ nullptr };
	// @todo: FiF
	vks::Buffer uniformBuffers{};

	VkDeviceSize bufferOffset{ 0 };
	VkDeviceSize sizeBuf{ 0 };
	VkDeviceSize imgOffset{ 0 };
	VkDeviceSize sizeImg{ 0 };
	VkDeviceSize samplerOffset{ 0 };

	std::vector<std::string> samplerNames{ "Linear", "Nearest" };

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
		vkDestroyBuffer(device, uniformBuffers.buffer, nullptr);
		vkDestroyBuffer(device, descriptorHeapResources.buffer, nullptr);
		vkDestroyBuffer(device, descriptorHeapSamplers.buffer, nullptr);
		vkDestroyPipeline(device, pipeline, nullptr);
		for (auto& cube : cubes) {
			cube.texture.destroy();
		}
	}

	virtual void getEnabledFeatures()
	{
		if (deviceFeatures.samplerAnisotropy) {
			enabledFeatures.samplerAnisotropy = VK_TRUE;
		};
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
			loadShader(getShadersPath() + "descriptorheap/cube.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
			loadShader(getShadersPath() + "descriptorheap/cube.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT)
		};

		// Descriptor heaps can be used without having to explicitly change the shaders
		// This is done by specifiying the bindings and their types at the shader stage level
		// As samplers require a different heap (than images), we can't use combined images

		std::array<VkDescriptorSetAndBindingMappingEXT, 3> setAndBindingMappings = {

			// Buffer binding is straigt forward
			VkDescriptorSetAndBindingMappingEXT{
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_AND_BINDING_MAPPING_EXT,
				.descriptorSet = 0,
				.firstBinding = 0,
				.bindingCount = 1,
				.resourceMask = VK_SPIRV_RESOURCE_TYPE_UNIFORM_BUFFER_BIT_EXT,
				.source = VK_DESCRIPTOR_MAPPING_SOURCE_HEAP_WITH_CONSTANT_OFFSET_EXT,
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
						.heapOffset = static_cast<uint32_t>(imgOffset),
						// @todo: align
						.heapArrayStride = static_cast<uint32_t>(descriptorHeapProperties.imageDescriptorSize)
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
						.heapOffset = static_cast<uint32_t>(samplerOffset),
						// @todo: align
						.heapArrayStride = static_cast<uint32_t>(descriptorHeapProperties.samplerDescriptorSize)
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

		// With descriptor heaps we no longer need a pipeline layout
		// This struct must be chained into pipeline creation to enable the use of heaps (allowing us to leave pipelineLayout empty)
		VkPipelineCreateFlags2CreateInfo pipelineCreateFlags2CI{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_CREATE_FLAGS_2_CREATE_INFO,
			.flags = VK_PIPELINE_CREATE_2_DESCRIPTOR_HEAP_BIT_EXT
		};
		pipelineCI.pNext = &pipelineCreateFlags2CI;

		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipeline));
	}

	void prepareDescriptorHeaps()
	{
		// @todo: move?

		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&uniformBuffers,
			sizeof(UniformData)));
		uniformBuffers.map();
		getBufferDeviceAddress(uniformBuffers);

		// Descriptor heaps have varying offset, size and alignment requirements, so we store it's properties for later user
		assert(vkGetPhysicalDeviceProperties2KHR);
		VkPhysicalDeviceProperties2KHR deviceProps2{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR };
		descriptorHeapProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_HEAP_PROPERTIES_EXT;
		deviceProps2.pNext = &descriptorHeapProperties;
		vkGetPhysicalDeviceProperties2KHR(physicalDevice, &deviceProps2);

		// There are two descriptor heap types: One that can store resources (buffers, images) and one that can store samplers

		const VkDeviceSize heapSizeBuf = vks::tools::alignedVkSize(512 * 4 + descriptorHeapProperties.minResourceHeapReservedRange, descriptorHeapProperties.resourceHeapAlignment);
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_DESCRIPTOR_HEAP_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&descriptorHeapResources,
			heapSizeBuf));
		descriptorHeapResources.map();
		getBufferDeviceAddress(descriptorHeapResources);

		const VkDeviceSize heapSizeSamplers = vks::tools::alignedVkSize(512 + descriptorHeapProperties.minSamplerHeapReservedRange, descriptorHeapProperties.samplerHeapAlignment);
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_DESCRIPTOR_HEAP_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&descriptorHeapSamplers,
			heapSizeSamplers));
		descriptorHeapSamplers.map();
		getBufferDeviceAddress(descriptorHeapSamplers);
	
		// Sampler heap
		// @todo: Multiple samplers?

		auto sizeSampler = vks::tools::alignedVkSize(descriptorHeapProperties.samplerDescriptorSize, descriptorHeapProperties.samplerDescriptorAlignment);
		auto sampStart = vks::tools::alignedVkSize(descriptorHeapProperties.minSamplerHeapReservedRange, descriptorHeapProperties.samplerDescriptorAlignment);
		samplerOffset = vks::tools::alignedVkSize(sampStart, descriptorHeapProperties.samplerDescriptorSize);

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
				.mipLodBias = 0.0f,
				.maxAnisotropy = 16.0f,
				.compareOp = VK_COMPARE_OP_NEVER,
				.minLod = 0.0f,
				.maxLod = (float)cubes[0].texture.mipLevels,
				.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
			},
			VkSamplerCreateInfo{
				.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
				.magFilter = VK_FILTER_NEAREST,
				.minFilter = VK_FILTER_NEAREST,
				.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
				.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
				.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
				.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
				.mipLodBias = 0.0f,
				.maxAnisotropy = 16.0f,
				.compareOp = VK_COMPARE_OP_NEVER,
				.minLod = 0.0f,
				.maxLod = (float)cubes[0].texture.mipLevels,
				.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
			}
		};

		VkHostAddressRangeEXT hostAddressRangeSamplers{
			.address = static_cast<uint8_t*>(descriptorHeapSamplers.mapped) + samplerOffset,
			.size = sizeSampler * static_cast<uint32_t>(samplerCreateInfos.size())
		};
		vkWriteSamplerDescriptorsEXT(device, 1, samplerCreateInfos.data(), &hostAddressRangeSamplers);

		// Resource heap (buffer and images)

		// @todo: FiF

		bufferOffset = 0;
		sizeBuf = vks::tools::alignedVkSize(descriptorHeapProperties.bufferDescriptorSize * 1, descriptorHeapProperties.bufferDescriptorAlignment);
		imgOffset = vks::tools::alignedVkSize(bufferOffset + sizeBuf, descriptorHeapProperties.imageDescriptorAlignment);
		sizeImg = vks::tools::alignedVkSize(descriptorHeapProperties.imageDescriptorSize * 2, descriptorHeapProperties.imageDescriptorAlignment);
		auto sizeSingleImg = vks::tools::alignedVkSize(descriptorHeapProperties.imageDescriptorSize, descriptorHeapProperties.imageDescriptorAlignment);

		// Uniform buffers

		std::array<VkHostAddressRangeEXT, 3> vharsRes;
		vharsRes[0] = {
			.address = static_cast<uint8_t*>(descriptorHeapResources.mapped),
			.size = sizeBuf
		};
		vharsRes[1] = {
			.address = static_cast<uint8_t*>(descriptorHeapResources.mapped) + imgOffset,
			.size = sizeImg
		};
		vharsRes[2] = {
			.address = static_cast<uint8_t*>(descriptorHeapResources.mapped) + imgOffset + sizeSingleImg,
			.size = sizeImg
		};

		VkDeviceAddressRangeEXT darBuf = { uniformBuffers.deviceAddress, uniformBuffers.size };
		VkResourceDescriptorInfoEXT rdiBuf = {
			.sType = VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT,
			.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.data = {
				.pAddressRange = &darBuf
			}
		};

		// Images
		std::array<VkImageViewCreateInfo, 2> vcis{};
		std::array<VkImageDescriptorInfoEXT, 2> idis{};
		std::array<VkResourceDescriptorInfoEXT, 2> rdisImg{};

		for (uint32_t i = 0; i < static_cast<uint32_t>(cubes.size()); i++) {
			vcis[i] = {
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.image = cubes[i].texture.image,
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.format = cubes[i].texture.format,
				.subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = cubes[i].texture.mipLevels, .baseArrayLayer = 0, .layerCount = 1},
			};

			idis[i] = {
				.sType = VK_STRUCTURE_TYPE_IMAGE_DESCRIPTOR_INFO_EXT,
				.pView = &vcis[i],
				.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			};

			rdisImg[i] = {
				.sType = VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT,
				.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
				.data = {
					.pImage = &idis[i]
				}
			};
		}

		std::vector<VkResourceDescriptorInfoEXT> rdis{ rdiBuf, rdisImg[0], rdisImg[1] };
		vkWriteResourceDescriptorsEXT(device, static_cast<uint32_t>(rdis.size()), rdis.data(), vharsRes.data());
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
		memcpy(uniformBuffers.mapped, &uniformData, sizeof(UniformData));
	}

	void prepare()
	{
		VulkanExampleBase::prepare();

		// Using descriptor heaps requires some extensions, and with that functions to be loaded explicitly
		vkGetPhysicalDeviceProperties2KHR = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2KHR>(vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceProperties2KHR"));
		vkGetBufferDeviceAddressKHR = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(vkGetDeviceProcAddr(device, "vkGetBufferDeviceAddressKHR"));
		vkWriteResourceDescriptorsEXT = reinterpret_cast<PFN_vkWriteResourceDescriptorsEXT>(vkGetDeviceProcAddr(device, "vkWriteResourceDescriptorsEXT"));
		vkCmdBindResourceHeapEXT = reinterpret_cast<PFN_vkCmdBindResourceHeapEXT>(vkGetDeviceProcAddr(device, "vkCmdBindResourceHeapEXT"));
		vkCmdBindSamplerHeapEXT = reinterpret_cast<PFN_vkCmdBindSamplerHeapEXT>(vkGetDeviceProcAddr(device, "vkCmdBindSamplerHeapEXT"));
		vkGetPhysicalDeviceDescriptorSizeEXT = reinterpret_cast<PFN_vkGetPhysicalDeviceDescriptorSizeEXT>(vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceDescriptorSizeEXT"));
		vkWriteSamplerDescriptorsEXT = reinterpret_cast<PFN_vkWriteSamplerDescriptorsEXT>(vkGetInstanceProcAddr(instance, "vkWriteSamplerDescriptorsEXT"));

		loadAssets();
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

		// Res Heap
		// @todo: offset per fif
		// Bind the heap containing resources (buffers and images)
		VkBindHeapInfoEXT bindHeapInfoRes{
			.sType = VK_STRUCTURE_TYPE_BIND_HEAP_INFO_EXT,
			.heapRange{
				.address = descriptorHeapResources.deviceAddress,
				.size = descriptorHeapResources.size
			},
			.reservedRangeSize = descriptorHeapProperties.minResourceHeapReservedRange
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
		if (overlay->comboBox("Sampler", &uniformData.selectedSampler, samplerNames)) {
			updateUniformBuffers();
		}
	}
};

VULKAN_EXAMPLE_MAIN()
