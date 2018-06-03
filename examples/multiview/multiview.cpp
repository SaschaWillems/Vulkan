/*
* Vulkan Example - Multiview (VK_KHR_multiview)
*
* VK_KHR_multiview allows rendering to multiple views of a single renderpass
*
* Copyright (C) 2018 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <vector>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vulkan/vulkan.h>
#include "vulkanexamplebase.h"
#include "VulkanModel.hpp"

#define ENABLE_VALIDATION false

class VulkanExample : public VulkanExampleBase
{
public:
	// Vertex layout for the models
	vks::VertexLayout vertexLayout = vks::VertexLayout({
		vks::VERTEX_COMPONENT_POSITION,
		vks::VERTEX_COMPONENT_NORMAL,
		vks::VERTEX_COMPONENT_COLOR,
	});

	struct ColorAttachment {
		VkImage image;
		VkImageView view;
		VkDeviceMemory memory;
	} colorAttachment;

	vks::Model scene;

	struct UBO {
		glm::mat4 projection[2];
		glm::mat4 modelview[2];
		glm::vec4 lightPos = glm::vec4(-2.5f, -3.5f, 0.0f, 1.0f);
	} ubo;

	vks::Buffer uniformBuffer;

	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;
	VkDescriptorSet descriptorSet;
	VkDescriptorSetLayout descriptorSetLayout;

	// Semaphore used to synchronize blit to swapchain
	VkSemaphore blitCompleteSemaphore;
	std::vector<VkCommandBuffer> blitCommandBuffers;
	std::vector<VkFence> blitWaitFences;

	// Camera and view properties
	float eyeSeparation = 0.08f;
	const float focalLength = 0.5f;
	const float fov = 90.0f;
	const float zNear = 0.1f;
	const float zFar = 256.0f;

	VulkanExample() : VulkanExampleBase(ENABLE_VALIDATION)
	{
		title = "Multiview";
		camera.type = Camera::CameraType::firstperson;
		camera.setRotation(glm::vec3(0.0f, 90.0f, 0.0f));
		camera.setTranslation(glm::vec3(7.0f, 3.2f, 0.0f));
		camera.movementSpeed = 5.0f;
		settings.overlay = false;

		enabledDeviceExtensions.push_back(VK_KHR_MULTIVIEW_EXTENSION_NAME);
		enabledInstanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	}

	~VulkanExample()
	{
		vkDestroyPipeline(device, pipeline, nullptr);

		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

		vkDestroyImageView(device, colorAttachment.view, nullptr);
		vkDestroyImage(device, colorAttachment.image, nullptr);
		vkFreeMemory(device, colorAttachment.memory, nullptr);

		vkDestroySemaphore(device, blitCompleteSemaphore, nullptr);
		for (auto& fence : blitWaitFences) {
			vkDestroyFence(device, fence, nullptr);
		}

		scene.destroy();

		uniformBuffer.destroy();
	}

	/*
		Custom depth/stencil setup
		Creates a depth/stencil framebuffer with multiple layers rendered to in a single pass
	*/
	void setupDepthStencil()
	{
		VkImageCreateInfo image = {};
		image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		image.pNext = NULL;
		image.imageType = VK_IMAGE_TYPE_2D;
		image.format = depthFormat;
		image.extent = { width, height, 1 };
		image.mipLevels = 1;
		image.arrayLayers = 2; // Two layers for two viewports
		image.samples = VK_SAMPLE_COUNT_1_BIT;
		image.tiling = VK_IMAGE_TILING_OPTIMAL;
		image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		image.flags = 0;

		VkMemoryAllocateInfo mem_alloc = {};
		mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		mem_alloc.pNext = NULL;
		mem_alloc.allocationSize = 0;
		mem_alloc.memoryTypeIndex = 0;

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
		depthStencilView.subresourceRange.layerCount = 1;

		VkMemoryRequirements memReqs;

		VK_CHECK_RESULT(vkCreateImage(device, &image, nullptr, &depthStencil.image));
		vkGetImageMemoryRequirements(device, depthStencil.image, &memReqs);
		mem_alloc.allocationSize = memReqs.size;
		mem_alloc.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device, &mem_alloc, nullptr, &depthStencil.mem));
		VK_CHECK_RESULT(vkBindImageMemory(device, depthStencil.image, depthStencil.mem, 0));

		depthStencilView.image = depthStencil.image;
		VK_CHECK_RESULT(vkCreateImageView(device, &depthStencilView, nullptr, &depthStencil.view));
	}

	/*
		Custom renderpass setup
	*/
	void setupRenderPass()
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
		attachments[0].finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
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
		subpassDescription.inputAttachmentCount = 0;
		subpassDescription.pInputAttachments = nullptr;
		subpassDescription.preserveAttachmentCount = 0;
		subpassDescription.pPreserveAttachments = nullptr;
		subpassDescription.pResolveAttachments = nullptr;

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
			Bit mask that specifices correlation between views
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

		VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassCI, nullptr, &renderPass));

		// The custom render pass does not include the swapchain images, so we need to do an initial layout transition

		VkCommandBuffer layoutCmd = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
		VkImageSubresourceRange subresourceRange { VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_MIP_LEVELS };
		for (uint32_t i = 0; i < swapChain.imageCount; i++) {
			vks::tools::setImageLayout(
				layoutCmd,
				swapChain.images[i],
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
				subresourceRange);
		}
		vulkanDevice->flushCommandBuffer(layoutCmd, queue);
	}

	/*
		Custom framebuffer setup
		Creates a color framebuffer with multiple layers rendered to in a single pass
	*/
	void setupFrameBuffer()
	{
		VkImageCreateInfo imageCI = vks::initializers::imageCreateInfo();
		imageCI.imageType = VK_IMAGE_TYPE_2D;
		imageCI.format = swapChain.colorFormat;
		imageCI.extent = { width, height, 1 };
		imageCI.mipLevels = 1;
		// Two layers for two views
		imageCI.arrayLayers = 2;
		imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCI.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

		VkMemoryRequirements memReqs;
		VK_CHECK_RESULT(vkCreateImage(device, &imageCI, nullptr, &colorAttachment.image));
		vkGetImageMemoryRequirements(device, colorAttachment.image, &memReqs);

		VkMemoryAllocateInfo memoryAllocInfo = vks::initializers::memoryAllocateInfo();
		memoryAllocInfo.allocationSize = memReqs.size;
		memoryAllocInfo.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device, &memoryAllocInfo, nullptr, &colorAttachment.memory));
		VK_CHECK_RESULT(vkBindImageMemory(device, colorAttachment.image, colorAttachment.memory, 0));

		VkImageViewCreateInfo imageViewCI = vks::initializers::imageViewCreateInfo();
		imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
		imageViewCI.format = swapChain.colorFormat;
		imageViewCI.flags = 0;
		imageViewCI.subresourceRange = {};
		imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewCI.subresourceRange.baseMipLevel = 0;
		imageViewCI.subresourceRange.levelCount = 1;
		imageViewCI.subresourceRange.baseArrayLayer = 0;
		// Two layers for two views
		imageViewCI.subresourceRange.layerCount = 2;
		imageViewCI.image = colorAttachment.image;
		VK_CHECK_RESULT(vkCreateImageView(device, &imageViewCI, nullptr, &colorAttachment.view));

		// Depth/Stencil attachment is the same for all frame buffers
		std::vector<VkImageView> attachments = { colorAttachment.view, depthStencil.view };

		VkFramebufferCreateInfo frameBufferCreateInfo = {};
		frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frameBufferCreateInfo.pNext = NULL;
		frameBufferCreateInfo.renderPass = renderPass;
		frameBufferCreateInfo.attachmentCount = 2;
		frameBufferCreateInfo.pAttachments = attachments.data();
		frameBufferCreateInfo.width = width;
		frameBufferCreateInfo.height = height;
		frameBufferCreateInfo.layers = 1;

		// Create frame buffers for every swap chain image
		frameBuffers.resize(swapChain.imageCount);
		for (uint32_t i = 0; i < frameBuffers.size(); i++) {
			VK_CHECK_RESULT(vkCreateFramebuffer(device, &frameBufferCreateInfo, nullptr, &frameBuffers[i]));
		}
	}

	void buildCommandBuffers()
	{
		/*
			Scene rendering
		*/

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

			VkViewport viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
			vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

			VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);
			vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

			vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

			VkDeviceSize offsets[1] = { 0 };
			vkCmdBindVertexBuffers(drawCmdBuffers[i], 0, 1, &scene.vertices.buffer, offsets);
			vkCmdBindIndexBuffer(drawCmdBuffers[i], scene.indices.buffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
			vkCmdDrawIndexed(drawCmdBuffers[i], scene.indexCount, 1, 0, 0, 0);

			vkCmdEndRenderPass(drawCmdBuffers[i]);

			VkImageSubresourceRange subresourceRange{};
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
			subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;

			VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
		}

		/*
			Layered color attachment to swapchain blit
		*/
		blitCommandBuffers.resize(drawCmdBuffers.size());

		VkCommandBufferAllocateInfo cmdBufAllocateInfo = vks::initializers::commandBufferAllocateInfo(cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, static_cast<uint32_t>(drawCmdBuffers.size()));
		VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, blitCommandBuffers.data()));

		for (int32_t i = 0; i < blitCommandBuffers.size(); ++i) {
			VK_CHECK_RESULT(vkBeginCommandBuffer(blitCommandBuffers[i], &cmdBufInfo));

			VkImageSubresourceRange subresourceRange { VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_MIP_LEVELS };

			vks::tools::setImageLayout(
				blitCommandBuffers[i],
				swapChain.images[i],
				VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				subresourceRange);

			VkImageBlit imageBlit{};
			imageBlit.srcOffsets[0] = { 0, 0, 0 };
			imageBlit.srcOffsets[1] = { static_cast<int32_t>(width), static_cast<int32_t>(height), 1 };
			imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageBlit.srcSubresource.layerCount = 1;
			imageBlit.dstSubresource = imageBlit.srcSubresource;

			// Blit first color attachment layer to the left of the swapchain image
			imageBlit.dstOffsets[0] = { 0, 0, 0 };
			imageBlit.dstOffsets[1] = { static_cast<int32_t>(width) / 2, static_cast<int32_t>(height), 1 };
			imageBlit.srcSubresource.baseArrayLayer = 0;
			vkCmdBlitImage(
				blitCommandBuffers[i], 
				colorAttachment.image, 
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
				swapChain.images[i], 
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
				1, 
				&imageBlit,
				VK_FILTER_NEAREST);

			// Blit second color attachment layer to the left of the swapchain image
			imageBlit.dstOffsets[0] = { static_cast<int32_t>(width) / 2, 0, 0 };
			imageBlit.dstOffsets[1] = { static_cast<int32_t>(width), static_cast<int32_t>(height), 1 };
			imageBlit.srcSubresource.baseArrayLayer = 1;
			vkCmdBlitImage(
				blitCommandBuffers[i], 
				colorAttachment.image, 
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
				swapChain.images[i], 
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
				1, 
				&imageBlit,
				VK_FILTER_NEAREST);

			vks::tools::setImageLayout(
				blitCommandBuffers[i],
				swapChain.images[i],
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
				subresourceRange);

			VK_CHECK_RESULT(vkEndCommandBuffer(blitCommandBuffers[i]));
		}

	}

	void loadAssets()
	{
		scene.loadFromFile(getAssetPath() + "models/sampleroom.dae", vertexLayout, 0.25f, vulkanDevice, queue);		
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
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
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
			// Binding 0: Vertex shader UBO
			vks::initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffer.descriptor),	
		};
		vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
	}

	void preparePipelines()
	{

		VkSemaphoreCreateInfo semaphoreCI = vks::initializers::semaphoreCreateInfo();
		VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCI, nullptr, &blitCompleteSemaphore));

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
		VkPipelineRasterizationStateCreateInfo rasterizationStateCI = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE);
		VkPipelineColorBlendAttachmentState blendAttachmentState = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
		VkPipelineColorBlendStateCreateInfo colorBlendStateCI =	vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
		VkPipelineDepthStencilStateCreateInfo depthStencilStateCI = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
		VkPipelineViewportStateCreateInfo viewportStateCI = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
		VkPipelineMultisampleStateCreateInfo multisampleStateCI = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);
		std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicStateCI = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);

		std::vector<VkVertexInputBindingDescription> vertexInputBindings = {
			vks::initializers::vertexInputBindingDescription(0, vertexLayout.stride(), VK_VERTEX_INPUT_RATE_VERTEX),
		};
		std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = {
			vks::initializers::vertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0),					// Location 0: Position			
			vks::initializers::vertexInputAttributeDescription(0, 1, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 3),	// Location 1: Normals			
			vks::initializers::vertexInputAttributeDescription(0, 2, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 6),	// Location 2: Color

		};
		VkPipelineVertexInputStateCreateInfo vertexInputState = vks::initializers::pipelineVertexInputStateCreateInfo();
		vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindings.size());
		vertexInputState.pVertexBindingDescriptions = vertexInputBindings.data();
		vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
		vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();

		VkGraphicsPipelineCreateInfo pipelineCI = vks::initializers::pipelineCreateInfo(pipelineLayout, renderPass);
		pipelineCI.pVertexInputState = &vertexInputState;
		pipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
		pipelineCI.pRasterizationState = &rasterizationStateCI;
		pipelineCI.pColorBlendState = &colorBlendStateCI;
		pipelineCI.pMultisampleState = &multisampleStateCI;
		pipelineCI.pViewportState = &viewportStateCI;
		pipelineCI.pDepthStencilState = &depthStencilStateCI;
		pipelineCI.pDynamicState = &dynamicStateCI;
		pipelineCI.renderPass = renderPass;

		/*
			Load shaders
			Contrary to the viewport array example we don't need a geometry shader for broadcasting
		*/
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;
		shaderStages[0] = loadShader(getAssetPath() + "shaders/multiview/multiview.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getAssetPath() + "shaders/multiview/multiview.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		pipelineCI.stageCount = 2;
		pipelineCI.pStages = shaderStages.data();

		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipeline));
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
		camFront.x = -cos(glm::radians(rotation.x)) * sin(glm::radians(rotation.y));
		camFront.y = sin(glm::radians(rotation.x));
		camFront.z = cos(glm::radians(rotation.x)) * cos(glm::radians(rotation.y));
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

		// Render
		VK_CHECK_RESULT(vkWaitForFences(device, 1, &waitFences[currentBuffer], VK_TRUE, UINT64_MAX));
		VK_CHECK_RESULT(vkResetFences(device, 1, &waitFences[currentBuffer]));
		submitInfo.pWaitSemaphores = &semaphores.presentComplete;
		submitInfo.pSignalSemaphores = &semaphores.renderComplete;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, waitFences[currentBuffer]));

		// Blit
		VK_CHECK_RESULT(vkWaitForFences(device, 1, &blitWaitFences[currentBuffer], VK_TRUE, UINT64_MAX));
		VK_CHECK_RESULT(vkResetFences(device, 1, &blitWaitFences[currentBuffer]));
		submitInfo.pWaitSemaphores = &semaphores.renderComplete;
		submitInfo.pSignalSemaphores = &blitCompleteSemaphore;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &blitCommandBuffers[currentBuffer];
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, blitWaitFences[currentBuffer]));

		VK_CHECK_RESULT(swapChain.queuePresent(queue, currentBuffer, blitCompleteSemaphore));
	}

	void prepare()
	{
		VulkanExampleBase::prepare();
		loadAssets();
		prepareUniformBuffers();
		prepareDescriptors();
		preparePipelines();
		buildCommandBuffers();

		VkFenceCreateInfo fenceCreateInfo = vks::initializers::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
		blitWaitFences.resize(blitCommandBuffers.size());
		for (auto& fence : blitWaitFences) {
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
		}
	}

};

VULKAN_EXAMPLE_MAIN()