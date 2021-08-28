/*
* Vulkan Example - Multiview (VK_KHR_multiview)
*
* Uses VK_KHR_multiview for simultaneously rendering to multiple views and displays these with barrel distortion using a fragment shader
*
* Copyright (C) 2018 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "vulkanexamplebase.h"
#include "VulkanglTFModel.h"

#define ENABLE_VALIDATION false

class VulkanExample : public VulkanExampleBase
{
public:
	struct MultiviewPass {
		struct FrameBufferAttachment {
			VkImage image;
			VkDeviceMemory memory;
			VkImageView view;
		} color, depth;
		VkFramebuffer frameBuffer;
		VkRenderPass renderPass;
		VkDescriptorImageInfo descriptor;
		VkSampler sampler;
		VkSemaphore semaphore;
		std::vector<VkCommandBuffer> commandBuffers;
		std::vector<VkFence> waitFences;
	} multiviewPass;

	vkglTF::Model scene;

	struct UBO {
		glm::mat4 projection[2];
		glm::mat4 modelview[2];
		glm::vec4 lightPos = glm::vec4(-2.5f, -3.5f, 0.0f, 1.0f);
		float distortionAlpha = 0.2f;
	} ubo;

	vks::Buffer uniformBuffer;

	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;
	VkDescriptorSet descriptorSet;
	VkDescriptorSetLayout descriptorSetLayout;

	VkPipeline viewDisplayPipelines[2];

	// Camera and view properties
	float eyeSeparation = 0.08f;
	const float focalLength = 0.5f;
	const float fov = 90.0f;
	const float zNear = 0.1f;
	const float zFar = 256.0f;

	VulkanExample() : VulkanExampleBase(ENABLE_VALIDATION)
	{
		title = "Multiview rendering";
		camera.type = Camera::CameraType::firstperson;
		camera.setRotation(glm::vec3(0.0f, 90.0f, 0.0f));
		camera.setTranslation(glm::vec3(7.0f, 3.2f, 0.0f));
		camera.movementSpeed = 5.0f;

		// Enable extension required for multiview
		enabledDeviceExtensions.push_back(VK_KHR_MULTIVIEW_EXTENSION_NAME);

		// Reading device properties and features for multiview requires VK_KHR_get_physical_device_properties2 to be enabled
		enabledInstanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	}

	~VulkanExample()
	{
		vkDestroyPipeline(device, pipeline, nullptr);

		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

		// Multiview pass

		vkDestroyImageView(device, multiviewPass.color.view, nullptr);
		vkDestroyImage(device, multiviewPass.color.image, nullptr);
		vkFreeMemory(device, multiviewPass.color.memory, nullptr);
		vkDestroyImageView(device, multiviewPass.depth.view, nullptr);
		vkDestroyImage(device, multiviewPass.depth.image, nullptr);
		vkFreeMemory(device, multiviewPass.depth.memory, nullptr);

		vkDestroyRenderPass(device, multiviewPass.renderPass, nullptr);
		vkDestroySampler(device, multiviewPass.sampler, nullptr);
		vkDestroyFramebuffer(device, multiviewPass.frameBuffer, nullptr);

		vkDestroySemaphore(device, multiviewPass.semaphore, nullptr);
		for (auto& fence : multiviewPass.waitFences) {
			vkDestroyFence(device, fence, nullptr);
		}

		for (auto& pipeline : viewDisplayPipelines) {
			vkDestroyPipeline(device, pipeline, nullptr);
		}

		uniformBuffer.destroy();
	}

	/*
		Prepares all resources required for the multiview attachment
		Images, views, attachments, renderpass, framebuffer, etc.
	*/
	void prepareMultiview()
	{
		// Example renders to two views (left/right)
		const uint32_t multiviewLayerCount = 2;

		/*
			Layered depth/stencil framebuffer
		*/
		{
			VkImageCreateInfo imageCI= vks::initializers::imageCreateInfo();
			imageCI.imageType = VK_IMAGE_TYPE_2D;
			imageCI.format = depthFormat;
			imageCI.extent = { width, height, 1 };
			imageCI.mipLevels = 1;
			imageCI.arrayLayers = multiviewLayerCount;
			imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
			imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			imageCI.flags = 0;
			VK_CHECK_RESULT(vkCreateImage(device, &imageCI, nullptr, &multiviewPass.depth.image));

			VkMemoryRequirements memReqs;
			vkGetImageMemoryRequirements(device, multiviewPass.depth.image, &memReqs);

			VkMemoryAllocateInfo memAllocInfo{};
			memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			memAllocInfo.allocationSize = 0;
			memAllocInfo.memoryTypeIndex = 0;

			VkImageViewCreateInfo depthStencilView = {};
			depthStencilView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			depthStencilView.pNext = NULL;
			depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
			depthStencilView.format = depthFormat;
			depthStencilView.flags = 0;
			depthStencilView.subresourceRange = {};
			depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
			depthStencilView.subresourceRange.baseMipLevel = 0;
			depthStencilView.subresourceRange.levelCount = 1;
			depthStencilView.subresourceRange.baseArrayLayer = 0;
			depthStencilView.subresourceRange.layerCount = multiviewLayerCount;
			depthStencilView.image = multiviewPass.depth.image;

			memAllocInfo.allocationSize = memReqs.size;
			memAllocInfo.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			VK_CHECK_RESULT(vkAllocateMemory(device, &memAllocInfo, nullptr, &multiviewPass.depth.memory));
			VK_CHECK_RESULT(vkBindImageMemory(device, multiviewPass.depth.image, multiviewPass.depth.memory, 0));
			VK_CHECK_RESULT(vkCreateImageView(device, &depthStencilView, nullptr, &multiviewPass.depth.view));
		}

		/*
			Layered color attachment
		*/
		{
			VkImageCreateInfo imageCI = vks::initializers::imageCreateInfo();
			imageCI.imageType = VK_IMAGE_TYPE_2D;
			imageCI.format = swapChain.colorFormat;
			imageCI.extent = { width, height, 1 };
			imageCI.mipLevels = 1;
			imageCI.arrayLayers = multiviewLayerCount;
			imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
			imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageCI.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			VK_CHECK_RESULT(vkCreateImage(device, &imageCI, nullptr, &multiviewPass.color.image));

			VkMemoryRequirements memReqs;
			vkGetImageMemoryRequirements(device, multiviewPass.color.image, &memReqs);

			VkMemoryAllocateInfo memoryAllocInfo = vks::initializers::memoryAllocateInfo();
			memoryAllocInfo.allocationSize = memReqs.size;
			memoryAllocInfo.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			VK_CHECK_RESULT(vkAllocateMemory(device, &memoryAllocInfo, nullptr, &multiviewPass.color.memory));
			VK_CHECK_RESULT(vkBindImageMemory(device, multiviewPass.color.image, multiviewPass.color.memory, 0));

			VkImageViewCreateInfo imageViewCI = vks::initializers::imageViewCreateInfo();
			imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
			imageViewCI.format = swapChain.colorFormat;
			imageViewCI.flags = 0;
			imageViewCI.subresourceRange = {};
			imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageViewCI.subresourceRange.baseMipLevel = 0;
			imageViewCI.subresourceRange.levelCount = 1;
			imageViewCI.subresourceRange.baseArrayLayer = 0;
			imageViewCI.subresourceRange.layerCount = multiviewLayerCount;
			imageViewCI.image = multiviewPass.color.image;
			VK_CHECK_RESULT(vkCreateImageView(device, &imageViewCI, nullptr, &multiviewPass.color.view));

			// Create sampler to sample from the attachment in the fragment shader
			VkSamplerCreateInfo samplerCI = vks::initializers::samplerCreateInfo();
			samplerCI.magFilter = VK_FILTER_NEAREST;
			samplerCI.minFilter = VK_FILTER_NEAREST;
			samplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			samplerCI.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerCI.addressModeV = samplerCI.addressModeU;
			samplerCI.addressModeW = samplerCI.addressModeU;
			samplerCI.mipLodBias = 0.0f;
			samplerCI.maxAnisotropy = 1.0f;
			samplerCI.minLod = 0.0f;
			samplerCI.maxLod = 1.0f;
			samplerCI.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
			VK_CHECK_RESULT(vkCreateSampler(device, &samplerCI, nullptr, &multiviewPass.sampler));

			// Fill a descriptor for later use in a descriptor set
			multiviewPass.descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			multiviewPass.descriptor.imageView = multiviewPass.color.view;
			multiviewPass.descriptor.sampler = multiviewPass.sampler;
		}

		/*
			Renderpass
		*/
		{
			std::array<VkAttachmentDescription, 2> attachments = {};
			// Color attachment
			attachments[0].format = swapChain.colorFormat;
			attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
			attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attachments[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			// Depth attachment
			attachments[1].format = depthFormat;
			attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
			attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			VkAttachmentReference colorReference = {};
			colorReference.attachment = 0;
			colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkAttachmentReference depthReference = {};
			depthReference.attachment = 1;
			depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			VkSubpassDescription subpassDescription = {};
			subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpassDescription.colorAttachmentCount = 1;
			subpassDescription.pColorAttachments = &colorReference;
			subpassDescription.pDepthStencilAttachment = &depthReference;

			// Subpass dependencies for layout transitions
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

			VkRenderPassCreateInfo renderPassCI{};
			renderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassCI.attachmentCount = static_cast<uint32_t>(attachments.size());
			renderPassCI.pAttachments = attachments.data();
			renderPassCI.subpassCount = 1;
			renderPassCI.pSubpasses = &subpassDescription;
			renderPassCI.dependencyCount = static_cast<uint32_t>(dependencies.size());
			renderPassCI.pDependencies = dependencies.data();

			/*
				Setup multiview info for the renderpass
			*/

			/*
				Bit mask that specifies which view rendering is broadcast to
				0011 = Broadcast to first and second view (layer)
			*/
			const uint32_t viewMask = 0b00000011;

			/*
				Bit mask that specifies correlation between views
				An implementation may use this for optimizations (concurrent render)
			*/
			const uint32_t correlationMask = 0b00000011;

			VkRenderPassMultiviewCreateInfo renderPassMultiviewCI{};
			renderPassMultiviewCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_CREATE_INFO;
			renderPassMultiviewCI.subpassCount = 1;
			renderPassMultiviewCI.pViewMasks = &viewMask;
			renderPassMultiviewCI.correlationMaskCount = 1;
			renderPassMultiviewCI.pCorrelationMasks = &correlationMask;

			renderPassCI.pNext = &renderPassMultiviewCI;

			VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassCI, nullptr, &multiviewPass.renderPass));
		}

		/*
			Framebuffer
		*/
		{
			VkImageView attachments[2];
			attachments[0] = multiviewPass.color.view;
			attachments[1] = multiviewPass.depth.view;

			VkFramebufferCreateInfo framebufferCI = vks::initializers::framebufferCreateInfo();
			framebufferCI.renderPass = multiviewPass.renderPass;
			framebufferCI.attachmentCount = 2;
			framebufferCI.pAttachments = attachments;
			framebufferCI.width = width;
			framebufferCI.height = height;
			framebufferCI.layers = 1;
			VK_CHECK_RESULT(vkCreateFramebuffer(device, &framebufferCI, nullptr, &multiviewPass.frameBuffer));
		}
	}

	void buildCommandBuffers()
	{
		/*
			View display
		*/
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
				VkViewport viewport = vks::initializers::viewport((float)width / 2.0f, (float)height, 0.0f, 1.0f);
				VkRect2D scissor = vks::initializers::rect2D(width / 2, height, 0, 0);
				vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);
				vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

				vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

				// Left eye
				vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, viewDisplayPipelines[0]);
				vkCmdDraw(drawCmdBuffers[i], 3, 1, 0, 0);

				// Right eye
				viewport.x = (float)width / 2;
				scissor.offset.x = width / 2;
				vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);
				vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

				vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, viewDisplayPipelines[1]);
				vkCmdDraw(drawCmdBuffers[i], 3, 1, 0, 0);

				drawUI(drawCmdBuffers[i]);

				vkCmdEndRenderPass(drawCmdBuffers[i]);
				VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
			}
		}

		/*
			Multiview layered attachment scene rendering
		*/

		multiviewPass.commandBuffers.resize(drawCmdBuffers.size());

		VkCommandBufferAllocateInfo cmdBufAllocateInfo = vks::initializers::commandBufferAllocateInfo(cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, static_cast<uint32_t>(drawCmdBuffers.size()));
		VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, multiviewPass.commandBuffers.data()));

		{
			VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

			VkClearValue clearValues[2];
			clearValues[0].color = defaultClearColor;
			clearValues[1].depthStencil = { 1.0f, 0 };

			VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
			renderPassBeginInfo.renderPass = multiviewPass.renderPass;
			renderPassBeginInfo.renderArea.offset.x = 0;
			renderPassBeginInfo.renderArea.offset.y = 0;
			renderPassBeginInfo.renderArea.extent.width = width;
			renderPassBeginInfo.renderArea.extent.height = height;
			renderPassBeginInfo.clearValueCount = 2;
			renderPassBeginInfo.pClearValues = clearValues;

			for (int32_t i = 0; i < multiviewPass.commandBuffers.size(); ++i) {
				renderPassBeginInfo.framebuffer = multiviewPass.frameBuffer;

				VK_CHECK_RESULT(vkBeginCommandBuffer(multiviewPass.commandBuffers[i], &cmdBufInfo));
				vkCmdBeginRenderPass(multiviewPass.commandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
				VkViewport viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
				vkCmdSetViewport(multiviewPass.commandBuffers[i], 0, 1, &viewport);
				VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);
				vkCmdSetScissor(multiviewPass.commandBuffers[i], 0, 1, &scissor);

				vkCmdBindDescriptorSets(multiviewPass.commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
				vkCmdBindPipeline(multiviewPass.commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
				scene.draw(multiviewPass.commandBuffers[i]);

				vkCmdEndRenderPass(multiviewPass.commandBuffers[i]);
				VK_CHECK_RESULT(vkEndCommandBuffer(multiviewPass.commandBuffers[i]));
			}
		}
	}

	void loadAssets()
	{
		scene.loadFromFile(getAssetPath() + "models/sampleroom.gltf", vulkanDevice, queue, vkglTF::FileLoadingFlags::PreTransformVertices | vkglTF::FileLoadingFlags::PreMultiplyVertexColors | vkglTF::FileLoadingFlags::FlipY);
	}

	void prepareDescriptors()
	{
		/*
			Pool
		*/
		std::vector<VkDescriptorPoolSize> poolSizes = {
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1),
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
		};
		VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(static_cast<uint32_t>(poolSizes.size()), poolSizes.data(), 1);
		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));

		/*
			Layouts
		*/
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0),
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1)
		};
		VkDescriptorSetLayoutCreateInfo descriptorLayout = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayout));
		VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo =vks::initializers::pipelineLayoutCreateInfo(&descriptorSetLayout, 1);
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &pipelineLayout));

		/*
			Descriptors
		*/
		VkDescriptorSetAllocateInfo allocateInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocateInfo, &descriptorSet));
		std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
			vks::initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffer.descriptor),
			vks::initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &multiviewPass.descriptor),
		};
		vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
	}

	void preparePipelines()
	{

		VkSemaphoreCreateInfo semaphoreCI = vks::initializers::semaphoreCreateInfo();
		VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCI, nullptr, &multiviewPass.semaphore));

		/*
			Display multi view features and properties
		*/

		VkPhysicalDeviceFeatures2KHR deviceFeatures2{};
		VkPhysicalDeviceMultiviewFeaturesKHR extFeatures{};
		extFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES_KHR;
		deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR;
		deviceFeatures2.pNext = &extFeatures;
		PFN_vkGetPhysicalDeviceFeatures2KHR vkGetPhysicalDeviceFeatures2KHR = reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures2KHR>(vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceFeatures2KHR"));
		vkGetPhysicalDeviceFeatures2KHR(physicalDevice, &deviceFeatures2);
		std::cout << "Multiview features:" << std::endl;
		std::cout << "\tmultiview = " << extFeatures.multiview << std::endl;
		std::cout << "\tmultiviewGeometryShader = " << extFeatures.multiviewGeometryShader << std::endl;
		std::cout << "\tmultiviewTessellationShader = " << extFeatures.multiviewTessellationShader << std::endl;
		std::cout << std::endl;

		VkPhysicalDeviceProperties2KHR deviceProps2{};
		VkPhysicalDeviceMultiviewPropertiesKHR extProps{};
		extProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_PROPERTIES_KHR;
		deviceProps2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;
		deviceProps2.pNext = &extProps;
		PFN_vkGetPhysicalDeviceProperties2KHR vkGetPhysicalDeviceProperties2KHR = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2KHR>(vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceProperties2KHR"));
		vkGetPhysicalDeviceProperties2KHR(physicalDevice, &deviceProps2);
		std::cout << "Multiview properties:" << std::endl;
		std::cout << "\tmaxMultiviewViewCount = " << extProps.maxMultiviewViewCount << std::endl;
		std::cout << "\tmaxMultiviewInstanceIndex = " << extProps.maxMultiviewInstanceIndex << std::endl;

		/*
			Create graphics pipeline
		*/

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
		VkPipelineRasterizationStateCreateInfo rasterizationStateCI = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
		VkPipelineColorBlendAttachmentState blendAttachmentState = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
		VkPipelineColorBlendStateCreateInfo colorBlendStateCI =	vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
		VkPipelineDepthStencilStateCreateInfo depthStencilStateCI = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
		VkPipelineViewportStateCreateInfo viewportStateCI = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
		VkPipelineMultisampleStateCreateInfo multisampleStateCI = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);
		std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicStateCI = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);

		VkGraphicsPipelineCreateInfo pipelineCI = vks::initializers::pipelineCreateInfo(pipelineLayout, multiviewPass.renderPass);
		pipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
		pipelineCI.pRasterizationState = &rasterizationStateCI;
		pipelineCI.pColorBlendState = &colorBlendStateCI;
		pipelineCI.pMultisampleState = &multisampleStateCI;
		pipelineCI.pViewportState = &viewportStateCI;
		pipelineCI.pDepthStencilState = &depthStencilStateCI;
		pipelineCI.pDynamicState = &dynamicStateCI;
		pipelineCI.pVertexInputState  = vkglTF::Vertex::getPipelineVertexInputState({vkglTF::VertexComponent::Position, vkglTF::VertexComponent::Normal, vkglTF::VertexComponent::Color});

		/*
			Load shaders
			Contrary to the viewport array example we don't need a geometry shader for broadcasting
		*/
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;
		shaderStages[0] = loadShader(getShadersPath() + "multiview/multiview.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getShadersPath() + "multiview/multiview.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		pipelineCI.stageCount = 2;
		pipelineCI.pStages = shaderStages.data();
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipeline));

		/*
			Full screen pass
		*/

		float multiviewArrayLayer = 0.0f;

		VkSpecializationMapEntry specializationMapEntry{ 0, 0, sizeof(float) };

		VkSpecializationInfo specializationInfo{};
		specializationInfo.dataSize = sizeof(float);
		specializationInfo.mapEntryCount = 1;
		specializationInfo.pMapEntries = &specializationMapEntry;
		specializationInfo.pData = &multiviewArrayLayer;

		rasterizationStateCI.cullMode = VK_CULL_MODE_FRONT_BIT;

		/*
			Separate pipelines per eye (view) using specialization constants to set view array layer to sample from
		*/
		for (uint32_t i = 0; i < 2; i++) {
			shaderStages[0] = loadShader(getShadersPath() + "multiview/viewdisplay.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
			shaderStages[1] = loadShader(getShadersPath() + "multiview/viewdisplay.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
			shaderStages[1].pSpecializationInfo = &specializationInfo;
			multiviewArrayLayer = (float)i;
			VkPipelineVertexInputStateCreateInfo emptyInputState = vks::initializers::pipelineVertexInputStateCreateInfo();
			pipelineCI.pVertexInputState = &emptyInputState;
			pipelineCI.layout = pipelineLayout;
			pipelineCI.renderPass = renderPass;
			VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &viewDisplayPipelines[i]));
		}

	}

	// Prepare and initialize uniform buffer containing shader uniforms
	void prepareUniformBuffers()
	{
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&uniformBuffer,
			sizeof(ubo)));
		VK_CHECK_RESULT(uniformBuffer.map());
		updateUniformBuffers();
	}

	void updateUniformBuffers()
	{
		// Matrices for the two viewports
		// See http://paulbourke.net/stereographics/stereorender/

		// Calculate some variables
		float aspectRatio = (float)(width * 0.5f) / (float)height;
		float wd2 = zNear * tan(glm::radians(fov / 2.0f));
		float ndfl = zNear / focalLength;
		float left, right;
		float top = wd2;
		float bottom = -wd2;

		glm::vec3 camFront;
		camFront.x = -cos(glm::radians(camera.rotation.x)) * sin(glm::radians(camera.rotation.y));
		camFront.y = sin(glm::radians(camera.rotation.x));
		camFront.z = cos(glm::radians(camera.rotation.x)) * cos(glm::radians(camera.rotation.y));
		camFront = glm::normalize(camFront);
		glm::vec3 camRight = glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 1.0f, 0.0f)));

		glm::mat4 rotM = glm::mat4(1.0f);
		glm::mat4 transM;

		rotM = glm::rotate(rotM, glm::radians(camera.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		rotM = glm::rotate(rotM, glm::radians(camera.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		rotM = glm::rotate(rotM, glm::radians(camera.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

		// Left eye
		left = -aspectRatio * wd2 + 0.5f * eyeSeparation * ndfl;
		right = aspectRatio * wd2 + 0.5f * eyeSeparation * ndfl;

		transM = glm::translate(glm::mat4(1.0f), camera.position - camRight * (eyeSeparation / 2.0f));

		ubo.projection[0] = glm::frustum(left, right, bottom, top, zNear, zFar);
		ubo.modelview[0] = rotM * transM;

		// Right eye
		left = -aspectRatio * wd2 - 0.5f * eyeSeparation * ndfl;
		right = aspectRatio * wd2 - 0.5f * eyeSeparation * ndfl;

		transM = glm::translate(glm::mat4(1.0f), camera.position + camRight * (eyeSeparation / 2.0f));

		ubo.projection[1] = glm::frustum(left, right, bottom, top, zNear, zFar);
		ubo.modelview[1] = rotM * transM;

		memcpy(uniformBuffer.mapped, &ubo, sizeof(ubo));
	}

	void draw()
	{
		VulkanExampleBase::prepareFrame();

		// Multiview offscreen render
		VK_CHECK_RESULT(vkWaitForFences(device, 1, &multiviewPass.waitFences[currentBuffer], VK_TRUE, UINT64_MAX));
		VK_CHECK_RESULT(vkResetFences(device, 1, &multiviewPass.waitFences[currentBuffer]));
		submitInfo.pWaitSemaphores = &semaphores.presentComplete;
		submitInfo.pSignalSemaphores = &multiviewPass.semaphore;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &multiviewPass.commandBuffers[currentBuffer];
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, multiviewPass.waitFences[currentBuffer]));

		// View display
		VK_CHECK_RESULT(vkWaitForFences(device, 1, &waitFences[currentBuffer], VK_TRUE, UINT64_MAX));
		VK_CHECK_RESULT(vkResetFences(device, 1, &waitFences[currentBuffer]));
		submitInfo.pWaitSemaphores = &multiviewPass.semaphore;
		submitInfo.pSignalSemaphores = &semaphores.renderComplete;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, waitFences[currentBuffer]));

		VulkanExampleBase::submitFrame();
	}

	void prepare()
	{
		VulkanExampleBase::prepare();
		loadAssets();
		prepareMultiview();
		prepareUniformBuffers();
		prepareDescriptors();
		preparePipelines();
		buildCommandBuffers();

		VkFenceCreateInfo fenceCreateInfo = vks::initializers::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
		multiviewPass.waitFences.resize(multiviewPass.commandBuffers.size());
		for (auto& fence : multiviewPass.waitFences) {
			VK_CHECK_RESULT(vkCreateFence(device, &fenceCreateInfo, nullptr, &fence));
		}

		prepared = true;
	}

	virtual void render()
	{
		if (!prepared)
			return;
		draw();
		if (camera.updated) {
			updateUniformBuffers();
		}
	}

	virtual void OnUpdateUIOverlay(vks::UIOverlay *overlay)
	{
		if (overlay->header("Settings")) {
			if (overlay->sliderFloat("Eye separation", &eyeSeparation, -1.0f, 1.0f)) {
				updateUniformBuffers();
			}
			if (overlay->sliderFloat("Barrel distortion", &ubo.distortionAlpha, -0.6f, 0.6f)) {
				updateUniformBuffers();
			}
		}
	}

};

VULKAN_EXAMPLE_MAIN()