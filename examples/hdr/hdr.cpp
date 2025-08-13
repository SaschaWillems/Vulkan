/*
* Vulkan Example - High dynamic range rendering pipeline
*
* This sample implements a HDR rendering pipeline that uses a wider range of possible colors via float component image formats
* It also does a bloom filter on the HDR image
* The final output is standard definition range (SDR)
* Note: Does not make use of HDR display capability. HDR is only internally used for offscreen rendering.
* 
* Copyright (C) 2016-2025 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "vulkanexamplebase.h"
#include "VulkanglTFModel.h"

class VulkanExample : public VulkanExampleBase
{
public:
	bool bloom = true;
	bool displaySkybox = true;

	struct {
		vks::TextureCubeMap envmap;
	} textures;

	struct Models {
		vkglTF::Model skybox;
		std::vector<vkglTF::Model> objects;
		int32_t index{ 1 };
	} models;
	std::vector<std::string> modelNames{};

	struct UniformData {
		glm::mat4 projection;
		glm::mat4 modelview;
		glm::mat4 inverseModelview;
		float exposure{ 1.0f };
	} uniformData;
	std::array<vks::Buffer, maxConcurrentFrames> uniformBuffers;

	struct {
		VkPipelineLayout models{ VK_NULL_HANDLE };
		VkPipelineLayout composition{ VK_NULL_HANDLE };
		VkPipelineLayout bloomFilter{ VK_NULL_HANDLE };
	} pipelineLayouts;

	struct {
		VkPipeline skybox{ VK_NULL_HANDLE };
		VkPipeline reflect{ VK_NULL_HANDLE };
		VkPipeline composition{ VK_NULL_HANDLE };
		// Bloom is a two pass filter (one pass for vertical and horizontal blur)
		VkPipeline bloom[2]{ VK_NULL_HANDLE };
	} pipelines;

	struct {
		VkDescriptorSetLayout models{ VK_NULL_HANDLE };
		VkDescriptorSetLayout composition{ VK_NULL_HANDLE };
		VkDescriptorSetLayout bloomFilter{ VK_NULL_HANDLE };
	} descriptorSetLayouts;

	struct DescriptorSets {
		VkDescriptorSet object{ VK_NULL_HANDLE };
		VkDescriptorSet skybox{ VK_NULL_HANDLE };
		VkDescriptorSet composition{ VK_NULL_HANDLE };
		VkDescriptorSet bloomFilter{ VK_NULL_HANDLE };
	};
	std::array<DescriptorSets, maxConcurrentFrames> descriptorSets{};

	// Framebuffer for offscreen rendering
	struct FrameBufferAttachment {
		VkImage image;
		VkDeviceMemory mem;
		VkImageView view;
		VkFormat format;
		void destroy(VkDevice device)
		{
			vkDestroyImageView(device, view, nullptr);
			vkDestroyImage(device, image, nullptr);
			vkFreeMemory(device, mem, nullptr);
		}
	};
	struct FrameBuffer {
		int32_t width, height;
		VkFramebuffer frameBuffer;
		FrameBufferAttachment color[2];
		FrameBufferAttachment depth;
		VkRenderPass renderPass;
		VkSampler sampler;
	} offscreen;

	struct {
		int32_t width, height;
		VkFramebuffer frameBuffer;
		FrameBufferAttachment color[1];
		VkRenderPass renderPass;
		VkSampler sampler;
	} filterPass;

	VulkanExample() : VulkanExampleBase()
	{
		title = "High dynamic range rendering";
		camera.type = Camera::CameraType::lookat;
		camera.setPosition(glm::vec3(0.0f, 0.0f, -6.0f));
		camera.setRotation(glm::vec3(0.0f, 0.0f, 0.0f));
		camera.setPerspective(60.0f, (float)width / (float)height, 0.1f, 256.0f);
	}

	~VulkanExample()
	{
		if (device) {
			vkDestroyPipeline(device, pipelines.skybox, nullptr);
			vkDestroyPipeline(device, pipelines.reflect, nullptr);
			vkDestroyPipeline(device, pipelines.composition, nullptr);
			vkDestroyPipeline(device, pipelines.bloom[0], nullptr);
			vkDestroyPipeline(device, pipelines.bloom[1], nullptr);
			vkDestroyPipelineLayout(device, pipelineLayouts.models, nullptr);
			vkDestroyPipelineLayout(device, pipelineLayouts.composition, nullptr);
			vkDestroyPipelineLayout(device, pipelineLayouts.bloomFilter, nullptr);
			vkDestroyDescriptorSetLayout(device, descriptorSetLayouts.models, nullptr);
			vkDestroyDescriptorSetLayout(device, descriptorSetLayouts.composition, nullptr);
			vkDestroyDescriptorSetLayout(device, descriptorSetLayouts.bloomFilter, nullptr);
			vkDestroyRenderPass(device, offscreen.renderPass, nullptr);
			vkDestroyRenderPass(device, filterPass.renderPass, nullptr);
			vkDestroyFramebuffer(device, offscreen.frameBuffer, nullptr);
			vkDestroyFramebuffer(device, filterPass.frameBuffer, nullptr);
			vkDestroySampler(device, offscreen.sampler, nullptr);
			vkDestroySampler(device, filterPass.sampler, nullptr);
			offscreen.depth.destroy(device);
			offscreen.color[0].destroy(device);
			offscreen.color[1].destroy(device);
			filterPass.color[0].destroy(device);
			textures.envmap.destroy();
			for (auto& buffer : uniformBuffers) {
				buffer.destroy();
			}
		}
	}

	void createAttachment(VkFormat format, VkImageUsageFlagBits usage, FrameBufferAttachment *attachment)
	{
		VkImageAspectFlags aspectMask = 0;
		VkImageLayout imageLayout;

		attachment->format = format;

		if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
		{
			aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		}
		if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
		{
			aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			if (format >= VK_FORMAT_D16_UNORM_S8_UINT)
				aspectMask |=VK_IMAGE_ASPECT_STENCIL_BIT;
			imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		}

		assert(aspectMask > 0);

		VkImageCreateInfo image = vks::initializers::imageCreateInfo();
		image.imageType = VK_IMAGE_TYPE_2D;
		image.format = format;
		image.extent.width = offscreen.width;
		image.extent.height = offscreen.height;
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

	// Prepare a new framebuffer and attachments for offscreen rendering (G-Buffer)
	void prepareoffscreenfer()
	{
		{
			offscreen.width = width;
			offscreen.height = height;

			// Color attachments

			// Two floating point color buffers
			createAttachment(VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &offscreen.color[0]);
			createAttachment(VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &offscreen.color[1]);
			// Depth attachment
			createAttachment(depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, &offscreen.depth);

			// Set up separate renderpass with references to the color and depth attachments
			std::array<VkAttachmentDescription, 3> attachmentDescs = {};

			// Init attachment properties
			for (uint32_t i = 0; i < 3; ++i)
			{
				attachmentDescs[i].samples = VK_SAMPLE_COUNT_1_BIT;
				attachmentDescs[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
				attachmentDescs[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
				attachmentDescs[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				attachmentDescs[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				if (i == 2)
				{
					attachmentDescs[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
					attachmentDescs[i].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				}
				else
				{
					attachmentDescs[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
					attachmentDescs[i].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				}
			}

			// Formats
			attachmentDescs[0].format = offscreen.color[0].format;
			attachmentDescs[1].format = offscreen.color[1].format;
			attachmentDescs[2].format = offscreen.depth.format;

			std::vector<VkAttachmentReference> colorReferences;
			colorReferences.push_back({ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
			colorReferences.push_back({ 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });

			VkAttachmentReference depthReference = {};
			depthReference.attachment = 2;
			depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			VkSubpassDescription subpass = {};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.pColorAttachments = colorReferences.data();
			subpass.colorAttachmentCount = 2;
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
			dependencies[1].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependencies[1].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			dependencies[2].srcSubpass = 0;
			dependencies[2].dstSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependencies[2].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			dependencies[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependencies[2].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			dependencies[2].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			VkRenderPassCreateInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassInfo.pAttachments = attachmentDescs.data();
			renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescs.size());
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpass;
			renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
			renderPassInfo.pDependencies = dependencies.data();

			VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &offscreen.renderPass));

			std::array<VkImageView, 3> attachments;
			attachments[0] = offscreen.color[0].view;
			attachments[1] = offscreen.color[1].view;
			attachments[2] = offscreen.depth.view;

			VkFramebufferCreateInfo fbufCreateInfo = {};
			fbufCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			fbufCreateInfo.pNext = NULL;
			fbufCreateInfo.renderPass = offscreen.renderPass;
			fbufCreateInfo.pAttachments = attachments.data();
			fbufCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			fbufCreateInfo.width = offscreen.width;
			fbufCreateInfo.height = offscreen.height;
			fbufCreateInfo.layers = 1;
			VK_CHECK_RESULT(vkCreateFramebuffer(device, &fbufCreateInfo, nullptr, &offscreen.frameBuffer));

			// Create sampler to sample from the color attachments
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
			VK_CHECK_RESULT(vkCreateSampler(device, &sampler, nullptr, &offscreen.sampler));
		}

		// Bloom separable filter pass
		{
			filterPass.width = width;
			filterPass.height = height;

			// Color attachments

			// Two floating point color buffers
			createAttachment(VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &filterPass.color[0]);

			// Set up separate renderpass with references to the color and depth attachments
			std::array<VkAttachmentDescription, 1> attachmentDescs = {};

			// Init attachment properties
			attachmentDescs[0].samples = VK_SAMPLE_COUNT_1_BIT;
			attachmentDescs[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachmentDescs[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachmentDescs[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachmentDescs[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachmentDescs[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attachmentDescs[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			attachmentDescs[0].format = filterPass.color[0].format;

			std::vector<VkAttachmentReference> colorReferences;
			colorReferences.push_back({ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });

			VkSubpassDescription subpass = {};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.pColorAttachments = colorReferences.data();
			subpass.colorAttachmentCount = 1;

			// Use subpass dependencies for attachment layout transitions
			std::array<VkSubpassDependency, 2> dependencies;

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
			renderPassInfo.pAttachments = attachmentDescs.data();
			renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescs.size());
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpass;
			renderPassInfo.dependencyCount = 2;
			renderPassInfo.pDependencies = dependencies.data();

			VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &filterPass.renderPass));

			std::array<VkImageView, 1> attachments;
			attachments[0] = filterPass.color[0].view;

			VkFramebufferCreateInfo fbufCreateInfo = {};
			fbufCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			fbufCreateInfo.pNext = NULL;
			fbufCreateInfo.renderPass = filterPass.renderPass;
			fbufCreateInfo.pAttachments = attachments.data();
			fbufCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			fbufCreateInfo.width = filterPass.width;
			fbufCreateInfo.height = filterPass.height;
			fbufCreateInfo.layers = 1;
			VK_CHECK_RESULT(vkCreateFramebuffer(device, &fbufCreateInfo, nullptr, &filterPass.frameBuffer));

			// Create sampler to sample from the color attachments
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
			VK_CHECK_RESULT(vkCreateSampler(device, &sampler, nullptr, &filterPass.sampler));
		}
	}

	void loadAssets()
	{
		// Load glTF models
		const uint32_t glTFLoadingFlags = vkglTF::FileLoadingFlags::PreTransformVertices | vkglTF::FileLoadingFlags::FlipY;
		models.skybox.loadFromFile(getAssetPath() + "models/cube.gltf", vulkanDevice, queue, glTFLoadingFlags);
		std::vector<std::string> filenames = { "sphere.gltf", "teapot.gltf", "torusknot.gltf", "venus.gltf" };
		modelNames = { "Sphere", "Teapot", "Torusknot", "Venus" };
		models.objects.resize(filenames.size());
		for (size_t i = 0; i < filenames.size(); i++) {
			models.objects[i].loadFromFile(getAssetPath() + "models/" + filenames[i], vulkanDevice, queue, glTFLoadingFlags);
		}
		// Load HDR cube map
		textures.envmap.loadFromFile(getAssetPath() + "textures/hdr/uffizi_cube.ktx", VK_FORMAT_R16G16B16A16_SFLOAT, vulkanDevice, queue);
	}

	void setupDescriptors()
	{
		// Pool
		std::vector<VkDescriptorPoolSize> poolSizes = {
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, maxConcurrentFrames * 2),
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, maxConcurrentFrames * 6)
		};
		VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(static_cast<uint32_t>(poolSizes.size()), poolSizes.data(), maxConcurrentFrames * 4);
		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));

		// Layouts
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0),
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
		};
		VkDescriptorSetLayoutCreateInfo descriptorLayoutInfo = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings.data(), static_cast<uint32_t>(setLayoutBindings.size()));
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayoutInfo, nullptr, &descriptorSetLayouts.models));

		// Bloom filter
		setLayoutBindings = {
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
		};

		descriptorLayoutInfo = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings.data(), static_cast<uint32_t>(setLayoutBindings.size()));
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayoutInfo, nullptr, &descriptorSetLayouts.bloomFilter));

		// G-Buffer composition
		setLayoutBindings = {
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
		};

		descriptorLayoutInfo = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings.data(), static_cast<uint32_t>(setLayoutBindings.size()));
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayoutInfo, nullptr, &descriptorSetLayouts.composition));

		// Sets per frame, just like the buffers themselves
		// Images do not need to be duplicated per frame, we reuse the same one for each frame
		for (auto i = 0; i < uniformBuffers.size(); i++) {
			// 3D object descriptor set
			VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayouts.models, 1);
			VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets[i].object));
			std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
				vks::initializers::writeDescriptorSet(descriptorSets[i].object, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffers[i].descriptor),
				vks::initializers::writeDescriptorSet(descriptorSets[i].object, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &textures.envmap.descriptor),
			};
			vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);

			// Sky box descriptor set
			VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets[i].skybox));
			writeDescriptorSets = {
				vks::initializers::writeDescriptorSet(descriptorSets[i].skybox, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0,&uniformBuffers[i].descriptor),
				vks::initializers::writeDescriptorSet(descriptorSets[i].skybox, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &textures.envmap.descriptor),
			};
			vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);

			// Bloom filter
			allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayouts.bloomFilter, 1);
			VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets[i].bloomFilter));
			std::vector<VkDescriptorImageInfo> colorDescriptors = {
				vks::initializers::descriptorImageInfo(offscreen.sampler, offscreen.color[0].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
				vks::initializers::descriptorImageInfo(offscreen.sampler, offscreen.color[1].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
			};
			writeDescriptorSets = {
				vks::initializers::writeDescriptorSet(descriptorSets[i].bloomFilter, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &colorDescriptors[0]),
				vks::initializers::writeDescriptorSet(descriptorSets[i].bloomFilter, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &colorDescriptors[1]),
			};
			vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);

			// Composition descriptor set
			allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayouts.composition, 1);
			VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets[i].composition));
			colorDescriptors = {
				vks::initializers::descriptorImageInfo(offscreen.sampler, offscreen.color[0].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
				vks::initializers::descriptorImageInfo(offscreen.sampler, filterPass.color[0].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
			};
			writeDescriptorSets = {
				vks::initializers::writeDescriptorSet(descriptorSets[i].composition, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &colorDescriptors[0]),
				vks::initializers::writeDescriptorSet(descriptorSets[i].composition, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &colorDescriptors[1]),
			};
			vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
		}
	}

	void preparePipelines()
	{
		// Layouts
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(&descriptorSetLayouts.models, 1);
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayouts.models));

		pipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(&descriptorSetLayouts.bloomFilter, 1);
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayouts.bloomFilter));

		pipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(&descriptorSetLayouts.composition, 1);
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayouts.composition));

		// Pipelines
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
		VkPipelineRasterizationStateCreateInfo rasterizationState = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
		VkPipelineColorBlendAttachmentState blendAttachmentState = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
		VkPipelineColorBlendStateCreateInfo colorBlendState = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
		VkPipelineDepthStencilStateCreateInfo depthStencilState = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_FALSE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL);
		VkPipelineViewportStateCreateInfo viewportState = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
		VkPipelineMultisampleStateCreateInfo multisampleState = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
		std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicState = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};

		VkGraphicsPipelineCreateInfo pipelineCI = vks::initializers::pipelineCreateInfo(pipelineLayouts.models, renderPass, 0);
		pipelineCI.pInputAssemblyState = &inputAssemblyState;
		pipelineCI.pRasterizationState = &rasterizationState;
		pipelineCI.pColorBlendState = &colorBlendState;
		pipelineCI.pMultisampleState = &multisampleState;
		pipelineCI.pViewportState = &viewportState;
		pipelineCI.pDepthStencilState = &depthStencilState;
		pipelineCI.pDynamicState = &dynamicState;
		pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineCI.pStages = shaderStages.data();

		VkSpecializationInfo specializationInfo;
		std::array<VkSpecializationMapEntry, 1> specializationMapEntries{};

		// Full screen pipelines

		// Empty vertex input state, full screen triangles are generated by the vertex shader
		VkPipelineVertexInputStateCreateInfo emptyInputState = vks::initializers::pipelineVertexInputStateCreateInfo();
		pipelineCI.pVertexInputState = &emptyInputState;

		// Final fullscreen composition pass pipeline
		std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates = {
			vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
			vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
		};
		pipelineCI.layout = pipelineLayouts.composition;
		pipelineCI.renderPass = renderPass;
		rasterizationState.cullMode = VK_CULL_MODE_NONE;
		colorBlendState.attachmentCount = 1;
		colorBlendState.pAttachments = blendAttachmentStates.data();
		shaderStages[0] = loadShader(getShadersPath() + "hdr/composition.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getShadersPath() + "hdr/composition.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.composition));

		// Bloom pass
		shaderStages[0] = loadShader(getShadersPath() + "hdr/bloom.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getShadersPath() + "hdr/bloom.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		colorBlendState.pAttachments = &blendAttachmentState;
		blendAttachmentState.colorWriteMask = 0xF;
		blendAttachmentState.blendEnable = VK_TRUE;
		blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
		blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
		blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
		blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;

		// Set constant parameters via specialization constants
		specializationMapEntries[0] = vks::initializers::specializationMapEntry(0, 0, sizeof(uint32_t));
		uint32_t dir = 1;
		specializationInfo = vks::initializers::specializationInfo(1, specializationMapEntries.data(), sizeof(dir), &dir);
		shaderStages[1].pSpecializationInfo = &specializationInfo;
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.bloom[0]));

		// Second blur pass (into separate framebuffer)
		pipelineCI.renderPass = filterPass.renderPass;
		dir = 0;
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.bloom[1]));

		// Object rendering pipelines
		// Use vertex input state from glTF model setup
		pipelineCI.pVertexInputState = vkglTF::Vertex::getPipelineVertexInputState({ vkglTF::VertexComponent::Position, vkglTF::VertexComponent::Normal });

		blendAttachmentState.blendEnable = VK_FALSE;
		pipelineCI.layout = pipelineLayouts.models;
		pipelineCI.renderPass = offscreen.renderPass;
		colorBlendState.attachmentCount = 2;
		colorBlendState.pAttachments = blendAttachmentStates.data();
		shaderStages[0] = loadShader(getShadersPath() + "hdr/gbuffer.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getShadersPath() + "hdr/gbuffer.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		// Set constant parameters via specialization constants
		specializationMapEntries[0] = vks::initializers::specializationMapEntry(0, 0, sizeof(uint32_t));
		uint32_t shadertype = 0;
		specializationInfo = vks::initializers::specializationInfo(1, specializationMapEntries.data(), sizeof(shadertype), &shadertype);
		shaderStages[0].pSpecializationInfo = &specializationInfo;
		shaderStages[1].pSpecializationInfo = &specializationInfo;
		// Skybox pipeline (background cube)
		rasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.skybox));

		// Object rendering pipeline
		shadertype = 1;
		// Enable depth test and write
		depthStencilState.depthWriteEnable = VK_TRUE;
		depthStencilState.depthTestEnable = VK_TRUE;
		// Flip cull mode
		rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.reflect));
	}

	// Prepare and initialize uniform buffer containing shader uniforms
	void prepareUniformBuffers()
	{
		for (auto& buffer : uniformBuffers) {
			// Scene matrices uniform buffer
			VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &buffer, sizeof(UniformData)));
			VK_CHECK_RESULT(buffer.map());
		}
	}

	void updateUniformBuffers()
	{
		uniformData.projection = camera.matrices.perspective;
		uniformData.modelview = camera.matrices.view;
		uniformData.inverseModelview = glm::inverse(camera.matrices.view);
		memcpy(uniformBuffers[currentBuffer].mapped, &uniformData, sizeof(uniformData));
	}

	void prepare()
	{
		VulkanExampleBase::prepare();
		loadAssets();
		prepareUniformBuffers();
		prepareoffscreenfer();
		setupDescriptors();
		preparePipelines();
		prepared = true;
	}

	void buildCommandBuffer()
	{
		VkCommandBuffer cmdBuffer = drawCmdBuffers[currentBuffer];
		
		VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

		VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));

		{
			/*
				First pass: Render scene to offscreen framebuffer
			*/

			std::array<VkClearValue, 3> clearValues{};
			clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
			clearValues[1].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
			clearValues[2].depthStencil = { 1.0f, 0 };

			VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
			renderPassBeginInfo.renderPass = offscreen.renderPass;
			renderPassBeginInfo.framebuffer = offscreen.frameBuffer;
			renderPassBeginInfo.renderArea.extent.width = offscreen.width;
			renderPassBeginInfo.renderArea.extent.height = offscreen.height;
			renderPassBeginInfo.clearValueCount = 3;
			renderPassBeginInfo.pClearValues = clearValues.data();

			vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport viewport = vks::initializers::viewport((float)offscreen.width, (float)offscreen.height, 0.0f, 1.0f);
			vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

			VkRect2D scissor = vks::initializers::rect2D(offscreen.width, offscreen.height, 0, 0);
			vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

			VkDeviceSize offsets[1] = { 0 };

			// Skybox
			if (displaySkybox)
			{
				vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.models, 0, 1, &descriptorSets[currentBuffer].skybox, 0, nullptr);
				vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &models.skybox.vertices.buffer, offsets);
				vkCmdBindIndexBuffer(cmdBuffer, models.skybox.indices.buffer, 0, VK_INDEX_TYPE_UINT32);
				vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.skybox);
				models.skybox.draw(cmdBuffer);
			}

			// 3D object
			vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.models, 0, 1, &descriptorSets[currentBuffer].object, 0, nullptr);
			vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &models.objects[models.index].vertices.buffer, offsets);
			vkCmdBindIndexBuffer(cmdBuffer, models.objects[models.index].indices.buffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.reflect);
			models.objects[models.index].draw(cmdBuffer);

			vkCmdEndRenderPass(cmdBuffer);
		}

		/*
			Second render pass: First bloom pass
		*/
		if (bloom) {
			VkClearValue clearValues[2]{};
			clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
			clearValues[1].depthStencil = { 1.0f, 0 };

			// Bloom filter
			VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
			renderPassBeginInfo.framebuffer = filterPass.frameBuffer;
			renderPassBeginInfo.renderPass = filterPass.renderPass;
			renderPassBeginInfo.clearValueCount = 1;
			renderPassBeginInfo.renderArea.extent.width = filterPass.width;
			renderPassBeginInfo.renderArea.extent.height = filterPass.height;
			renderPassBeginInfo.pClearValues = clearValues;

			vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport viewport = vks::initializers::viewport((float)filterPass.width, (float)filterPass.height, 0.0f, 1.0f);
			vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

			VkRect2D scissor = vks::initializers::rect2D(filterPass.width, filterPass.height, 0, 0);
			vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

			vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.bloomFilter, 0, 1, &descriptorSets[currentBuffer].bloomFilter, 0, nullptr);

			vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.bloom[1]);
			vkCmdDraw(cmdBuffer, 3, 1, 0, 0);

			vkCmdEndRenderPass(cmdBuffer);
		}

		/*
			Note: Explicit synchronization is not required between the render pass, as this is done implicit via sub pass dependencies
		*/

		/*
			Third render pass: Scene rendering with applied second bloom pass (when enabled)
		*/
		{
			VkClearValue clearValues[2]{};
			clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
			clearValues[1].depthStencil = { 1.0f, 0 };

			// Final composition
			VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
			renderPassBeginInfo.framebuffer = frameBuffers[currentImageIndex];
			renderPassBeginInfo.renderPass = renderPass;
			renderPassBeginInfo.clearValueCount = 2;
			renderPassBeginInfo.renderArea.extent.width = width;
			renderPassBeginInfo.renderArea.extent.height = height;
			renderPassBeginInfo.pClearValues = clearValues;

			vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
			vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

			VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);
			vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

			vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.composition, 0, 1, &descriptorSets[currentBuffer].composition, 0, nullptr);

			// Scene
			vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.composition);
			vkCmdDraw(cmdBuffer, 3, 1, 0, 0);

			// Bloom
			if (bloom) {
				vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.bloom[0]);
				vkCmdDraw(cmdBuffer, 3, 1, 0, 0);
			}

			drawUI(cmdBuffer);

			vkCmdEndRenderPass(cmdBuffer);
		}

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
		if (overlay->header("Settings")) {
			overlay->comboBox("Object type", &models.index, modelNames);
			overlay->inputFloat("Exposure", &uniformData.exposure, 0.025f, 3);
			overlay->checkBox("Bloom", &bloom);
			overlay->checkBox("Skybox", &displaySkybox);
		}
	}
};

VULKAN_EXAMPLE_MAIN()
