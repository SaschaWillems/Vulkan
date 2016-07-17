/*
* Vulkan framebuffer class
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#include <algorithm>
#include <iterator>
#include <vector>
#include "vulkan/vulkan.h"
#include "vulkandevice.hpp"
#include "vulkantools.h"

namespace vk
{
	/**
	* @brief Encapsulates a single frame buffer attachment 
	*/
	struct FramebufferAttachment
	{
		VkImage image;
		VkDeviceMemory memory;
		VkImageView view;
		VkFormat format;
		VkImageSubresourceRange subresourceRange;
		VkAttachmentDescription description;
		VkImageLayout initialLayout;

		/**
		* @brief Returns true if the attachment has a depth component
		*/
		bool hasDepth()
		{
			std::vector<VkFormat> formats = 
			{
				VK_FORMAT_D16_UNORM,
				VK_FORMAT_X8_D24_UNORM_PACK32,
				VK_FORMAT_D32_SFLOAT,
				VK_FORMAT_D16_UNORM_S8_UINT,
				VK_FORMAT_D24_UNORM_S8_UINT,
				VK_FORMAT_D32_SFLOAT_S8_UINT,
			};
			return std::find(formats.begin(), formats.end(), format) != std::end(formats);
		}

		/**
		* @brief Returns true if the attachment has a stencil component
		*/
		bool hasStencil()
		{
			std::vector<VkFormat> formats = 
			{
				VK_FORMAT_S8_UINT,
				VK_FORMAT_D16_UNORM_S8_UINT,
				VK_FORMAT_D24_UNORM_S8_UINT,
				VK_FORMAT_D32_SFLOAT_S8_UINT,
			};
			return std::find(formats.begin(), formats.end(), format) != std::end(formats);
		}

		/**
		* @brief Returns true if the attachment is a depth and/or stencil attachment
		*/
		bool isDepthStencil()
		{
			return(hasDepth() || hasStencil());
		}

	};

	/**
	* @brief Describes the attributes of an attachment to be created
	*/
	struct AttachmentCreateInfo
	{
		uint32_t width, height;
		uint32_t layerCount;
		VkFormat format;
		VkImageUsageFlags usage;
	};

	/**
	* @brief Encaspulates a complete Vulkan framebuffer with an arbitrary number and combination of attachments
	*/
	struct Framebuffer
	{
	private:
		vk::VulkanDevice *vulkanDevice;
	public:
		uint32_t width, height;
		VkFramebuffer framebuffer;
		VkRenderPass renderPass;
		VkSampler sampler;
		std::vector<vk::FramebufferAttachment> attachments;

		/**
		* Default constructor
		*
		* @param vulkanDevice Pointer to a valid VulkanDevice
		*/
		Framebuffer(vk::VulkanDevice *vulkanDevice)
		{
			assert(vulkanDevice);
			this->vulkanDevice = vulkanDevice;
		}

		/**
		* Destroy and free Vulkan resources used for the framebuffer and all of it's attachments
		*/
		~Framebuffer()
		{
			assert(vulkanDevice);
			for (auto attachment : attachments)
			{
				vkDestroyImage(vulkanDevice->device, attachment.image, nullptr);
				vkDestroyImageView(vulkanDevice->device, attachment.view, nullptr);
				vkFreeMemory(vulkanDevice->device, attachment.memory, nullptr);
			}
			vkDestroySampler(vulkanDevice->device, sampler, nullptr);
			vkDestroyRenderPass(vulkanDevice->device, renderPass, nullptr);
			vkDestroyFramebuffer(vulkanDevice->device, framebuffer, nullptr);
		}

		/**
		* Add a new attachment described by createinfo to the framebuffer's attachment list
		*
		* @param createinfo Structure that specifices the framebuffer to be constructed
		* @param layoutCmd A valid and active command buffer used for the initial layout transitions
		*
		* @return Index of the new attachment
		*/
		uint32_t addAttachment(vk::AttachmentCreateInfo createinfo, VkCommandBuffer layoutCmd)
		{
			vk::FramebufferAttachment attachment;

			attachment.format = createinfo.format;

			VkImageAspectFlags aspectMask = VK_FLAGS_NONE;
			VkImageLayout imageLayout;

			// Select aspect mask and layout depending on usage

			// Color attachment
			if (createinfo.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
			{
				aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				imageLayout = (createinfo.usage & VK_IMAGE_USAGE_SAMPLED_BIT) ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			}

			// Depth (and/or stencil) attachment
			if (createinfo.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
			{
				if (attachment.hasDepth())
				{
					aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
				}
				if (attachment.hasStencil())
				{
					aspectMask = aspectMask | VK_IMAGE_ASPECT_STENCIL_BIT;
				}
				attachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				imageLayout = (createinfo.usage & VK_IMAGE_USAGE_SAMPLED_BIT) ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			}

			assert(aspectMask > 0);

			VkImageCreateInfo image = vkTools::initializers::imageCreateInfo();
			image.imageType = VK_IMAGE_TYPE_2D;
			image.format = createinfo.format;
			image.extent.width = createinfo.width;
			image.extent.height = createinfo.height;
			image.extent.depth = 1;
			image.mipLevels = 1;
			image.arrayLayers = createinfo.layerCount;
			image.samples = VK_SAMPLE_COUNT_1_BIT;
			image.tiling = VK_IMAGE_TILING_OPTIMAL;
			image.usage = createinfo.usage;

			VkMemoryAllocateInfo memAlloc = vkTools::initializers::memoryAllocateInfo();
			VkMemoryRequirements memReqs;

			// Create image for this attachment
			VK_CHECK_RESULT(vkCreateImage(vulkanDevice->device, &image, nullptr, &attachment.image));
			vkGetImageMemoryRequirements(vulkanDevice->device, attachment.image, &memReqs);
			memAlloc.allocationSize = memReqs.size;
			memAlloc.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			VK_CHECK_RESULT(vkAllocateMemory(vulkanDevice->device, &memAlloc, nullptr, &attachment.memory));
			VK_CHECK_RESULT(vkBindImageMemory(vulkanDevice->device, attachment.image, attachment.memory, 0));

			attachment.subresourceRange = {};
			attachment.subresourceRange.aspectMask = aspectMask;
			attachment.subresourceRange.levelCount = 1;
			attachment.subresourceRange.layerCount = createinfo.layerCount;

			// Set the initial layout to shader read instead of attachment 
			// Note that the render loop has to take care of the transition from read to attachment
			vkTools::setImageLayout(
				layoutCmd,
				attachment.image,
				aspectMask,
				VK_IMAGE_LAYOUT_UNDEFINED,
				imageLayout,
				attachment.subresourceRange);

			VkImageViewCreateInfo imageView = vkTools::initializers::imageViewCreateInfo();
			imageView.viewType = (createinfo.layerCount == 1) ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_2D_ARRAY;
			imageView.format = createinfo.format;
			imageView.subresourceRange = attachment.subresourceRange;
			//todo: workaround for depth+stencil attachments
			imageView.subresourceRange.aspectMask = (attachment.hasDepth()) ? VK_IMAGE_ASPECT_DEPTH_BIT : aspectMask;
			imageView.image = attachment.image;
			VK_CHECK_RESULT(vkCreateImageView(vulkanDevice->device, &imageView, nullptr, &attachment.view));

			// Fill attachment description
			attachment.description = {};
			attachment.description.samples = VK_SAMPLE_COUNT_1_BIT;
			attachment.description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachment.description.storeOp = (createinfo.usage & VK_IMAGE_USAGE_SAMPLED_BIT) ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachment.description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachment.description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachment.description.format = createinfo.format;
			if (attachment.hasDepth() || attachment.hasStencil())
			{
				attachment.description.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				attachment.description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			}
			else
			{
				attachment.description.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				attachment.description.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			}

			attachments.push_back(attachment);

			return static_cast<uint32_t>(attachments.size() - 1);
		}

		/**
		* Creates a default sampler for sampling from any of the framebuffer attachments
		* Applications are free to create their own samplers for different use cases 
		*
		* @param magFilter Magnification filter for lookups
		* @param minFilter Minification filter for lookups
		* @param adressMode Adressing mode for the U,V and W coordinates
		*
		* @return VkResult for the sampler creation
		*/
		VkResult createSampler(VkFilter magFilter, VkFilter minFilter, VkSamplerAddressMode adressMode)
		{
			VkSamplerCreateInfo samplerInfo = vkTools::initializers::samplerCreateInfo();
			samplerInfo.magFilter = magFilter;
			samplerInfo.minFilter = minFilter;
			samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			samplerInfo.addressModeU = adressMode;
			samplerInfo.addressModeV = adressMode;
			samplerInfo.addressModeW = adressMode;
			samplerInfo.mipLodBias = 0.0f;
			samplerInfo.maxAnisotropy = 0;
			samplerInfo.minLod = 0.0f;
			samplerInfo.maxLod = 1.0f;
			samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
			return vkCreateSampler(vulkanDevice->device, &samplerInfo, nullptr, &sampler);
		}

		/**
		* Creates a default render pass setup with one sub pass
		*
		* @return VK_SUCCESS if all resources have been created successfully
		*/
		VkResult createRenderPass()
		{
			std::vector<VkAttachmentDescription> attachmentDescriptions;
			for (auto& attachment : attachments)
			{
				attachmentDescriptions.push_back(attachment.description);
			};

			// Collect attachment references
			std::vector<VkAttachmentReference> colorReferences;
			VkAttachmentReference depthReference = {};
			bool hasDepth = false; 
			bool hasColor = false;

			uint32_t attachmentIndex = 0;

			for (auto& attachment : attachments)
			{
				if (attachment.isDepthStencil())
				{
					// Only one depth attachment allowed
					assert(!hasDepth);
					depthReference.attachment = attachmentIndex;
					depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
					hasDepth = true;
				}
				else
				{
					colorReferences.push_back({ attachmentIndex, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
					hasColor = true;
				}
				attachmentIndex++;
			};

			// Default render pass setup uses only one subpass
			VkSubpassDescription subpass = {};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			if (hasColor)
			{
				subpass.pColorAttachments = colorReferences.data();
				subpass.colorAttachmentCount = static_cast<uint32_t>(colorReferences.size());
			}
			if (hasDepth)
			{
				subpass.pDepthStencilAttachment = &depthReference;
			}

			// Create render pass
			VkRenderPassCreateInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassInfo.pAttachments = attachmentDescriptions.data();
			renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpass;
			VK_CHECK_RESULT(vkCreateRenderPass(vulkanDevice->device, &renderPassInfo, nullptr, &renderPass));

			std::vector<VkImageView> attachmentViews;
			for (auto attachment : attachments)
			{
				attachmentViews.push_back(attachment.view);
			}

			// Find. max number of layers across attachments
			uint32_t maxLayers = 0;
			for (auto attachment : attachments)
			{
				if (attachment.subresourceRange.layerCount > maxLayers)
				{
					maxLayers = attachment.subresourceRange.layerCount;
				}
			}

			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.pAttachments = attachmentViews.data();
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachmentViews.size());
			framebufferInfo.width = width;
			framebufferInfo.height = height;
			framebufferInfo.layers = maxLayers;
			VK_CHECK_RESULT(vkCreateFramebuffer(vulkanDevice->device, &framebufferInfo, nullptr, &framebuffer));

			return VK_SUCCESS;
		}
	};
}