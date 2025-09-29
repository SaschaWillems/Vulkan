/*
 * Vulkan Example - Using subpasses for G-Buffer compositing
 *
 * Copyright (C) 2016-2025 by Sascha Willems - www.saschawillems.de
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 *
 * Summary:
 * Implements a deferred rendering setup with a forward transparency pass using sub passes
 *
 * Sub passes allow reading from the previous framebuffer (in the same render pass) at
 * the same pixel position.
 *
 * This is a feature that was especially designed for tile-based-renderers
 * (mostly mobile GPUs) and is a new optimization feature in Vulkan for those GPU types.
 *
 */

#include "vulkanexamplebase.h"
#include "VulkanglTFModel.h"

class VulkanExample : public VulkanExampleBase
{
public:
	struct {
		vks::Texture2D glass;
	} textures;

	struct {
		vkglTF::Model scene;
		vkglTF::Model transparent;
	} models;

	struct {
		glm::mat4 projection;
		glm::mat4 model;
		glm::mat4 view;
	} uboGBuffer;

	struct Light {
		glm::vec4 position;
		glm::vec3 color;
		float radius;
	};

	std::array<Light, 64> lights;

	struct UniformBuffers {
		vks::Buffer GBuffer;
		vks::Buffer lights;
	};
	std::array<UniformBuffers, maxConcurrentFrames> uniformBuffers;

	struct {
		VkPipeline offscreen;
		VkPipeline composition;
		VkPipeline transparent;
	} pipelines;

	struct {
		VkPipelineLayout offscreen;
		VkPipelineLayout composition;
		VkPipelineLayout transparent;
	} pipelineLayouts;

	struct {
		VkDescriptorSetLayout scene;
		VkDescriptorSetLayout composition;
		VkDescriptorSetLayout transparent;
	} descriptorSetLayouts;

	struct DescriptorSets {
		VkDescriptorSet scene;
		VkDescriptorSet composition;
		VkDescriptorSet transparent;
	};
	std::array<DescriptorSets, maxConcurrentFrames> descriptorSets;

	// G-Buffer framebuffer attachments
	struct FrameBufferAttachment {
		VkImage image = VK_NULL_HANDLE;
		VkDeviceMemory mem = VK_NULL_HANDLE;
		VkImageView view = VK_NULL_HANDLE;
		VkFormat format;
	};
	struct Attachments {
		FrameBufferAttachment position, normal, albedo;
		int32_t width;
		int32_t height;
	} attachments;

	VulkanExample() : VulkanExampleBase()
	{
		title = "Subpasses";
		camera.type = Camera::CameraType::firstperson;
		camera.movementSpeed = 5.0f;
#ifndef __ANDROID__
		camera.rotationSpeed = 0.25f;
#endif
		camera.setPosition(glm::vec3(-3.2f, 1.0f, 5.9f));
		camera.setRotation(glm::vec3(0.5f, 210.05f, 0.0f));
		camera.setPerspective(60.0f, (float)width / (float)height, 0.1f, 256.0f);
		ui.subpass = 2;

		enabledFeatures.fragmentStoresAndAtomics = VK_TRUE;
	}

	~VulkanExample()
	{
		if (device) {
			vkDestroyPipeline(device, pipelines.offscreen, nullptr);
			vkDestroyPipeline(device, pipelines.composition, nullptr);
			vkDestroyPipeline(device, pipelines.transparent, nullptr);
			vkDestroyPipelineLayout(device, pipelineLayouts.offscreen, nullptr);
			vkDestroyPipelineLayout(device, pipelineLayouts.composition, nullptr);
			vkDestroyPipelineLayout(device, pipelineLayouts.transparent, nullptr);
			vkDestroyDescriptorSetLayout(device, descriptorSetLayouts.scene, nullptr);
			vkDestroyDescriptorSetLayout(device, descriptorSetLayouts.composition, nullptr);
			vkDestroyDescriptorSetLayout(device, descriptorSetLayouts.transparent, nullptr);
			clearAttachment(&attachments.position);
			clearAttachment(&attachments.normal);
			clearAttachment(&attachments.albedo);
			textures.glass.destroy();
			for (auto& buffer : uniformBuffers) {
				buffer.GBuffer.destroy();
				buffer.lights.destroy();
			}
		}
	}

	// Enable physical device features required for this example
	virtual void getEnabledFeatures()
	{
		// Enable anisotropic filtering if supported
		if (deviceFeatures.samplerAnisotropy) {
			enabledFeatures.samplerAnisotropy = VK_TRUE;
		}
	};

	void clearAttachment(FrameBufferAttachment* attachment)
	{
		vkDestroyImageView(device, attachment->view, nullptr);
		vkDestroyImage(device, attachment->image, nullptr);
		vkFreeMemory(device, attachment->mem, nullptr);
	}

	// Create a frame buffer attachment
	void createAttachment(VkFormat format, VkImageUsageFlags usage, FrameBufferAttachment *attachment)
	{
		if (attachment->image != VK_NULL_HANDLE) {
			clearAttachment(attachment);
		}

		VkImageAspectFlags aspectMask = 0;

		attachment->format = format;

		if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
		{
			aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}
		if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
		{
			aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		}

		assert(aspectMask > 0);

		VkImageCreateInfo image = vks::initializers::imageCreateInfo();
		image.imageType = VK_IMAGE_TYPE_2D;
		image.format = format;
		image.extent.width = attachments.width;
		image.extent.height = attachments.height;
		image.extent.depth = 1;
		image.mipLevels = 1;
		image.arrayLayers = 1;
		image.samples = VK_SAMPLE_COUNT_1_BIT;
		image.tiling = VK_IMAGE_TILING_OPTIMAL;
		// VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT flag is required for input attachments
		image.usage = usage | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
		image.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		VkMemoryAllocateInfo memAlloc = vks::initializers::memoryAllocateInfo();
		VkMemoryRequirements memReqs;

		VK_CHECK_RESULT(vkCreateImage(device, &image, nullptr, &attachment->image));
		vkGetImageMemoryRequirements(device, attachment->image, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		memAlloc.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &attachment->mem));
		VK_CHECK_RESULT(vkBindImageMemory(device, attachment->image, attachment->mem, 0));

		VkImageViewCreateInfo imageView = vks::initializers::imageViewCreateInfo();
		imageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageView.format = format;
		imageView.subresourceRange = {};
		imageView.subresourceRange.aspectMask = aspectMask;
		imageView.subresourceRange.baseMipLevel = 0;
		imageView.subresourceRange.levelCount = 1;
		imageView.subresourceRange.baseArrayLayer = 0;
		imageView.subresourceRange.layerCount = 1;
		imageView.image = attachment->image;
		VK_CHECK_RESULT(vkCreateImageView(device, &imageView, nullptr, &attachment->view));
	}

	// Create color attachments for the G-Buffer components
	void createGBufferAttachments()
	{
		createAttachment(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &attachments.position);	// (World space) Positions
		createAttachment(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &attachments.normal);		// (World space) Normals
		createAttachment(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &attachments.albedo);			// Albedo (color)
	}

	// Override framebuffer setup from base class, will automatically be called upon setup and if a window is resized
	void setupFrameBuffer()
	{
		// If the window is resized, all the framebuffers/attachments used in our composition passes need to be recreated
		if (attachments.width != width || attachments.height != height) {
			attachments.width = width;
			attachments.height = height;
			createGBufferAttachments();
			// Since the framebuffers/attachments are referred in the descriptor sets, these need to be updated too
			VkDescriptorImageInfo texDescriptorPosition = vks::initializers::descriptorImageInfo(VK_NULL_HANDLE, attachments.position.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			VkDescriptorImageInfo texDescriptorNormal = vks::initializers::descriptorImageInfo(VK_NULL_HANDLE, attachments.normal.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			VkDescriptorImageInfo texDescriptorAlbedo = vks::initializers::descriptorImageInfo(VK_NULL_HANDLE, attachments.albedo.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			for (auto i = 0; i < uniformBuffers.size(); i++) {
				// Composition pass
				std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
					// Transparent (forward) pipeline
					vks::initializers::writeDescriptorSet(descriptorSets[i].transparent, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, &texDescriptorPosition),
					// Composition pass
					vks::initializers::writeDescriptorSet(descriptorSets[i].composition, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 0, &texDescriptorPosition),
					vks::initializers::writeDescriptorSet(descriptorSets[i].composition, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, &texDescriptorNormal),
					vks::initializers::writeDescriptorSet(descriptorSets[i].composition, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 2, &texDescriptorAlbedo),
				};
				vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
			}
		}

		VkImageView attachments[5]{};

		VkFramebufferCreateInfo frameBufferCreateInfo = {};
		frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frameBufferCreateInfo.renderPass = renderPass;
		frameBufferCreateInfo.attachmentCount = 5;
		frameBufferCreateInfo.pAttachments = attachments;
		frameBufferCreateInfo.width = width;
		frameBufferCreateInfo.height = height;
		frameBufferCreateInfo.layers = 1;

		// Create frame buffers for every swap chain image
		frameBuffers.resize(swapChain.images.size());
		for (uint32_t i = 0; i < frameBuffers.size(); i++)
		{
			attachments[0] = swapChain.imageViews[i];
			attachments[1] = this->attachments.position.view;
			attachments[2] = this->attachments.normal.view;
			attachments[3] = this->attachments.albedo.view;
			attachments[4] = depthStencil.view;
			VK_CHECK_RESULT(vkCreateFramebuffer(device, &frameBufferCreateInfo, nullptr, &frameBuffers[i]));
		}
	}

	// Override render pass setup from base class
	void setupRenderPass()
	{
		attachments.width = width;
		attachments.height = height;

		createGBufferAttachments();

		std::array<VkAttachmentDescription, 5> attachments{};
		// Color attachment
		attachments[0].format = swapChain.colorFormat;
		attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		// Deferred attachments
		// Position
		attachments[1].format = this->attachments.position.format;
		attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[1].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		// Normals
		attachments[2].format = this->attachments.normal.format;
		attachments[2].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[2].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		// Albedo
		attachments[3].format = this->attachments.albedo.format;
		attachments[3].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[3].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[3].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[3].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[3].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[3].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[3].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		// Depth attachment
		attachments[4].format = depthFormat;
		attachments[4].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[4].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[4].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[4].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[4].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[4].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[4].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		// Three subpasses
		std::array<VkSubpassDescription,3> subpassDescriptions{};

		// First subpass: Fill G-Buffer components
		// ----------------------------------------------------------------------------------------

		VkAttachmentReference colorReferences[4]{};
		colorReferences[0] = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
		colorReferences[1] = { 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
		colorReferences[2] = { 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
		colorReferences[3] = { 3, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
		VkAttachmentReference depthReference = { 4, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

		subpassDescriptions[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescriptions[0].colorAttachmentCount = 4;
		subpassDescriptions[0].pColorAttachments = colorReferences;
		subpassDescriptions[0].pDepthStencilAttachment = &depthReference;

		// Second subpass: Final composition (using G-Buffer components)
		// ----------------------------------------------------------------------------------------

		VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

		VkAttachmentReference inputReferences[3]{};
		inputReferences[0] = { 1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
		inputReferences[1] = { 2, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
		inputReferences[2] = { 3, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

		subpassDescriptions[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescriptions[1].colorAttachmentCount = 1;
		subpassDescriptions[1].pColorAttachments = &colorReference;
		subpassDescriptions[1].pDepthStencilAttachment = &depthReference;
		// Use the color attachments filled in the first pass as input attachments
		subpassDescriptions[1].inputAttachmentCount = 3;
		subpassDescriptions[1].pInputAttachments = inputReferences;

		// Third subpass: Forward transparency
		// ----------------------------------------------------------------------------------------
		colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

		inputReferences[0] = { 1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

		subpassDescriptions[2].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescriptions[2].colorAttachmentCount = 1;
		subpassDescriptions[2].pColorAttachments = &colorReference;
		subpassDescriptions[2].pDepthStencilAttachment = &depthReference;
		// Use the color/depth attachments filled in the first pass as input attachments
		subpassDescriptions[2].inputAttachmentCount = 1;
		subpassDescriptions[2].pInputAttachments = inputReferences;

		// Subpass dependencies for layout transitions
		std::array<VkSubpassDependency, 5> dependencies{};

		// This makes sure that writes to the depth image are done before we try to write to it again
		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;;
		dependencies[0].srcAccessMask = 0;
		dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[0].dependencyFlags = 0;

		dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].dstSubpass = 0;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].srcAccessMask = 0;
		dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[1].dependencyFlags = 0;

		// This dependency transitions the input attachment from color attachment to input attachment read
		dependencies[2].srcSubpass = 0;
		dependencies[2].dstSubpass = 1;
		dependencies[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[2].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[2].dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
		dependencies[2].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		dependencies[3].srcSubpass = 1;
		dependencies[3].dstSubpass = 2;
		dependencies[3].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[3].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[3].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[3].dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
		dependencies[3].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		dependencies[4].srcSubpass = 2;
		dependencies[4].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[4].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[4].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[4].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[4].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[4].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = static_cast<uint32_t>(subpassDescriptions.size());
		renderPassInfo.pSubpasses = subpassDescriptions.data();
		renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
		renderPassInfo.pDependencies = dependencies.data();

		VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));
	}

	void loadAssets()
	{
		const uint32_t glTFLoadingFlags = vkglTF::FileLoadingFlags::PreTransformVertices | vkglTF::FileLoadingFlags::PreMultiplyVertexColors | vkglTF::FileLoadingFlags::FlipY;
		models.scene.loadFromFile(getAssetPath() + "models/samplebuilding.gltf", vulkanDevice, queue, glTFLoadingFlags);
		models.transparent.loadFromFile(getAssetPath() + "models/samplebuilding_glass.gltf", vulkanDevice, queue, glTFLoadingFlags);
		textures.glass.loadFromFile(getAssetPath() + "textures/colored_glass_rgba.ktx", VK_FORMAT_R8G8B8A8_UNORM, vulkanDevice, queue);
	}

	void setupDescriptors()
	{
		// Pool
		std::vector<VkDescriptorPoolSize> poolSizes = {
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, maxConcurrentFrames * 2),
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, maxConcurrentFrames * 2),
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, maxConcurrentFrames),
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, maxConcurrentFrames * 4),
		};
		VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo( static_cast<uint32_t>(poolSizes.size()), poolSizes.data(), maxConcurrentFrames * 4);
		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));

		// Layouts

		// Offscreen scene rendering
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0)
		};
		VkDescriptorSetLayoutCreateInfo descriptorLayoutCI = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayoutCI, nullptr, &descriptorSetLayouts.scene));

		// Composition pass
		setLayoutBindings = {
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT, 2),
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 3),
		};
		descriptorLayoutCI = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayoutCI, nullptr, &descriptorSetLayouts.composition));

		// Transparent (forward) pipeline
		setLayoutBindings = {
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2),
		};
		descriptorLayoutCI = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayoutCI, nullptr, &descriptorSetLayouts.transparent));

		// Image descriptors for the offscreen color attachments
		VkDescriptorImageInfo texDescriptorPosition = vks::initializers::descriptorImageInfo(VK_NULL_HANDLE, attachments.position.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		VkDescriptorImageInfo texDescriptorNormal = vks::initializers::descriptorImageInfo(VK_NULL_HANDLE, attachments.normal.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		VkDescriptorImageInfo texDescriptorAlbedo = vks::initializers::descriptorImageInfo(VK_NULL_HANDLE, attachments.albedo.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		// Sets per frame, just like the buffers themselves
		// Images and static buffers do not need to be duplicated per frame, we reuse the same one for each frame
		for (auto i = 0; i < uniformBuffers.size(); i++) {		
			// Scene
			VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayouts.scene, 1);
			VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets[i].scene));
			std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
				// Binding 0: Vertex shader uniform buffer
				vks::initializers::writeDescriptorSet(descriptorSets[i].scene, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffers[i].GBuffer.descriptor)
			};
			vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
			
			// Transparent (forward) pipeline
			allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayouts.transparent, 1);
			VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets[i].transparent));
			writeDescriptorSets = {
				vks::initializers::writeDescriptorSet(descriptorSets[i].transparent, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffers[i].GBuffer.descriptor),
				vks::initializers::writeDescriptorSet(descriptorSets[i].transparent, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, &texDescriptorPosition),
				vks::initializers::writeDescriptorSet(descriptorSets[i].transparent, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, &textures.glass.descriptor),
			};
			vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, NULL);
			
			// Composition pass
			allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayouts.composition, 1);
			VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets[i].composition));
			writeDescriptorSets = {
				vks::initializers::writeDescriptorSet(descriptorSets[i].composition, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 0, &texDescriptorPosition),
				vks::initializers::writeDescriptorSet(descriptorSets[i].composition, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, &texDescriptorNormal),
				vks::initializers::writeDescriptorSet(descriptorSets[i].composition, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 2, &texDescriptorAlbedo),
				vks::initializers::writeDescriptorSet(descriptorSets[i].composition, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3, &uniformBuffers[i].lights.descriptor),
			};
			vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, NULL);
		}
	}

	void preparePipelines()
	{
		// Layouts
		// Offscreen scene rendering
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(&descriptorSetLayouts.scene, 1);
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayouts.offscreen));
		// Transparent (forward) pipeline
		pipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(&descriptorSetLayouts.transparent, 1);
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayouts.transparent));
		// Composition pass
		pipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(&descriptorSetLayouts.composition, 1);
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayouts.composition));

		// Pipelines
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
		VkPipelineRasterizationStateCreateInfo rasterizationState = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
		VkPipelineColorBlendAttachmentState blendAttachmentState = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
		VkPipelineColorBlendStateCreateInfo colorBlendState = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
		VkPipelineDepthStencilStateCreateInfo depthStencilState = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
		VkPipelineViewportStateCreateInfo viewportState = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
		VkPipelineMultisampleStateCreateInfo multisampleState = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
		std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicState = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};

		// Final fullscreen pass pipeline
		VkGraphicsPipelineCreateInfo pipelineCI = vks::initializers::pipelineCreateInfo(pipelineLayouts.offscreen, renderPass, 0);
		pipelineCI.pInputAssemblyState = &inputAssemblyState;
		pipelineCI.pRasterizationState = &rasterizationState;
		pipelineCI.pColorBlendState = &colorBlendState;
		pipelineCI.pMultisampleState = &multisampleState;
		pipelineCI.pViewportState = &viewportState;
		pipelineCI.pDepthStencilState = &depthStencilState;
		pipelineCI.pDynamicState = &dynamicState;
		pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineCI.pStages = shaderStages.data();
		pipelineCI.subpass = 0;
		pipelineCI.pVertexInputState = vkglTF::Vertex::getPipelineVertexInputState({vkglTF::VertexComponent::Position, vkglTF::VertexComponent::Color, vkglTF::VertexComponent::Normal, vkglTF::VertexComponent::UV});

		std::array<VkPipelineColorBlendAttachmentState, 4> blendAttachmentStates = {
			vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
			vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
			vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
			vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE)
		};

		colorBlendState.attachmentCount = static_cast<uint32_t>(blendAttachmentStates.size());
		colorBlendState.pAttachments = blendAttachmentStates.data();

		// Offscreen scene rendering pipeline
		shaderStages[0] = loadShader(getShadersPath() + "subpasses/gbuffer.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getShadersPath() + "subpasses/gbuffer.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.offscreen));

		// Composition pass
		VkPipelineVertexInputStateCreateInfo emptyInputState{ .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
		colorBlendState.attachmentCount = 1;
		rasterizationState.cullMode = VK_CULL_MODE_NONE;
		pipelineCI.pVertexInputState = &emptyInputState;
		pipelineCI.layout = pipelineLayouts.composition;
		// Used in first subpass
		pipelineCI.subpass = 1;
		depthStencilState.depthWriteEnable = VK_FALSE;
		shaderStages[0] = loadShader(getShadersPath() + "subpasses/composition.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getShadersPath() + "subpasses/composition.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.composition));

		// Transparent forward pass
		// Uses blending
		blendAttachmentState.blendEnable = VK_TRUE;
		blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
		blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
		blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendState.attachmentCount = 1;
		colorBlendState.pAttachments = &blendAttachmentState;
		pipelineCI.pVertexInputState = vkglTF::Vertex::getPipelineVertexInputState({ vkglTF::VertexComponent::Position, vkglTF::VertexComponent::Color, vkglTF::VertexComponent::Normal, vkglTF::VertexComponent::UV });
		pipelineCI.layout = pipelineLayouts.transparent;
		// Used in second subpass
		pipelineCI.subpass = 2;
		shaderStages[0] = loadShader(getShadersPath() + "subpasses/transparent.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getShadersPath() + "subpasses/transparent.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.transparent));
	}

	// Prepare and initialize uniform buffer containing shader uniforms
	void prepareUniformBuffers()
	{
		for (auto& buffer : uniformBuffers) {
			// Matrices
			vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &buffer.GBuffer, sizeof(uboGBuffer));
			VK_CHECK_RESULT(buffer.GBuffer.map());
			// Lights
			vulkanDevice->createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &buffer.lights, lights.size() * sizeof(Light));
			VK_CHECK_RESULT(buffer.lights.map());
		}
	}

	void updateUniformBuffers()
	{
		memcpy(uniformBuffers[currentBuffer].lights.mapped, lights.data(), lights.size() * sizeof(Light));
		uboGBuffer.projection = camera.matrices.perspective;
		uboGBuffer.view = camera.matrices.view;
		uboGBuffer.model = glm::mat4(1.0f);
		memcpy(uniformBuffers[currentBuffer].GBuffer.mapped, &uboGBuffer, sizeof(uboGBuffer));
	}

	void initLights()
	{
		std::vector<glm::vec3> colors =
		{
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
			light.position = glm::vec4(rndDist(rndGen) * 8.0f, 0.25f + std::abs(rndDist(rndGen)) * 4.0f, rndDist(rndGen) * 8.0f, 1.0f);
			//light.color = colors[rndCol(rndGen)];
			light.color = glm::vec3(rndCol(rndGen), rndCol(rndGen), rndCol(rndGen)) * 2.0f;
			light.radius = 1.0f + std::abs(rndDist(rndGen));
		}
	}

	void prepare()
	{
		VulkanExampleBase::prepare();
		loadAssets();
		prepareUniformBuffers();
		initLights();
		setupDescriptors();
		preparePipelines();
		prepared = true;
	}

	void buildCommandBuffer()
	{
		VkCommandBuffer cmdBuffer = drawCmdBuffers[currentBuffer];
		
		VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

		VkClearValue clearValues[5]{};
		clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
		clearValues[1].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
		clearValues[2].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
		clearValues[3].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
		clearValues[4].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
		renderPassBeginInfo.renderPass = renderPass;
		renderPassBeginInfo.renderArea.offset.x = 0;
		renderPassBeginInfo.renderArea.offset.y = 0;
		renderPassBeginInfo.renderArea.extent.width = width;
		renderPassBeginInfo.renderArea.extent.height = height;
		renderPassBeginInfo.clearValueCount = 5;
		renderPassBeginInfo.pClearValues = clearValues;
		renderPassBeginInfo.framebuffer = frameBuffers[currentImageIndex];

		VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));

		vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
		vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

		VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);
		vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

		// First sub pass
		// Renders the components of the scene to the G-Buffer attachments
		{
			vks::debugutils::cmdBeginLabel(cmdBuffer, "Subpass 0: Deferred G-Buffer creation", { 1.0f, 0.78f, 0.05f, 1.0f });

			vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.offscreen);
			vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.offscreen, 0, 1, &descriptorSets[currentBuffer].scene, 0, nullptr);
			models.scene.draw(cmdBuffer);

			vks::debugutils::cmdEndLabel(cmdBuffer);
		}

		// Second sub pass
		// This subpass will use the G-Buffer components that have been filled in the first subpass as input attachment for the final compositing
		{
			vks::debugutils::cmdBeginLabel(cmdBuffer, "Subpass 1: Deferred composition", { 0.0f, 0.5f, 1.0f, 1.0f });

			vkCmdNextSubpass(cmdBuffer, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.composition);
			vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.composition, 0, 1, &descriptorSets[currentBuffer].composition, 0, nullptr);
			vkCmdDraw(cmdBuffer, 3, 1, 0, 0);

			vks::debugutils::cmdEndLabel(cmdBuffer);
		}

		// Third subpass
		// Render transparent geometry using a forward pass that compares against depth generated during G-Buffer fill
		{
			vks::debugutils::cmdBeginLabel(cmdBuffer, "Subpass 2: Forward transparency", { 0.5f, 0.76f, 0.34f, 1.0f });

			vkCmdNextSubpass(cmdBuffer, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.transparent);
			vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.transparent, 0, 1, &descriptorSets[currentBuffer].transparent, 0, nullptr);
			models.transparent.draw(cmdBuffer);

			vks::debugutils::cmdEndLabel(cmdBuffer);
		}

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
		if (overlay->header("Subpasses")) {
			overlay->text("0: Deferred G-Buffer creation");
			overlay->text("1: Deferred composition");
			overlay->text("2: Forward transparency");
		}
		if (overlay->header("Settings")) {
			if (overlay->button("Randomize lights")) {
				initLights();
			}
		}
	}
};

VULKAN_EXAMPLE_MAIN()
