/*
 * Vulkan Example - Using VK_KHR_dynamic_rendering with VK_KHR_dynamic_rendering_local_read to replace render- and subpasses
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
	PFN_vkCmdBeginRenderingKHR vkCmdBeginRenderingKHR{ VK_NULL_HANDLE };
	PFN_vkCmdEndRenderingKHR vkCmdEndRenderingKHR{ VK_NULL_HANDLE };
	PFN_vkCmdPipelineBarrier2KHR vkCmdPipelineBarrier2KHR{ VK_NULL_HANDLE };
	PFN_vkCmdSetRenderingInputAttachmentIndicesKHR vkCmdSetRenderingInputAttachmentIndicesKHR{ VK_NULL_HANDLE };

	VkPhysicalDeviceDynamicRenderingFeaturesKHR enabledDynamicRenderingFeaturesKHR{};
	VkPhysicalDeviceDynamicRenderingLocalReadFeaturesKHR enabledDynamicRenderingLocalReadFeaturesKHR{};
	VkPhysicalDeviceSynchronization2FeaturesKHR enabledSynchronization2FeaturesKHR{};

	vkglTF::Model scene;

	struct UniformData {
		glm::mat4 projection;
		glm::mat4 model;
		glm::mat4 view;
	} uniformData;
	std::array<vks::Buffer, maxConcurrentFrames> uniformBuffers;

	VkDescriptorSetLayout descriptorSetLayoutUniformBuffers{};
	std::array<VkDescriptorSet, maxConcurrentFrames> descriptorSets{};

	struct Pass {
		VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };
		VkPipeline pipeline{ VK_NULL_HANDLE };
		VkDescriptorSetLayout descriptorSetLayout{ VK_NULL_HANDLE };
		VkDescriptorSet descriptorSet{ VK_NULL_HANDLE };
	};
	struct Passes {
		Pass scene, composition;
	} passes;

	struct FrameBufferAttachment {
		VkImage image{ VK_NULL_HANDLE };
		VkDeviceMemory memory{ VK_NULL_HANDLE };
		VkImageView view{ VK_NULL_HANDLE };
		VkFormat format{ VK_FORMAT_UNDEFINED };
	};
	struct Attachments {
		FrameBufferAttachment positionDepth, normal, albedo;
	} attachments{};

	struct Light {
		glm::vec4 position{};
		glm::vec3 color{};
		float radius{};
	};
	std::array<Light, 96> lights;
	vks::Buffer lightsBuffer;

	std::vector<uint32_t> colorAttachmentInputIndices{};
	VkRenderingInputAttachmentIndexInfo renderingInputAttachmentIndexInfo{};

	VulkanExample() : VulkanExampleBase()
	{
		title = "Dynamic rendering local read";
		camera.type = Camera::CameraType::firstperson;
		camera.setPosition(glm::vec3(0.0f, 1.0f, 0.0f));
		camera.setRotation(glm::vec3(0.0f, -90.0f, 0.0f));
		camera.setPerspective(60.0f, (float)width / (float)height, 0.1f, 256.0f);

		enabledInstanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

		enabledDeviceExtensions.push_back(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
		enabledDeviceExtensions.push_back(VK_KHR_DYNAMIC_RENDERING_LOCAL_READ_EXTENSION_NAME);
		enabledDeviceExtensions.push_back(VK_KHR_MAINTENANCE2_EXTENSION_NAME);
		enabledDeviceExtensions.push_back(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);
		enabledDeviceExtensions.push_back(VK_KHR_MULTIVIEW_EXTENSION_NAME);
		enabledDeviceExtensions.push_back(VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME);
		enabledDeviceExtensions.push_back(VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME);

		// in addition to the extension, the feature needs to be explicitly enabled too by chaining the extension structure into device creation
		enabledDynamicRenderingLocalReadFeaturesKHR = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_LOCAL_READ_FEATURES_KHR,
			.dynamicRenderingLocalRead = VK_TRUE
		};

		enabledDynamicRenderingFeaturesKHR = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR,
			.pNext = &enabledDynamicRenderingLocalReadFeaturesKHR,
			.dynamicRendering = VK_TRUE
		};

		enabledSynchronization2FeaturesKHR = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR,
			.pNext = &enabledDynamicRenderingFeaturesKHR,
			.synchronization2 = VK_TRUE,
		};
		deviceCreatepNextChain = &enabledSynchronization2FeaturesKHR;
	}

	~VulkanExample()
	{
		if (device) {
			for (Pass pass : {passes.scene, passes.composition}) {
				if (pass.pipeline != VK_NULL_HANDLE) {
					vkDestroyPipeline(device, pass.pipeline, nullptr);
				}
				if (pass.pipelineLayout != VK_NULL_HANDLE) {
					vkDestroyPipelineLayout(device, pass.pipelineLayout, nullptr);
				}
				if (pass.descriptorSetLayout != VK_NULL_HANDLE) {
					vkDestroyDescriptorSetLayout(device, pass.descriptorSetLayout, nullptr);
				}
			}
			for (FrameBufferAttachment attachment : {attachments.albedo, attachments.normal, attachments.positionDepth}) {
				destroyAttachment(attachment);
			}			
			vkDestroyDescriptorSetLayout(device, descriptorSetLayoutUniformBuffers, nullptr);
			for (auto& buffer : uniformBuffers) {
				buffer.destroy();
			}
			lightsBuffer.destroy();
		}
	}

	// With VK_KHR_dynamic_rendering we no longer need a render pass, so skip the sample base render pass setup
	void setupRenderPass() override {}

	// With VK_KHR_dynamic_rendering we no longer need a frame buffer, so skip the sample base framebuffer setup
	void setupFrameBuffer() override {}

	void windowResized() override
	{
		if (resized) {
			createAttachments();
			// Dynamic rendering uses a new layout to make writes to attachments visible for reads via input attachments
			const VkImageLayout image_layout = VK_IMAGE_LAYOUT_RENDERING_LOCAL_READ_KHR;
			// Update descriptors (e.g. on resize)
			// The attachments will be used as input attachments for some of the passes in this sample
			std::vector<VkDescriptorImageInfo> descriptorImageInfos = {
				vks::initializers::descriptorImageInfo(VK_NULL_HANDLE, attachments.positionDepth.view, image_layout),
				vks::initializers::descriptorImageInfo(VK_NULL_HANDLE, attachments.normal.view, image_layout),
				vks::initializers::descriptorImageInfo(VK_NULL_HANDLE, attachments.albedo.view, image_layout),
			};
			std::vector<VkWriteDescriptorSet> writeDescriptorSets;
			for (size_t i = 0; i < descriptorImageInfos.size(); i++) {
				writeDescriptorSets.push_back(vks::initializers::writeDescriptorSet(passes.composition.descriptorSet, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, static_cast<uint32_t>(i), &descriptorImageInfos[i]));
			}
			vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
			resized = false;
		}
	}

	// Enable physical device features required for this example
	virtual void getEnabledFeatures()
	{
		// Enable anisotropic filtering if supported
		if (deviceFeatures.samplerAnisotropy) {
			enabledFeatures.samplerAnisotropy = VK_TRUE;
		};
	}

	void loadAssets()
	{
		const uint32_t glTFLoadingFlags = vkglTF::FileLoadingFlags::PreTransformVertices | vkglTF::FileLoadingFlags::PreMultiplyVertexColors | vkglTF::FileLoadingFlags::FlipY;
		scene.loadFromFile(getAssetPath() + "models/sponza/sponza.gltf", vulkanDevice, queue, glTFLoadingFlags);
	}

	void createAttachment(VkFormat format, VkImageUsageFlags usage, FrameBufferAttachment& attachment)
	{
		if (attachment.image != VK_NULL_HANDLE) {
			destroyAttachment(attachment);
		}

		VkImageAspectFlags aspectMask{};
		attachment.format = format;
		if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
			aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}
		if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
			aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		assert(aspectMask > 0);

		VkImageCreateInfo imageCI{
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = format,
			.extent = {.width = width, .height = height, .depth = 1 },
			.mipLevels = 1,
			.arrayLayers = 1,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = VK_IMAGE_TILING_OPTIMAL,
			.usage = usage | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		};

		VkMemoryAllocateInfo memAllocInfo = vks::initializers::memoryAllocateInfo();
		VkMemoryRequirements memReqs{};
		VK_CHECK_RESULT(vkCreateImage(device, &imageCI, nullptr, &attachment.image));
		vkGetImageMemoryRequirements(device, attachment.image, &memReqs);
		memAllocInfo.allocationSize = memReqs.size;
		memAllocInfo.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device, &memAllocInfo, nullptr, &attachment.memory));
		VK_CHECK_RESULT(vkBindImageMemory(device, attachment.image, attachment.memory, 0));

		VkImageViewCreateInfo imageViewCI{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = attachment.image,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = format,
			.subresourceRange = {.aspectMask = aspectMask, .baseMipLevel = 0, .levelCount = VK_REMAINING_MIP_LEVELS, .baseArrayLayer = 0, .layerCount = VK_REMAINING_ARRAY_LAYERS },
		};
		VK_CHECK_RESULT(vkCreateImageView(device, &imageViewCI, nullptr, &attachment.view));

		// Without render passes and their implicit layout transitions, we need to explicitly transition the attachments
		// We use a new layout introduced by this extension that makes writes to images visible via input attachments
		VkCommandBuffer cmdBuf = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
		VkImageMemoryBarrier2KHR imageMemoryBarrier{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR,
			.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
			.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
			.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT_KHR,
			.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.newLayout = VK_IMAGE_LAYOUT_RENDERING_LOCAL_READ_KHR,
			.image = attachment.image,
			.subresourceRange = imageViewCI.subresourceRange,
		};
		VkDependencyInfoKHR dependencyInfo{
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR,
			.imageMemoryBarrierCount = 1,
			.pImageMemoryBarriers = &imageMemoryBarrier
		};
		vkCmdPipelineBarrier2KHR(cmdBuf, &dependencyInfo);
		vulkanDevice->flushCommandBuffer(cmdBuf, queue);
	}

	void destroyAttachment(FrameBufferAttachment& attachment)
	{
		vkDestroyImageView(device, attachment.view, nullptr);
		vkDestroyImage(device, attachment.image, nullptr);
		vkFreeMemory(device, attachment.memory, nullptr);
		attachment = {};
	}

	void createAttachments()
	{
		// The deferred setup used in this sample stores positions, normals and albedo into separate attachments
		// In a real-world application one would try to pack as much information as possible into as small targets as possible to e.g. save bandwidth
		createAttachment(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, attachments.positionDepth);
		createAttachment(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, attachments.normal);
		createAttachment(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, attachments.albedo);
	}

	void setupDescriptors()
	{	
		// Pool
		std::vector<VkDescriptorPoolSize> poolSizes = {
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, maxConcurrentFrames * 2 * 2),
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, maxConcurrentFrames * 2 * 2),
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, maxConcurrentFrames * 2),
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, maxConcurrentFrames * 4 * 2),
		};
		VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, maxConcurrentFrames * 10);
		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));

		// Sets per frame for uniform buffers
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
		};
		VkDescriptorSetLayoutCreateInfo descriptorLayoutCI = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayoutCI, nullptr, &descriptorSetLayoutUniformBuffers));
		VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayoutUniformBuffers, 1);
		for (auto i = 0; i < uniformBuffers.size(); i++) {
			VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets[i]));
			std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
				vks::initializers::writeDescriptorSet(descriptorSets[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffers[i].descriptor),
			};
			vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
		}

		// Opaque scene pass only uses uniform buffers and glTF images, for which we already have a descriptor set

		VkDescriptorImageInfo texDescriptorPosition = vks::initializers::descriptorImageInfo(VK_NULL_HANDLE, attachments.positionDepth.view, VK_IMAGE_LAYOUT_RENDERING_LOCAL_READ_KHR);
		VkDescriptorImageInfo texDescriptorNormal = vks::initializers::descriptorImageInfo(VK_NULL_HANDLE, attachments.normal.view, VK_IMAGE_LAYOUT_RENDERING_LOCAL_READ_KHR);
		VkDescriptorImageInfo texDescriptorAlbedo = vks::initializers::descriptorImageInfo(VK_NULL_HANDLE, attachments.albedo.view, VK_IMAGE_LAYOUT_RENDERING_LOCAL_READ_KHR);

		// Composition pass
		setLayoutBindings = {
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT, 2),
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 3),
		};
		descriptorLayoutCI = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayoutCI, nullptr, &passes.composition.descriptorSetLayout));

		allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &passes.composition.descriptorSetLayout, 1);
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &passes.composition.descriptorSet));
		std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
			vks::initializers::writeDescriptorSet(passes.composition.descriptorSet, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 0, &texDescriptorPosition),
			vks::initializers::writeDescriptorSet(passes.composition.descriptorSet, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, &texDescriptorNormal),
			vks::initializers::writeDescriptorSet(passes.composition.descriptorSet, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 2, &texDescriptorAlbedo),
			vks::initializers::writeDescriptorSet(passes.composition.descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3, &lightsBuffer.descriptor),
		};
		vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
	}

	void preparePipelines()
	{
		// Pipeline
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
		VkPipelineRasterizationStateCreateInfo rasterizationState = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
		VkPipelineColorBlendAttachmentState blendAttachmentState = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
		VkPipelineColorBlendStateCreateInfo colorBlendState = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
		VkPipelineDepthStencilStateCreateInfo depthStencilState = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
		VkPipelineViewportStateCreateInfo viewportState = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
		VkPipelineMultisampleStateCreateInfo multisampleState = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
		std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicState = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};

		// We're using the same attachment configuraiton for multiple passes, but don't want to include the swap chain image in the composition pass
		// For that we need to configure how attachments map, and set the swapchain one to unsed (=ignore)
		colorAttachmentInputIndices = { VK_ATTACHMENT_UNUSED, 0, 1, 2 };
		renderingInputAttachmentIndexInfo = {
			.sType = VK_STRUCTURE_TYPE_RENDERING_INPUT_ATTACHMENT_INDEX_INFO_KHR,
			.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentInputIndices.size()),
			.pColorAttachmentInputIndices = colorAttachmentInputIndices.data()
		};

		// New create info to define color, depth and stencil attachments at pipeline create time
		std::vector<VkFormat> colorAttachmentFormats{
			swapChain.colorFormat,
			attachments.positionDepth.format,
			attachments.normal.format,
			attachments.albedo.format
		};
		VkPipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
			.pNext = &renderingInputAttachmentIndexInfo,
			.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentFormats.size()),
			.pColorAttachmentFormats = colorAttachmentFormats.data(),
			.depthAttachmentFormat = depthFormat,
			.stencilAttachmentFormat = depthFormat
		};

		// We no longer need to set a renderpass for the pipeline create info
		VkGraphicsPipelineCreateInfo pipelineCI{
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.pNext = &pipelineRenderingCreateInfo,
			.stageCount = static_cast<uint32_t>(shaderStages.size()),
			.pStages = shaderStages.data(),
			.pVertexInputState = vkglTF::Vertex::getPipelineVertexInputState({vkglTF::VertexComponent::Position, vkglTF::VertexComponent::Color, vkglTF::VertexComponent::Normal, vkglTF::VertexComponent::UV}),
			.pInputAssemblyState = &inputAssemblyState,
			.pViewportState = &viewportState,
			.pRasterizationState = &rasterizationState,
			.pMultisampleState = &multisampleState,
			.pDepthStencilState = &depthStencilState,
			.pColorBlendState = &colorBlendState,
			.pDynamicState = &dynamicState,
		};

		std::array<VkPipelineColorBlendAttachmentState, 4> blendAttachmentStates = {
			vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
			vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
			vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
			vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE)
		};

		colorBlendState.attachmentCount = static_cast<uint32_t>(blendAttachmentStates.size());
		colorBlendState.pAttachments = blendAttachmentStates.data();

		std::vector<VkDescriptorSetLayout> setLayouts{};
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};

		// Offscreen opaque scene parts
		setLayouts = { descriptorSetLayoutUniformBuffers, vkglTF::descriptorSetLayoutImage };
		pipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(setLayouts.data(), static_cast<uint32_t>(setLayouts.size()));
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &passes.scene.pipelineLayout));
		pipelineCI.layout = passes.scene.pipelineLayout;
		shaderStages[0] = loadShader(getShadersPath() + "dynamicrenderinglocalread/scene.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getShadersPath() + "dynamicrenderinglocalread/scene.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &passes.scene.pipeline));

		// Composition
		rasterizationState.cullMode = VK_CULL_MODE_NONE;
		depthStencilState.depthWriteEnable = VK_FALSE;
		depthStencilState.depthTestEnable = VK_FALSE;
		setLayouts = { passes.composition.descriptorSetLayout };
		pipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(setLayouts.data(), static_cast<uint32_t>(setLayouts.size()));
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &passes.composition.pipelineLayout));
		pipelineCI.layout = passes.composition.pipelineLayout;
		shaderStages[0] = loadShader(getShadersPath() + "dynamicrenderinglocalread/composition.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getShadersPath() + "dynamicrenderinglocalread/composition.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &passes.composition.pipeline));
	}

	// Prepare and initialize uniform buffer containing shader uniforms
	void prepareUniformBuffers()
	{
		for (auto& buffer : uniformBuffers) {
			VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &buffer, sizeof(UniformData), &uniformData));
			VK_CHECK_RESULT(buffer.map());
		}
		vulkanDevice->createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &lightsBuffer, lights.size() * sizeof(Light));
		VK_CHECK_RESULT(lightsBuffer.map());
		memcpy(lightsBuffer.mapped, lights.data(), lights.size() * sizeof(Light));
	}

	void updateUniformBuffers()
	{
		uniformData.projection = camera.matrices.perspective;
		uniformData.view = camera.matrices.view;
		uniformData.model = glm::mat4(1.0f);
		memcpy(uniformBuffers[currentBuffer].mapped, &uniformData, sizeof(uniformData));
	}

	void initLights()
	{
		std::vector<glm::vec3> colors = {
			glm::vec3(1.0f, 1.0f, 1.0f),
			glm::vec3(1.0f, 0.0f, 0.0f),
			glm::vec3(0.0f, 1.0f, 0.0f),
			glm::vec3(0.0f, 0.0f, 1.0f),
			glm::vec3(1.0f, 1.0f, 0.0f),
		};
		std::random_device rndDevice;
		std::default_random_engine rndGen(benchmark.active ? 0 : rndDevice());
		std::uniform_real_distribution<float> rndDist(-1.0f, 1.0f);
		std::uniform_real_distribution<float> rndCol(0.0f, 0.5f);
		for (auto& light : lights) {
			light.position = glm::vec4(rndDist(rndGen) * 10.0f, 0.25f + std::abs(rndDist(rndGen)) * 5.0f, rndDist(rndGen) * 4.0f, 1.0f);
			light.color = glm::vec3(rndCol(rndGen), rndCol(rndGen), rndCol(rndGen)) * 2.0f;
			light.radius = 1.0f + std::abs(rndDist(rndGen)) * 2.0f;
		}
	}

	void prepare()
	{
		VulkanExampleBase::prepare();
		// Since we use an extension, we need to expliclity load the function pointers for extension related Vulkan commands
		vkCmdBeginRenderingKHR = reinterpret_cast<PFN_vkCmdBeginRenderingKHR>(vkGetDeviceProcAddr(device, "vkCmdBeginRenderingKHR"));
		vkCmdEndRenderingKHR = reinterpret_cast<PFN_vkCmdEndRenderingKHR>(vkGetDeviceProcAddr(device, "vkCmdEndRenderingKHR"));
		vkCmdPipelineBarrier2KHR = reinterpret_cast<PFN_vkCmdPipelineBarrier2KHR>(vkGetDeviceProcAddr(device, "vkCmdPipelineBarrier2KHR"));
		vkCmdSetRenderingInputAttachmentIndicesKHR = reinterpret_cast<PFN_vkCmdSetRenderingInputAttachmentIndicesKHR>(vkGetDeviceProcAddr(device, "vkCmdSetRenderingInputAttachmentIndicesKHR"));

		initLights();
		createAttachments();
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
		VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));

		// With dynamic rendering there are no subpass dependencies, so we need to take care of proper layout transitions by using barriers
		vks::tools::insertImageMemoryBarrier(
			cmdBuffer,
			swapChain.images[currentImageIndex],
			0,
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
		vks::tools::insertImageMemoryBarrier(
			cmdBuffer,
			depthStencil.image,
			0,
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			VkImageSubresourceRange{ VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1 });

		// New structures are used to define the attachments used in dynamic rendering
		std::vector<VkImageView> imageViewList = { swapChain.imageViews[currentImageIndex], attachments.positionDepth.view, attachments.normal.view, attachments.albedo.view };
		std::vector<VkRenderingAttachmentInfoKHR> colorAttachments(imageViewList.size());
		for (auto i = 0; i < colorAttachments.size(); i++) {
			colorAttachments[i] = {
				.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
				.imageView = imageViewList[i],
				.imageLayout = VK_IMAGE_LAYOUT_RENDERING_LOCAL_READ_KHR,
				.resolveMode = VK_RESOLVE_MODE_NONE,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.clearValue = {.color = { 0.0f, 0.0f, 0.0f, 0.0f } }
			};
		}

		// A single depth stencil attachment info can be used, but they can also be specified separately.
		// When both are specified separately, the only requirement is that the image view is identical.			
		VkRenderingAttachmentInfoKHR depthStencilAttachment{
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
			.imageView = depthStencil.view,
			.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.clearValue = {.depthStencil = { 1.0f, 0 } }
		};

		VkRenderingInfoKHR renderingInfo{
			.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
			.renderArea = { 0, 0, width, height },
			.layerCount = 1,
			.colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size()),
			.pColorAttachments = colorAttachments.data(),
			.pDepthAttachment = &depthStencilAttachment,
			.pStencilAttachment = &depthStencilAttachment
		};

		vkCmdBeginRenderingKHR(cmdBuffer, &renderingInfo);
		VkViewport viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
		vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
		VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);
		vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

		std::vector<VkDescriptorSet> descSets{ descriptorSets[currentBuffer] };

		// First draw fills the G-Buffer attachments containing image data for the deferred composition (color+depth, normals, albedo)

		// We're using the same attachment configuraiton for multiple passes, but don't want to include the swap chain image in the composition pass
		// For that we need to configure how attachments map, and set the swapchain one to unsed (=ignore)
		// This call applies that information which was also set for the respective pipeline as command buffer state
		vkCmdSetRenderingInputAttachmentIndicesKHR(cmdBuffer, &renderingInputAttachmentIndexInfo);

		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, passes.scene.pipeline);
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, passes.scene.pipelineLayout, 0, static_cast<uint32_t>(descSets.size()), descSets.data(), 0, nullptr);
		scene.draw(cmdBuffer, vkglTF::RenderFlags::BindImages, passes.scene.pipelineLayout, 1);

		// A new feature of the dynamic rendering local read extension is the ability to use pipeline barriers in the dynamic render pass
		// to allow framebuffer-local dependencies (i.e. read-after-write) between draw calls using the "by region" flag
		// So with this barrier we can use the output attachments from the draw call above as input attachments in the next call
		VkMemoryBarrier2KHR memoryBarrier{
			.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2_KHR,
			.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
			.dstAccessMask = VK_ACCESS_2_INPUT_ATTACHMENT_READ_BIT
		};
		VkDependencyInfoKHR dependencyInfo{
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR,
			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
			.memoryBarrierCount = 1,
			.pMemoryBarriers = &memoryBarrier
		};
		vkCmdPipelineBarrier2KHR(cmdBuffer, &dependencyInfo);

		// Second draw will use the G-Buffer attachments that have been filled in the first draw as input attachment for the deferred scene composition
		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, passes.composition.pipeline);
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, passes.composition.pipelineLayout, 0, 1, &passes.composition.descriptorSet, 0, nullptr);
		vkCmdDraw(cmdBuffer, 3, 1, 0, 0);

		drawUI(cmdBuffer);

		vkCmdEndRenderingKHR(cmdBuffer);

		// This set of barriers prepares the swap chain image for presentation, we don't need to care for the depth image
		vks::tools::insertImageMemoryBarrier(
			cmdBuffer,
			swapChain.images[currentImageIndex],
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			0,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

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
};

VULKAN_EXAMPLE_MAIN()
