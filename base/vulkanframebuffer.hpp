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
	};

	/**
	* @brief Describes the attributes of an attachment to be created
	*/
	struct AttachmentCreateInfo
	{
		uint32_t width, height;
		uint32_t layerCount;
		VkFormat format;
		// todo: could be derived from format
		VkImageUsageFlags usage;
		// todo: rename
		bool sample;
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
		* Destroy and free resources used for the framebuffer and all of it's attachments
		*/
		void FreeResources(VkDevice device)
		{
			for (auto attachment : attachments)
			{
				vkDestroyImage(device, attachment.image, nullptr);
				vkDestroyImageView(device, attachment.view, nullptr);
				vkFreeMemory(device, attachment.memory, nullptr);
			}
			vkDestroySampler(device, sampler, nullptr);
			vkDestroyRenderPass(device, renderPass, nullptr);
			vkDestroyFramebuffer(device, framebuffer, nullptr);
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
			if (createinfo.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
			{
				aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageLayout = createinfo.sample ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			}

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
				imageLayout = createinfo.sample ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
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

			attachments.push_back(attachment);

			return static_cast<uint32_t>(attachments.size() - 1);
		}
	};
}