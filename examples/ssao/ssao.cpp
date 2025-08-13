/*
* Vulkan Example - Screen space ambient occlusion example
*
* Copyright (C) 2016-2025 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "vulkanexamplebase.h"
#include "VulkanglTFModel.h"

#define SSAO_KERNEL_SIZE 64
#define SSAO_RADIUS 0.3f

// We use a smaller noise kernel size on Android due to lower computational power
#if defined(__ANDROID__)
#define SSAO_NOISE_DIM 4
#else
#define SSAO_NOISE_DIM 8
#endif

class VulkanExample : public VulkanExampleBase
{
public:
	vks::Texture2D ssaoNoise;
	vkglTF::Model scene;

	struct UBOSceneParams {
		glm::mat4 projection;
		glm::mat4 model;
		glm::mat4 view;
		float nearPlane = 0.1f;
		float farPlane = 64.0f;
	} uboSceneParams;

	struct UBOSSAOParams {
		glm::mat4 projection;
		int32_t ssao = true;
		int32_t ssaoOnly = false;
		int32_t ssaoBlur = true;
	} uboSSAOParams;

	struct {
		VkPipelineLayout gBuffer{ VK_NULL_HANDLE };
		VkPipelineLayout ssao{ VK_NULL_HANDLE };
		VkPipelineLayout ssaoBlur{ VK_NULL_HANDLE };
		VkPipelineLayout composition{ VK_NULL_HANDLE };
	} pipelineLayouts;

	struct {
		VkPipeline offscreen{ VK_NULL_HANDLE };
		VkPipeline composition{ VK_NULL_HANDLE };
		VkPipeline ssao{ VK_NULL_HANDLE };
		VkPipeline ssaoBlur{ VK_NULL_HANDLE };
	} pipelines;

	struct {
		VkDescriptorSetLayout gBuffer{ VK_NULL_HANDLE };
		VkDescriptorSetLayout ssao{ VK_NULL_HANDLE };
		VkDescriptorSetLayout ssaoBlur{ VK_NULL_HANDLE };
		VkDescriptorSetLayout composition{ VK_NULL_HANDLE };
	} descriptorSetLayouts;

	struct DescriptorSets {
		VkDescriptorSet gBuffer{ VK_NULL_HANDLE };
		VkDescriptorSet ssao{ VK_NULL_HANDLE };
		VkDescriptorSet ssaoBlur{ VK_NULL_HANDLE };
		VkDescriptorSet composition{ VK_NULL_HANDLE };
	};
	std::array<DescriptorSets, maxConcurrentFrames> descriptorSets;

	struct UniformBuffers {
		vks::Buffer sceneParams;
		vks::Buffer ssaoKernel;
		vks::Buffer ssaoParams;
	};
	std::array<UniformBuffers, maxConcurrentFrames> uniformBuffers;

	// Framebuffer for offscreen rendering
	struct FrameBufferAttachment {
		VkImage image;
		VkDeviceMemory mem;
		VkImageView view;
		VkFormat format;
		void destroy(VkDevice device)
		{
			vkDestroyImage(device, image, nullptr);
			vkDestroyImageView(device, view, nullptr);
			vkFreeMemory(device, mem, nullptr);
		}
	};
	struct FrameBuffer {
		int32_t width, height;
		VkFramebuffer frameBuffer;
		VkRenderPass renderPass;
		void setSize(int32_t w, int32_t h)
		{
			this->width = w;
			this->height = h;
		}
		void destroy(VkDevice device)
		{
			vkDestroyFramebuffer(device, frameBuffer, nullptr);
			vkDestroyRenderPass(device, renderPass, nullptr);
		}
	};

	struct {
		struct Offscreen : public FrameBuffer {
			FrameBufferAttachment position, normal, albedo, depth;
		} offscreen;
		struct SSAO : public FrameBuffer {
			FrameBufferAttachment color;
		} ssao, ssaoBlur;
	} frameBuffers{};

	// One sampler for the frame buffer color attachments
	VkSampler colorSampler;

	VulkanExample() : VulkanExampleBase()
	{
		title = "Screen space ambient occlusion";
		camera.type = Camera::CameraType::firstperson;
#ifndef __ANDROID__
		camera.rotationSpeed = 0.25f;
#endif
		camera.position = { 1.0f, 0.75f, 0.0f };
		camera.setRotation(glm::vec3(0.0f, 90.0f, 0.0f));
		camera.setPerspective(60.0f, (float)width / (float)height, uboSceneParams.nearPlane, uboSceneParams.farPlane);
	}

	~VulkanExample()
	{
		if (device) {
			vkDestroySampler(device, colorSampler, nullptr);
			frameBuffers.offscreen.position.destroy(device);
			frameBuffers.offscreen.normal.destroy(device);
			frameBuffers.offscreen.albedo.destroy(device);
			frameBuffers.offscreen.depth.destroy(device);
			frameBuffers.ssao.color.destroy(device);
			frameBuffers.ssaoBlur.color.destroy(device);
			frameBuffers.offscreen.destroy(device);
			frameBuffers.ssao.destroy(device);
			frameBuffers.ssaoBlur.destroy(device);
			vkDestroyPipeline(device, pipelines.offscreen, nullptr);
			vkDestroyPipeline(device, pipelines.composition, nullptr);
			vkDestroyPipeline(device, pipelines.ssao, nullptr);
			vkDestroyPipeline(device, pipelines.ssaoBlur, nullptr);
			vkDestroyPipelineLayout(device, pipelineLayouts.gBuffer, nullptr);
			vkDestroyPipelineLayout(device, pipelineLayouts.ssao, nullptr);
			vkDestroyPipelineLayout(device, pipelineLayouts.ssaoBlur, nullptr);
			vkDestroyPipelineLayout(device, pipelineLayouts.composition, nullptr);
			vkDestroyDescriptorSetLayout(device, descriptorSetLayouts.gBuffer, nullptr);
			vkDestroyDescriptorSetLayout(device, descriptorSetLayouts.ssao, nullptr);
			vkDestroyDescriptorSetLayout(device, descriptorSetLayouts.ssaoBlur, nullptr);
			vkDestroyDescriptorSetLayout(device, descriptorSetLayouts.composition, nullptr);
			for (auto& buffer : uniformBuffers) {
				buffer.sceneParams.destroy();
				buffer.ssaoKernel.destroy();
				buffer.ssaoParams.destroy();
			}
			ssaoNoise.destroy();
		}
	}

	void getEnabledFeatures()
	{
		enabledFeatures.samplerAnisotropy = deviceFeatures.samplerAnisotropy;
	}

	// Create a frame buffer attachment
	void createAttachment(
		VkFormat format,
		VkImageUsageFlagBits usage,
		FrameBufferAttachment *attachment,
		uint32_t width,
		uint32_t height)
	{
		VkImageAspectFlags aspectMask = 0;

		attachment->format = format;

		if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
		{
			aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}
		if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
		{
			aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			if (format >= VK_FORMAT_D16_UNORM_S8_UINT)
				aspectMask |=VK_IMAGE_ASPECT_STENCIL_BIT;
		}

		assert(aspectMask > 0);

		VkImageCreateInfo image = vks::initializers::imageCreateInfo();
		image.imageType = VK_IMAGE_TYPE_2D;
		image.format = format;
		image.extent.width = width;
		image.extent.height = height;
		image.extent.depth = 1;
		image.mipLevels = 1;
		image.arrayLayers = 1;
		image.samples = VK_SAMPLE_COUNT_1_BIT;
		image.tiling = VK_IMAGE_TILING_OPTIMAL;
		image.usage = usage | VK_IMAGE_USAGE_SAMPLED_BIT;

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

	void prepareOffscreenFramebuffers()
	{
		// Attachments
#if defined(__ANDROID__)
		const uint32_t ssaoWidth = width / 2;
		const uint32_t ssaoHeight = height / 2;
#else
		const uint32_t ssaoWidth = width;
		const uint32_t ssaoHeight = height;
#endif

		frameBuffers.offscreen.setSize(width, height);
		frameBuffers.ssao.setSize(ssaoWidth, ssaoHeight);
		frameBuffers.ssaoBlur.setSize(width, height);

		// Find a suitable depth format
		VkFormat attDepthFormat;
		VkBool32 validDepthFormat = vks::tools::getSupportedDepthFormat(physicalDevice, &attDepthFormat);
		assert(validDepthFormat);

		// G-Buffer
		createAttachment(VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &frameBuffers.offscreen.position, width, height);	// Position + Depth
		createAttachment(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &frameBuffers.offscreen.normal, width, height);			// Normals
		createAttachment(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &frameBuffers.offscreen.albedo, width, height);			// Albedo (color)
		createAttachment(attDepthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, &frameBuffers.offscreen.depth, width, height);			// Depth

		// SSAO
		createAttachment(VK_FORMAT_R8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &frameBuffers.ssao.color, ssaoWidth, ssaoHeight);				// Color

		// SSAO blur
		createAttachment(VK_FORMAT_R8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &frameBuffers.ssaoBlur.color, width, height);					// Color

		// Render passes

		// G-Buffer creation
		{
			std::array<VkAttachmentDescription, 4> attachmentDescs = {};

			// Init attachment properties
			for (uint32_t i = 0; i < static_cast<uint32_t>(attachmentDescs.size()); i++)
			{
				attachmentDescs[i].samples = VK_SAMPLE_COUNT_1_BIT;
				attachmentDescs[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
				attachmentDescs[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
				attachmentDescs[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				attachmentDescs[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				attachmentDescs[i].finalLayout = (i == 3) ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			}

			// Formats
			attachmentDescs[0].format = frameBuffers.offscreen.position.format;
			attachmentDescs[1].format = frameBuffers.offscreen.normal.format;
			attachmentDescs[2].format = frameBuffers.offscreen.albedo.format;
			attachmentDescs[3].format = frameBuffers.offscreen.depth.format;

			std::vector<VkAttachmentReference> colorReferences;
			colorReferences.push_back({ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
			colorReferences.push_back({ 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
			colorReferences.push_back({ 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });

			VkAttachmentReference depthReference = {};
			depthReference.attachment = 3;
			depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			VkSubpassDescription subpass = {};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.pColorAttachments = colorReferences.data();
			subpass.colorAttachmentCount = static_cast<uint32_t>(colorReferences.size());
			subpass.pDepthStencilAttachment = &depthReference;

			// Use subpass dependencies for attachment layout transitions
			std::array<VkSubpassDependency, 3> dependencies{};

			dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[0].dstSubpass = 0;
			dependencies[0].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			dependencies[0].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
			dependencies[0].dependencyFlags = 0;

			dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[1].dstSubpass = 0;
			dependencies[1].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependencies[1].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			dependencies[2].srcSubpass = 0;
			dependencies[2].dstSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependencies[2].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			dependencies[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependencies[2].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			dependencies[2].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			VkRenderPassCreateInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassInfo.pAttachments = attachmentDescs.data();
			renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescs.size());
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpass;
			renderPassInfo.dependencyCount = 2;
			renderPassInfo.pDependencies = dependencies.data();
			VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &frameBuffers.offscreen.renderPass));

			std::array<VkImageView, 4> attachments{};
			attachments[0] = frameBuffers.offscreen.position.view;
			attachments[1] = frameBuffers.offscreen.normal.view;
			attachments[2] = frameBuffers.offscreen.albedo.view;
			attachments[3] = frameBuffers.offscreen.depth.view;

			VkFramebufferCreateInfo fbufCreateInfo = vks::initializers::framebufferCreateInfo();
			fbufCreateInfo.renderPass = frameBuffers.offscreen.renderPass;
			fbufCreateInfo.pAttachments = attachments.data();
			fbufCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			fbufCreateInfo.width = frameBuffers.offscreen.width;
			fbufCreateInfo.height = frameBuffers.offscreen.height;
			fbufCreateInfo.layers = 1;
			VK_CHECK_RESULT(vkCreateFramebuffer(device, &fbufCreateInfo, nullptr, &frameBuffers.offscreen.frameBuffer));
		}

		// SSAO
		{
			VkAttachmentDescription attachmentDescription{};
			attachmentDescription.format = frameBuffers.ssao.color.format;
			attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
			attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

			VkSubpassDescription subpass = {};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.pColorAttachments = &colorReference;
			subpass.colorAttachmentCount = 1;

			std::array<VkSubpassDependency, 2> dependencies{};

			dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[0].dstSubpass = 0;
			dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			dependencies[1].srcSubpass = 0;
			dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			VkRenderPassCreateInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassInfo.pAttachments = &attachmentDescription;
			renderPassInfo.attachmentCount = 1;
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpass;
			renderPassInfo.dependencyCount = 2;
			renderPassInfo.pDependencies = dependencies.data();
			VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &frameBuffers.ssao.renderPass));

			VkFramebufferCreateInfo fbufCreateInfo = vks::initializers::framebufferCreateInfo();
			fbufCreateInfo.renderPass = frameBuffers.ssao.renderPass;
			fbufCreateInfo.pAttachments = &frameBuffers.ssao.color.view;
			fbufCreateInfo.attachmentCount = 1;
			fbufCreateInfo.width = frameBuffers.ssao.width;
			fbufCreateInfo.height = frameBuffers.ssao.height;
			fbufCreateInfo.layers = 1;
			VK_CHECK_RESULT(vkCreateFramebuffer(device, &fbufCreateInfo, nullptr, &frameBuffers.ssao.frameBuffer));
		}

		// SSAO Blur
		{
			VkAttachmentDescription attachmentDescription{};
			attachmentDescription.format = frameBuffers.ssaoBlur.color.format;
			attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
			attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

			VkSubpassDescription subpass = {};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.pColorAttachments = &colorReference;
			subpass.colorAttachmentCount = 1;

			std::array<VkSubpassDependency, 2> dependencies{};

			dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[0].dstSubpass = 0;
			dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			dependencies[1].srcSubpass = 0;
			dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			VkRenderPassCreateInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassInfo.pAttachments = &attachmentDescription;
			renderPassInfo.attachmentCount = 1;
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpass;
			renderPassInfo.dependencyCount = 2;
			renderPassInfo.pDependencies = dependencies.data();
			VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &frameBuffers.ssaoBlur.renderPass));

			VkFramebufferCreateInfo fbufCreateInfo = vks::initializers::framebufferCreateInfo();
			fbufCreateInfo.renderPass = frameBuffers.ssaoBlur.renderPass;
			fbufCreateInfo.pAttachments = &frameBuffers.ssaoBlur.color.view;
			fbufCreateInfo.attachmentCount = 1;
			fbufCreateInfo.width = frameBuffers.ssaoBlur.width;
			fbufCreateInfo.height = frameBuffers.ssaoBlur.height;
			fbufCreateInfo.layers = 1;
			VK_CHECK_RESULT(vkCreateFramebuffer(device, &fbufCreateInfo, nullptr, &frameBuffers.ssaoBlur.frameBuffer));
		}

		// Shared sampler used for all color attachments
		VkSamplerCreateInfo sampler = vks::initializers::samplerCreateInfo();
		sampler.magFilter = VK_FILTER_NEAREST;
		sampler.minFilter = VK_FILTER_NEAREST;
		sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler.addressModeV = sampler.addressModeU;
		sampler.addressModeW = sampler.addressModeU;
		sampler.mipLodBias = 0.0f;
		sampler.maxAnisotropy = 1.0f;
		sampler.minLod = 0.0f;
		sampler.maxLod = 1.0f;
		sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		VK_CHECK_RESULT(vkCreateSampler(device, &sampler, nullptr, &colorSampler));
	}

	void loadAssets()
	{
		vkglTF::descriptorBindingFlags  = vkglTF::DescriptorBindingFlags::ImageBaseColor;
		const uint32_t gltfLoadingFlags = vkglTF::FileLoadingFlags::FlipY | vkglTF::FileLoadingFlags::PreTransformVertices;
		scene.loadFromFile(getAssetPath() + "models/sponza/sponza.gltf", vulkanDevice, queue, gltfLoadingFlags);
	}

	void setupDescriptors()
	{
		// Pool
		std::vector<VkDescriptorPoolSize> poolSizes = {
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, maxConcurrentFrames * 4),
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, maxConcurrentFrames * 10)
		};
		VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, maxConcurrentFrames * 4);
		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));

		VkDescriptorSetAllocateInfo descriptorAllocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, nullptr, 1);
		std::vector<VkWriteDescriptorSet> writeDescriptorSets;

		// Layouts

		// G-Buffer creation (offscreen scene rendering)
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0),	// VS + FS Parameter UBO
		};
		VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings.data(), static_cast<uint32_t>(setLayoutBindings.size()));
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &setLayoutCreateInfo, nullptr, &descriptorSetLayouts.gBuffer));
		
		// SSAO Generation
		setLayoutBindings = {
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2),
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 3),
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 4),
		};
		setLayoutCreateInfo = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings.data(), static_cast<uint32_t>(setLayoutBindings.size()));
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &setLayoutCreateInfo, nullptr, &descriptorSetLayouts.ssao));

		// SSAO Blur
		setLayoutBindings = {
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
		};
		setLayoutCreateInfo = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings.data(), static_cast<uint32_t>(setLayoutBindings.size()));
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &setLayoutCreateInfo, nullptr, &descriptorSetLayouts.ssaoBlur));

		// Composition
		setLayoutBindings = {
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2),
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3),
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 4),
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 5),
		};
		setLayoutCreateInfo = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings.data(), static_cast<uint32_t>(setLayoutBindings.size()));
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &setLayoutCreateInfo, nullptr, &descriptorSetLayouts.composition));

		// Descriptor info for all images used as descriptors
		VkDescriptorImageInfo positionImgDescriptor = vks::initializers::descriptorImageInfo(colorSampler, frameBuffers.offscreen.position.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		VkDescriptorImageInfo normalImgDescriptor = vks::initializers::descriptorImageInfo(colorSampler, frameBuffers.offscreen.normal.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		VkDescriptorImageInfo albedoImgDescriptor = vks::initializers::descriptorImageInfo(colorSampler, frameBuffers.offscreen.albedo.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		VkDescriptorImageInfo ssaoImgDescriptor = vks::initializers::descriptorImageInfo(colorSampler, frameBuffers.ssao.color.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		VkDescriptorImageInfo ssaoBlurImgDescriptor = vks::initializers::descriptorImageInfo(colorSampler, frameBuffers.ssaoBlur.color.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		// Sets per frame, just like the buffers themselves
		// Images do not need to be duplicated per frame, we reuse the same one for each frame
		for (auto i = 0; i < uniformBuffers.size(); i++) {
			// G-Buffer creation (offscreen scene rendering)
			descriptorAllocInfo.pSetLayouts = &descriptorSetLayouts.gBuffer;
			VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &descriptorAllocInfo, &descriptorSets[i].gBuffer));
			writeDescriptorSets = {
				vks::initializers::writeDescriptorSet(descriptorSets[i].gBuffer, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffers[i].sceneParams.descriptor),
			};
			vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);

			// SSAO Generation
			descriptorAllocInfo.pSetLayouts = &descriptorSetLayouts.ssao;
			VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &descriptorAllocInfo, &descriptorSets[i].ssao));
			writeDescriptorSets = {
				vks::initializers::writeDescriptorSet(descriptorSets[i].ssao, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &positionImgDescriptor),
				vks::initializers::writeDescriptorSet(descriptorSets[i].ssao, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &normalImgDescriptor),
				vks::initializers::writeDescriptorSet(descriptorSets[i].ssao, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, &ssaoNoise.descriptor),
				vks::initializers::writeDescriptorSet(descriptorSets[i].ssao, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3, &uniformBuffers[i].ssaoKernel.descriptor),
				vks::initializers::writeDescriptorSet(descriptorSets[i].ssao, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4, &uniformBuffers[i].ssaoParams.descriptor),
			};
			vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);

			// SSAO Blur
			descriptorAllocInfo.pSetLayouts = &descriptorSetLayouts.ssaoBlur;
			VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &descriptorAllocInfo, &descriptorSets[i].ssaoBlur));
			writeDescriptorSets = {
				vks::initializers::writeDescriptorSet(descriptorSets[i].ssaoBlur, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &ssaoImgDescriptor),
			};
			vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);

			// Composition
			descriptorAllocInfo.pSetLayouts = &descriptorSetLayouts.composition;
			VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &descriptorAllocInfo, &descriptorSets[i].composition));
			writeDescriptorSets = {
				vks::initializers::writeDescriptorSet(descriptorSets[i].composition, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &positionImgDescriptor),
				vks::initializers::writeDescriptorSet(descriptorSets[i].composition, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &normalImgDescriptor),
				vks::initializers::writeDescriptorSet(descriptorSets[i].composition, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, &albedoImgDescriptor),
				vks::initializers::writeDescriptorSet(descriptorSets[i].composition, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, &ssaoImgDescriptor),
				vks::initializers::writeDescriptorSet(descriptorSets[i].composition, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4, &ssaoBlurImgDescriptor),
				vks::initializers::writeDescriptorSet(descriptorSets[i].composition, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 5, &uniformBuffers[i].ssaoParams.descriptor),
			};
			vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
		}
	}

	void preparePipelines()
	{
		// Layouts
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo();

		const std::vector<VkDescriptorSetLayout> setLayouts = { descriptorSetLayouts.gBuffer, vkglTF::descriptorSetLayoutImage };
		pipelineLayoutCreateInfo.pSetLayouts = setLayouts.data();
		pipelineLayoutCreateInfo.setLayoutCount = 2;
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayouts.gBuffer));

		pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayouts.ssao;
		pipelineLayoutCreateInfo.setLayoutCount = 1;
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayouts.ssao));

		pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayouts.ssaoBlur;
		pipelineLayoutCreateInfo.setLayoutCount = 1;
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayouts.ssaoBlur));

		pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayouts.composition;
		pipelineLayoutCreateInfo.setLayoutCount = 1;
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

		VkGraphicsPipelineCreateInfo pipelineCreateInfo = vks::initializers::pipelineCreateInfo( pipelineLayouts.composition, renderPass, 0);
		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
		pipelineCreateInfo.pRasterizationState = &rasterizationState;
		pipelineCreateInfo.pColorBlendState = &colorBlendState;
		pipelineCreateInfo.pMultisampleState = &multisampleState;
		pipelineCreateInfo.pViewportState = &viewportState;
		pipelineCreateInfo.pDepthStencilState = &depthStencilState;
		pipelineCreateInfo.pDynamicState = &dynamicState;
		pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineCreateInfo.pStages = shaderStages.data();

		// Empty vertex input state for fullscreen passes
		VkPipelineVertexInputStateCreateInfo emptyVertexInputState = vks::initializers::pipelineVertexInputStateCreateInfo();
		pipelineCreateInfo.pVertexInputState = &emptyVertexInputState;
		rasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;

		// Final composition pipeline
		shaderStages[0] = loadShader(getShadersPath() + "ssao/fullscreen.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getShadersPath() + "ssao/composition.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.composition));

		// SSAO generation pipeline
		pipelineCreateInfo.renderPass = frameBuffers.ssao.renderPass;
		pipelineCreateInfo.layout = pipelineLayouts.ssao;
		// SSAO Kernel size and radius are constant for this pipeline, so we set them using specialization constants
		struct SpecializationData {
			uint32_t kernelSize = SSAO_KERNEL_SIZE;
			float radius = SSAO_RADIUS;
		} specializationData;
		std::array<VkSpecializationMapEntry, 2> specializationMapEntries = {
			vks::initializers::specializationMapEntry(0, offsetof(SpecializationData, kernelSize), sizeof(SpecializationData::kernelSize)),
			vks::initializers::specializationMapEntry(1, offsetof(SpecializationData, radius), sizeof(SpecializationData::radius))
		};
		VkSpecializationInfo specializationInfo = vks::initializers::specializationInfo(2, specializationMapEntries.data(), sizeof(specializationData), &specializationData);
		shaderStages[1] = loadShader(getShadersPath() + "ssao/ssao.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		shaderStages[1].pSpecializationInfo = &specializationInfo;
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.ssao));

		// SSAO blur pipeline
		pipelineCreateInfo.renderPass = frameBuffers.ssaoBlur.renderPass;
		pipelineCreateInfo.layout = pipelineLayouts.ssaoBlur;
		shaderStages[1] = loadShader(getShadersPath() + "ssao/blur.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.ssaoBlur));

		// Fill G-Buffer pipeline
		// Vertex input state from glTF model loader
		pipelineCreateInfo.pVertexInputState = vkglTF::Vertex::getPipelineVertexInputState({ vkglTF::VertexComponent::Position, vkglTF::VertexComponent::UV, vkglTF::VertexComponent::Color, vkglTF::VertexComponent::Normal });
		pipelineCreateInfo.renderPass = frameBuffers.offscreen.renderPass;
		pipelineCreateInfo.layout = pipelineLayouts.gBuffer;
		// Blend attachment states required for all color attachments
		// This is important, as color write mask will otherwise be 0x0 and you
		// won't see anything rendered to the attachment
		std::array<VkPipelineColorBlendAttachmentState, 3> blendAttachmentStates = {
			vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
			vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
			vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE)
		};
		colorBlendState.attachmentCount = static_cast<uint32_t>(blendAttachmentStates.size());
		colorBlendState.pAttachments = blendAttachmentStates.data();
		rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
		shaderStages[0] = loadShader(getShadersPath() + "ssao/gbuffer.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getShadersPath() + "ssao/gbuffer.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.offscreen));
	}

	float lerp(float a, float b, float f)
	{
		return a + f * (b - a);
	}

	// Prepare and initialize uniform buffer containing shader uniforms
	void prepareBuffers()
	{
		// Set up SSAO sample kernel
		std::default_random_engine rndEngine(benchmark.active ? 0 : (unsigned)time(nullptr));
		std::uniform_real_distribution<float> rndDist(0.0f, 1.0f);
		std::vector<glm::vec4> ssaoKernel(SSAO_KERNEL_SIZE);
		for (uint32_t i = 0; i < SSAO_KERNEL_SIZE; ++i) {
			glm::vec3 sample(rndDist(rndEngine) * 2.0 - 1.0, rndDist(rndEngine) * 2.0 - 1.0, rndDist(rndEngine));
			sample = glm::normalize(sample);
			sample *= rndDist(rndEngine);
			float scale = float(i) / float(SSAO_KERNEL_SIZE);
			scale = lerp(0.1f, 1.0f, scale * scale);
			ssaoKernel[i] = glm::vec4(sample * scale, 0.0f);
		}

		for (auto& buffer : uniformBuffers) {
			// Scene matrices
			vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &buffer.sceneParams, sizeof(uboSceneParams));
			VK_CHECK_RESULT(buffer.sceneParams.map());
			// SSAO parameters
			vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &buffer.ssaoParams, sizeof(uboSSAOParams));
			VK_CHECK_RESULT(buffer.ssaoParams.map());
			// SSAO kernel
			vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &buffer.ssaoKernel, ssaoKernel.size() * sizeof(glm::vec4), ssaoKernel.data());
		}

		// Random noise for smoothing SSAO is uploaded as a texture
		std::vector<glm::vec4> noiseValues(SSAO_NOISE_DIM * SSAO_NOISE_DIM);
		for (uint32_t i = 0; i < static_cast<uint32_t>(noiseValues.size()); i++) {
			noiseValues[i] = glm::vec4(rndDist(rndEngine) * 2.0f - 1.0f, rndDist(rndEngine) * 2.0f - 1.0f, 0.0f, 0.0f);
		}
		ssaoNoise.fromBuffer(noiseValues.data(), noiseValues.size() * sizeof(glm::vec4), VK_FORMAT_R32G32B32A32_SFLOAT, SSAO_NOISE_DIM, SSAO_NOISE_DIM, vulkanDevice, queue, VK_FILTER_NEAREST);
	}

	void updateUniformBuffers()
	{
		// Scene
		uboSceneParams.projection = camera.matrices.perspective;
		uboSceneParams.view = camera.matrices.view;
		uboSceneParams.model = glm::mat4(1.0f);
		uniformBuffers[currentBuffer].sceneParams.copyTo(&uboSceneParams, sizeof(uboSceneParams));

		// SSAO parameters
		uboSSAOParams.projection = camera.matrices.perspective;
		uniformBuffers[currentBuffer].ssaoParams.copyTo(&uboSSAOParams, sizeof(uboSSAOParams));
	}

	void prepare()
	{
		VulkanExampleBase::prepare();
		loadAssets();
		prepareOffscreenFramebuffers();
		prepareBuffers();
		setupDescriptors();
		preparePipelines();
		prepared = true;
	}

	void buildCommandBuffer()
	{
		VkCommandBuffer cmdBuffer = drawCmdBuffers[currentBuffer];
		
		VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

		VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));

		/*
			Offscreen SSAO generation
		*/
		{
			// Clear values for all attachments written in the fragment shader
			std::array<VkClearValue, 4> clearValues{};
			clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
			clearValues[1].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
			clearValues[2].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
			clearValues[3].depthStencil = { 1.0f, 0 };

			VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
			renderPassBeginInfo.renderPass = frameBuffers.offscreen.renderPass;
			renderPassBeginInfo.framebuffer = frameBuffers.offscreen.frameBuffer;
			renderPassBeginInfo.renderArea.extent.width = frameBuffers.offscreen.width;
			renderPassBeginInfo.renderArea.extent.height = frameBuffers.offscreen.height;
			renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
			renderPassBeginInfo.pClearValues = clearValues.data();

			/*
				First pass: Fill G-Buffer components (positions+depth, normals, albedo) using MRT
			*/

			vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport viewport = vks::initializers::viewport((float)frameBuffers.offscreen.width, (float)frameBuffers.offscreen.height, 0.0f, 1.0f);
			vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

			VkRect2D scissor = vks::initializers::rect2D(frameBuffers.offscreen.width, frameBuffers.offscreen.height, 0, 0);
			vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

			vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.offscreen);

			vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.gBuffer, 0, 1, &descriptorSets[currentBuffer].gBuffer, 0, nullptr);
			scene.draw(cmdBuffer, vkglTF::RenderFlags::BindImages, pipelineLayouts.gBuffer);

			vkCmdEndRenderPass(cmdBuffer);

			/*
				Second pass: SSAO generation
			*/

			clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
			clearValues[1].depthStencil = { 1.0f, 0 };

			renderPassBeginInfo.framebuffer = frameBuffers.ssao.frameBuffer;
			renderPassBeginInfo.renderPass = frameBuffers.ssao.renderPass;
			renderPassBeginInfo.renderArea.extent.width = frameBuffers.ssao.width;
			renderPassBeginInfo.renderArea.extent.height = frameBuffers.ssao.height;
			renderPassBeginInfo.clearValueCount = 2;
			renderPassBeginInfo.pClearValues = clearValues.data();

			vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			viewport = vks::initializers::viewport((float)frameBuffers.ssao.width, (float)frameBuffers.ssao.height, 0.0f, 1.0f);
			vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
			scissor = vks::initializers::rect2D(frameBuffers.ssao.width, frameBuffers.ssao.height, 0, 0);
			vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

			vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.ssao, 0, 1, &descriptorSets[currentBuffer].ssao, 0, nullptr);
			vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.ssao);
			vkCmdDraw(cmdBuffer, 3, 1, 0, 0);

			vkCmdEndRenderPass(cmdBuffer);

			/*
				Third pass: SSAO blur
			*/

			renderPassBeginInfo.framebuffer = frameBuffers.ssaoBlur.frameBuffer;
			renderPassBeginInfo.renderPass = frameBuffers.ssaoBlur.renderPass;
			renderPassBeginInfo.renderArea.extent.width = frameBuffers.ssaoBlur.width;
			renderPassBeginInfo.renderArea.extent.height = frameBuffers.ssaoBlur.height;

			vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			viewport = vks::initializers::viewport((float)frameBuffers.ssaoBlur.width, (float)frameBuffers.ssaoBlur.height, 0.0f, 1.0f);
			vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
			scissor = vks::initializers::rect2D(frameBuffers.ssaoBlur.width, frameBuffers.ssaoBlur.height, 0, 0);
			vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

			vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.ssaoBlur, 0, 1, &descriptorSets[currentBuffer].ssaoBlur, 0, nullptr);
			vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.ssaoBlur);
			vkCmdDraw(cmdBuffer, 3, 1, 0, 0);

			vkCmdEndRenderPass(cmdBuffer);
		}

		/*
			Note: Explicit synchronization is not required between the render pass, as this is done implicit via sub pass dependencies
		*/

		/*
			Final render pass: Scene rendering with applied radial blur
		*/
		{
			std::array<VkClearValue, 2> clearValues{};
			clearValues[0].color = defaultClearColor;
			clearValues[1].depthStencil = { 1.0f, 0 };

			VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
			renderPassBeginInfo.renderPass = renderPass;
			renderPassBeginInfo.framebuffer = VulkanExampleBase::frameBuffers[currentImageIndex];
			renderPassBeginInfo.renderArea.extent.width = width;
			renderPassBeginInfo.renderArea.extent.height = height;
			renderPassBeginInfo.clearValueCount = 2;
			renderPassBeginInfo.pClearValues = clearValues.data();

			vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
			vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

			VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);
			vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

			vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.composition, 0, 1, &descriptorSets[currentBuffer].composition, 0, nullptr);

			// Final composition pass
			vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.composition);
			vkCmdDraw(cmdBuffer, 3, 1, 0, 0);

			drawUI(cmdBuffer);

			vkCmdEndRenderPass(cmdBuffer);
		}

		VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffer));
	}

	virtual void render()
	{
		if (!prepared) {
			return;
		}
		VulkanExampleBase::prepareFrame();
		updateUniformBuffers();
		buildCommandBuffer();
		VulkanExampleBase::submitFrame();
	}

	virtual void OnUpdateUIOverlay(vks::UIOverlay *overlay)
	{
		if (overlay->header("Settings")) {
			overlay->checkBox("Enable SSAO", &uboSSAOParams.ssao);
			overlay->checkBox("SSAO blur", &uboSSAOParams.ssaoBlur);
			overlay->checkBox("SSAO pass only", &uboSSAOParams.ssaoOnly);
		}
	}
};

VULKAN_EXAMPLE_MAIN()
