/*
* Vulkan Example - Example for VK_EXT_debug_marker extension. To be used in conjuction with a debugging app like RenderDoc (https://renderdoc.org)
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
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

#define VERTEX_BUFFER_BIND_ID 0
#define ENABLE_VALIDATION false

// Offscreen properties
#define OFFSCREEN_DIM 256
#define OFFSCREEN_FORMAT VK_FORMAT_R8G8B8A8_UNORM
#define OFFSCREEN_FILTER VK_FILTER_LINEAR;

// Setup and functions for the VK_EXT_debug_marker_extension
// Extension spec can be found at https://github.com/KhronosGroup/Vulkan-Docs/blob/1.0-VK_EXT_debug_marker/doc/specs/vulkan/appendices/VK_EXT_debug_marker.txt
// Note that the extension will only be present if run from an offline debugging application
// The actual check for extension presence and enabling it on the device is done in the example base class
// See VulkanExampleBase::createInstance and VulkanExampleBase::createDevice (base/vulkanexamplebase.cpp)
namespace DebugMarker
{
	bool active = false;

	PFN_vkDebugMarkerSetObjectTagEXT pfnDebugMarkerSetObjectTag = VK_NULL_HANDLE;
	PFN_vkDebugMarkerSetObjectNameEXT pfnDebugMarkerSetObjectName = VK_NULL_HANDLE;
	PFN_vkCmdDebugMarkerBeginEXT pfnCmdDebugMarkerBegin = VK_NULL_HANDLE;
	PFN_vkCmdDebugMarkerEndEXT pfnCmdDebugMarkerEnd = VK_NULL_HANDLE;
	PFN_vkCmdDebugMarkerInsertEXT pfnCmdDebugMarkerInsert = VK_NULL_HANDLE;

	// Get function pointers for the debug report extensions from the device
	void setup(VkDevice device)
	{
		pfnDebugMarkerSetObjectTag = (PFN_vkDebugMarkerSetObjectTagEXT)vkGetDeviceProcAddr(device, "vkDebugMarkerSetObjectTagEXT");
		pfnDebugMarkerSetObjectName = (PFN_vkDebugMarkerSetObjectNameEXT)vkGetDeviceProcAddr(device, "vkDebugMarkerSetObjectNameEXT");
		pfnCmdDebugMarkerBegin = (PFN_vkCmdDebugMarkerBeginEXT)vkGetDeviceProcAddr(device, "vkCmdDebugMarkerBeginEXT");
		pfnCmdDebugMarkerEnd = (PFN_vkCmdDebugMarkerEndEXT)vkGetDeviceProcAddr(device, "vkCmdDebugMarkerEndEXT");
		pfnCmdDebugMarkerInsert = (PFN_vkCmdDebugMarkerInsertEXT)vkGetDeviceProcAddr(device, "vkCmdDebugMarkerInsertEXT");

		// Set flag if at least one function pointer is present
		active = pfnDebugMarkerSetObjectName;
	}

	// Sets the debug name of an object
	// All Objects in Vulkan are represented by their 64-bit handles which are passed into this function
	// along with the object type
	void setObjectName(VkDevice device, uint64_t object, VkDebugReportObjectTypeEXT objectType, const char *name)
	{
		// Check for valid function pointer (may not be present if not running in a debugging application)
		if (pfnDebugMarkerSetObjectName)
		{
			VkDebugMarkerObjectNameInfoEXT nameInfo = {};
			nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
			nameInfo.objectType = objectType;
			nameInfo.object = object;
			nameInfo.pObjectName = name;
			pfnDebugMarkerSetObjectName(device, &nameInfo);
		}
	}

	// Set the tag for an object
	void setObjectTag(VkDevice device, uint64_t object, VkDebugReportObjectTypeEXT objectType, uint64_t name, size_t tagSize, const void* tag)
	{
		// Check for valid function pointer (may not be present if not running in a debugging application)
		if (pfnDebugMarkerSetObjectTag)
		{
			VkDebugMarkerObjectTagInfoEXT tagInfo = {};
			tagInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_TAG_INFO_EXT;
			tagInfo.objectType = objectType;
			tagInfo.object = object;
			tagInfo.tagName = name;
			tagInfo.tagSize = tagSize;
			tagInfo.pTag = tag;
			pfnDebugMarkerSetObjectTag(device, &tagInfo);
		}
	}

	// Start a new debug marker region
	void beginRegion(VkCommandBuffer cmdbuffer, const char* pMarkerName, glm::vec4 color)
	{
		// Check for valid function pointer (may not be present if not running in a debugging application)
		if (pfnCmdDebugMarkerBegin)
		{
			VkDebugMarkerMarkerInfoEXT markerInfo = {};
			markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
			memcpy(markerInfo.color, &color[0], sizeof(float) * 4);
			markerInfo.pMarkerName = pMarkerName;
			pfnCmdDebugMarkerBegin(cmdbuffer, &markerInfo);
		}
	}

	// Insert a new debug marker into the command buffer
	void insert(VkCommandBuffer cmdbuffer, std::string markerName, glm::vec4 color)
	{
		// Check for valid function pointer (may not be present if not running in a debugging application)
		if (pfnCmdDebugMarkerInsert)
		{
			VkDebugMarkerMarkerInfoEXT markerInfo = {};
			markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
			memcpy(markerInfo.color, &color[0], sizeof(float) * 4);
			markerInfo.pMarkerName = markerName.c_str();
			pfnCmdDebugMarkerInsert(cmdbuffer, &markerInfo);
		}
	}

	// End the current debug marker region
	void endRegion(VkCommandBuffer cmdBuffer)
	{
		// Check for valid function (may not be present if not runnin in a debugging application)
		if (pfnCmdDebugMarkerEnd)
		{
			pfnCmdDebugMarkerEnd(cmdBuffer);
		}
	}
};

// Vertex layout used in this example
struct Vertex {
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec2 uv;
	glm::vec3 color;
};

struct Scene {
	struct {
		VkBuffer buf;
		VkDeviceMemory mem;
	} vertices;
	struct {
		VkBuffer buf;
		VkDeviceMemory mem;
	} indices;

	// Store mesh offsets for vertex and indexbuffers
	struct Mesh
	{
		uint32_t indexStart;
		uint32_t indexCount;
		std::string name;
	};
	std::vector<Mesh> meshes;

	void draw(VkCommandBuffer cmdBuffer)
	{
		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(cmdBuffer, VERTEX_BUFFER_BIND_ID, 1, &vertices.buf, offsets);
		vkCmdBindIndexBuffer(cmdBuffer, indices.buf, 0, VK_INDEX_TYPE_UINT32);
		for (auto mesh : meshes)
		{
			// Add debug marker for mesh name
			DebugMarker::insert(cmdBuffer, "Draw \"" + mesh.name + "\"", glm::vec4(0.0f));
			vkCmdDrawIndexed(cmdBuffer, mesh.indexCount, 1, mesh.indexStart, 0, 0);
		}
	}
};

class VulkanExample : public VulkanExampleBase
{
public:
	bool wireframe = true;
	bool glow = true;

	struct {
		VkPipelineVertexInputStateCreateInfo inputState;
		std::vector<VkVertexInputBindingDescription> bindingDescriptions;
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
	} vertices;

	Scene scene, sceneGlow;

	struct {
		vkTools::UniformData vsScene;
	} uniformData;

	struct {
		glm::mat4 projection;
		glm::mat4 model;
		glm::vec4 lightPos = glm::vec4(0.0f, 5.0f, 15.0f, 1.0f);
	} uboVS;

	struct {
		VkPipeline toonshading;
		VkPipeline color;
		VkPipeline wireframe;
		VkPipeline postprocess;
	} pipelines;

	VkPipelineLayout pipelineLayout;
	VkDescriptorSetLayout descriptorSetLayout;

	struct {
		VkDescriptorSet scene;
		VkDescriptorSet fullscreen;
	} descriptorSets;

	// Framebuffer for offscreen rendering
	struct FrameBufferAttachment {
		VkImage image;
		VkDeviceMemory mem;
		VkImageView view;
	};
	struct FrameBuffer {
		int32_t width, height;
		VkFramebuffer frameBuffer;
		FrameBufferAttachment color, depth;
		vkTools::VulkanTexture textureTarget;
	} offScreenFrameBuf;

	VkCommandBuffer offScreenCmdBuffer = VK_NULL_HANDLE;

	// Random tag data
	struct {
		const char name[17] = "debug marker tag";
	} demoTag;

	VulkanExample() : VulkanExampleBase(ENABLE_VALIDATION)
	{
		zoom = -8.5f;
		zoomSpeed = 2.5f;
		rotationSpeed = 0.5f;
		rotation = { -4.35f, 16.25f, 0.0f };
		cameraPos = { 0.1f, 1.1f, 0.0f };
		enableTextOverlay = true;
		title = "Vulkan Example - VK_EXT_debug_marker";
	}

	~VulkanExample()
	{
		// Clean up used Vulkan resources 
		// Note : Inherited destructor cleans up resources stored in base class
		vkDestroyPipeline(device, pipelines.toonshading, nullptr);
		vkDestroyPipeline(device, pipelines.color, nullptr);
		vkDestroyPipeline(device, pipelines.wireframe, nullptr);
		vkDestroyPipeline(device, pipelines.postprocess, nullptr);

		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

		// Destroy and free mesh resources 
		vkDestroyBuffer(device, scene.vertices.buf, nullptr);
		vkFreeMemory(device, scene.vertices.mem, nullptr);
		vkDestroyBuffer(device, scene.indices.buf, nullptr);
		vkFreeMemory(device, scene.indices.mem, nullptr);
		vkDestroyBuffer(device, sceneGlow.vertices.buf, nullptr);
		vkFreeMemory(device, sceneGlow.vertices.mem, nullptr);
		vkDestroyBuffer(device, sceneGlow.indices.buf, nullptr);
		vkFreeMemory(device, sceneGlow.indices.mem, nullptr);

		vkTools::destroyUniformData(device, &uniformData.vsScene);

		// Offscreen
		// Texture target
		textureLoader->destroyTexture(offScreenFrameBuf.textureTarget);
		// Frame buffer
		// Color attachment
		vkDestroyImageView(device, offScreenFrameBuf.color.view, nullptr);
		vkDestroyImage(device, offScreenFrameBuf.color.image, nullptr);
		vkFreeMemory(device, offScreenFrameBuf.color.mem, nullptr);
		// Depth attachment
		vkDestroyImageView(device, offScreenFrameBuf.depth.view, nullptr);
		vkDestroyImage(device, offScreenFrameBuf.depth.image, nullptr);
		vkFreeMemory(device, offScreenFrameBuf.depth.mem, nullptr);
		vkDestroyFramebuffer(device, offScreenFrameBuf.frameBuffer, nullptr);
	}

	// Prepare a texture target and framebuffer for offscreen rendering
	void prepareOffscreen()
	{
		VkCommandBuffer cmdBuffer = VulkanExampleBase::createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

		VkFormatProperties formatProperties;

		// Get device properites for the requested texture format
		vkGetPhysicalDeviceFormatProperties(physicalDevice, OFFSCREEN_FORMAT, &formatProperties);
		// Check if blit destination is supported for the requested format
		// Only try for optimal tiling, linear tiling usually won't support blit as destination anyway
		assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT);

		// Texture target

		vkTools::VulkanTexture *tex = &offScreenFrameBuf.textureTarget;

		// Prepare blit target texture
		tex->width = OFFSCREEN_DIM;
		tex->height = OFFSCREEN_DIM;

		VkImageCreateInfo imageCreateInfo = vkTools::initializers::imageCreateInfo();
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.format = OFFSCREEN_FORMAT;
		imageCreateInfo.extent = { OFFSCREEN_DIM, OFFSCREEN_DIM, 1 };
		imageCreateInfo.mipLevels = 1;
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
		// Texture will be sampled in a shader and is also the blit destination
		imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		VkMemoryAllocateInfo memAllocInfo = vkTools::initializers::memoryAllocateInfo();
		VkMemoryRequirements memReqs;

		VK_CHECK_RESULT(vkCreateImage(device, &imageCreateInfo, nullptr, &tex->image));
		vkGetImageMemoryRequirements(device, tex->image, &memReqs);
		memAllocInfo.allocationSize = memReqs.size;
		memAllocInfo.memoryTypeIndex = getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device, &memAllocInfo, nullptr, &(tex->deviceMemory)));
		VK_CHECK_RESULT(vkBindImageMemory(device, tex->image, tex->deviceMemory, 0));

		// Transform image layout to transfer destination
		tex->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		vkTools::setImageLayout(
			cmdBuffer,
			tex->image,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_PREINITIALIZED,
			tex->imageLayout);

		// Create sampler
		VkSamplerCreateInfo sampler = vkTools::initializers::samplerCreateInfo();
		sampler.magFilter = OFFSCREEN_FILTER;
		sampler.minFilter = OFFSCREEN_FILTER;
		sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler.addressModeV = sampler.addressModeU;
		sampler.addressModeW = sampler.addressModeU;
		sampler.mipLodBias = 0.0f;
		sampler.maxAnisotropy = 0;
		sampler.compareOp = VK_COMPARE_OP_NEVER;
		sampler.minLod = 0.0f;
		sampler.maxLod = 0.0f;
		sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		VK_CHECK_RESULT(vkCreateSampler(device, &sampler, nullptr, &tex->sampler));

		// Create image view
		VkImageViewCreateInfo view = vkTools::initializers::imageViewCreateInfo();
		view.image = VK_NULL_HANDLE;
		view.viewType = VK_IMAGE_VIEW_TYPE_2D;
		view.format = OFFSCREEN_FORMAT;
		view.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		view.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		view.image = tex->image;
		VK_CHECK_RESULT(vkCreateImageView(device, &view, nullptr, &tex->view));

		// Name for debugging
		DebugMarker::setObjectName(device, (uint64_t)tex->image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, "Off-screen texture target image");
		DebugMarker::setObjectName(device, (uint64_t)tex->sampler, VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT, "Off-screen texture target sampler");

		// Frame buffer
		offScreenFrameBuf.width = OFFSCREEN_DIM;
		offScreenFrameBuf.height = OFFSCREEN_DIM;

		// Find a suitable depth format
		VkFormat fbDepthFormat;
		VkBool32 validDepthFormat = vkTools::getSupportedDepthFormat(physicalDevice, &fbDepthFormat);
		assert(validDepthFormat);

		// Color attachment
		VkImageCreateInfo image = vkTools::initializers::imageCreateInfo();
		image.imageType = VK_IMAGE_TYPE_2D;
		image.format = OFFSCREEN_FORMAT;
		image.extent.width = offScreenFrameBuf.width;
		image.extent.height = offScreenFrameBuf.height;
		image.extent.depth = 1;
		image.mipLevels = 1;
		image.arrayLayers = 1;
		image.samples = VK_SAMPLE_COUNT_1_BIT;
		image.tiling = VK_IMAGE_TILING_OPTIMAL;
		// Image of the framebuffer is blit source
		image.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		image.flags = 0;

		VkImageViewCreateInfo colorImageView = vkTools::initializers::imageViewCreateInfo();
		colorImageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
		colorImageView.format = OFFSCREEN_FORMAT;
		colorImageView.flags = 0;
		colorImageView.subresourceRange = {};
		colorImageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		colorImageView.subresourceRange.baseMipLevel = 0;
		colorImageView.subresourceRange.levelCount = 1;
		colorImageView.subresourceRange.baseArrayLayer = 0;
		colorImageView.subresourceRange.layerCount = 1;

		VK_CHECK_RESULT(vkCreateImage(device, &image, nullptr, &offScreenFrameBuf.color.image));
		vkGetImageMemoryRequirements(device, offScreenFrameBuf.color.image, &memReqs);
		memAllocInfo.allocationSize = memReqs.size;
		memAllocInfo.memoryTypeIndex = getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device, &memAllocInfo, nullptr, &offScreenFrameBuf.color.mem));
		VK_CHECK_RESULT(vkBindImageMemory(device, offScreenFrameBuf.color.image, offScreenFrameBuf.color.mem, 0));

		vkTools::setImageLayout(
			cmdBuffer,
			offScreenFrameBuf.color.image,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		colorImageView.image = offScreenFrameBuf.color.image;
		VK_CHECK_RESULT(vkCreateImageView(device, &colorImageView, nullptr, &offScreenFrameBuf.color.view));

		// Depth stencil attachment
		image.format = fbDepthFormat;
		image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

		VkImageViewCreateInfo depthStencilView = vkTools::initializers::imageViewCreateInfo();
		depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
		depthStencilView.format = fbDepthFormat;
		depthStencilView.flags = 0;
		depthStencilView.subresourceRange = {};
		depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		depthStencilView.subresourceRange.baseMipLevel = 0;
		depthStencilView.subresourceRange.levelCount = 1;
		depthStencilView.subresourceRange.baseArrayLayer = 0;
		depthStencilView.subresourceRange.layerCount = 1;

		VK_CHECK_RESULT(vkCreateImage(device, &image, nullptr, &offScreenFrameBuf.depth.image));
		vkGetImageMemoryRequirements(device, offScreenFrameBuf.depth.image, &memReqs);
		memAllocInfo.allocationSize = memReqs.size;
		memAllocInfo.memoryTypeIndex = getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device, &memAllocInfo, nullptr, &offScreenFrameBuf.depth.mem));
		VK_CHECK_RESULT(vkBindImageMemory(device, offScreenFrameBuf.depth.image, offScreenFrameBuf.depth.mem, 0));

		vkTools::setImageLayout(
			cmdBuffer,
			offScreenFrameBuf.depth.image,
			VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

		depthStencilView.image = offScreenFrameBuf.depth.image;
		VK_CHECK_RESULT(vkCreateImageView(device, &depthStencilView, nullptr, &offScreenFrameBuf.depth.view));

		VkImageView attachments[2];
		attachments[0] = offScreenFrameBuf.color.view;
		attachments[1] = offScreenFrameBuf.depth.view;

		VkFramebufferCreateInfo fbufCreateInfo = vkTools::initializers::framebufferCreateInfo();
		fbufCreateInfo.renderPass = renderPass;
		fbufCreateInfo.attachmentCount = 2;
		fbufCreateInfo.pAttachments = attachments;
		fbufCreateInfo.width = offScreenFrameBuf.width;
		fbufCreateInfo.height = offScreenFrameBuf.height;
		fbufCreateInfo.layers = 1;
		VK_CHECK_RESULT(vkCreateFramebuffer(device, &fbufCreateInfo, nullptr, &offScreenFrameBuf.frameBuffer));

		VulkanExampleBase::flushCommandBuffer(cmdBuffer, queue, true);

		// Command buffer for offscreen rendering
		offScreenCmdBuffer = VulkanExampleBase::createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, false);

		// Name for debugging
		DebugMarker::setObjectName(device, (uint64_t)offScreenFrameBuf.color.image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, "Off-screen color framebuffer");
		DebugMarker::setObjectName(device, (uint64_t)offScreenFrameBuf.depth.image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, "Off-screen depth framebuffer");
	}

	// Command buffer for rendering color only scene for glow
	void buildOffscreenCommandBuffer()
	{
		VkCommandBufferBeginInfo cmdBufInfo = vkTools::initializers::commandBufferBeginInfo();

		VkClearValue clearValues[2];
		clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderPassBeginInfo = vkTools::initializers::renderPassBeginInfo();
		renderPassBeginInfo.renderPass = renderPass;
		renderPassBeginInfo.framebuffer = offScreenFrameBuf.frameBuffer;
		renderPassBeginInfo.renderArea.extent.width = offScreenFrameBuf.width;
		renderPassBeginInfo.renderArea.extent.height = offScreenFrameBuf.height;
		renderPassBeginInfo.clearValueCount = 2;
		renderPassBeginInfo.pClearValues = clearValues;

		VK_CHECK_RESULT(vkBeginCommandBuffer(offScreenCmdBuffer, &cmdBufInfo));

		// Start a new debug marker region
		DebugMarker::beginRegion(offScreenCmdBuffer, "Off-screen scene rendering", glm::vec4(1.0f, 0.0f, 0.0f, 0.0f));

		VkViewport viewport = vkTools::initializers::viewport((float)offScreenFrameBuf.width, (float)offScreenFrameBuf.height, 0.0f, 1.0f);
		vkCmdSetViewport(offScreenCmdBuffer, 0, 1, &viewport);

		VkRect2D scissor = vkTools::initializers::rect2D(offScreenFrameBuf.width, offScreenFrameBuf.height, 0, 0);
		vkCmdSetScissor(offScreenCmdBuffer, 0, 1, &scissor);

		vkCmdBeginRenderPass(offScreenCmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindDescriptorSets(offScreenCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets.scene, 0, NULL);
		vkCmdBindPipeline(offScreenCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.color);

		// Draw glow scene
		sceneGlow.draw(offScreenCmdBuffer);

		vkCmdEndRenderPass(offScreenCmdBuffer);

		// Make sure color writes to the framebuffer are finished before using it as transfer source
		vkTools::setImageLayout(
			offScreenCmdBuffer,
			offScreenFrameBuf.color.image,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

		// Transform texture target to transfer destination
		vkTools::setImageLayout(
			offScreenCmdBuffer,
			offScreenFrameBuf.textureTarget.image,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		// Blit offscreen color buffer to our texture target
		VkImageBlit imgBlit;

		imgBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imgBlit.srcSubresource.mipLevel = 0;
		imgBlit.srcSubresource.baseArrayLayer = 0;
		imgBlit.srcSubresource.layerCount = 1;

		imgBlit.srcOffsets[0] = { 0, 0, 0 };
		imgBlit.srcOffsets[1].x = offScreenFrameBuf.width;
		imgBlit.srcOffsets[1].y = offScreenFrameBuf.height;
		imgBlit.srcOffsets[1].z = 1;

		imgBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imgBlit.dstSubresource.mipLevel = 0;
		imgBlit.dstSubresource.baseArrayLayer = 0;
		imgBlit.dstSubresource.layerCount = 1;

		imgBlit.dstOffsets[0] = { 0, 0, 0 };
		imgBlit.dstOffsets[1].x = offScreenFrameBuf.textureTarget.width;
		imgBlit.dstOffsets[1].y = offScreenFrameBuf.textureTarget.height;
		imgBlit.dstOffsets[1].z = 1;

		// Blit from framebuffer image to texture image
		// vkCmdBlitImage does scaling and (if necessary and possible) also does format conversions
		vkCmdBlitImage(
			offScreenCmdBuffer,
			offScreenFrameBuf.color.image,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			offScreenFrameBuf.textureTarget.image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&imgBlit,
			VK_FILTER_LINEAR
			);

		// Transform framebuffer color attachment back 
		vkTools::setImageLayout(
			offScreenCmdBuffer,
			offScreenFrameBuf.color.image,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		// Transform texture target back to shader read
		// Makes sure that writes to the texture are finished before
		// it's accessed in the shader
		vkTools::setImageLayout(
			offScreenCmdBuffer,
			offScreenFrameBuf.textureTarget.image,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		DebugMarker::endRegion(offScreenCmdBuffer);

		VK_CHECK_RESULT(vkEndCommandBuffer(offScreenCmdBuffer));
	}

	// Load a model file as separate meshes into a scene
	void loadModel(std::string filename, Scene *scene)
	{
		VulkanMeshLoader *meshLoader = new VulkanMeshLoader();
#if defined(__ANDROID__)
		meshLoader->assetManager = androidApp->activity->assetManager;
#endif
		meshLoader->LoadMesh(filename);

		scene->meshes.resize(meshLoader->m_Entries.size());

		// Generate vertex buffer
		float scale = 1.0f;
		std::vector<Vertex> vertexBuffer;
		// Iterate through all meshes in the file
		// and extract the vertex information used in this demo
		for (uint32_t m = 0; m < meshLoader->m_Entries.size(); m++)
		{
			for (uint32_t i = 0; i < meshLoader->m_Entries[m].Vertices.size(); i++)
			{
				Vertex vertex;

				vertex.pos = meshLoader->m_Entries[m].Vertices[i].m_pos * scale;
				vertex.normal = meshLoader->m_Entries[m].Vertices[i].m_normal;
				vertex.uv = meshLoader->m_Entries[m].Vertices[i].m_tex;
				vertex.color = meshLoader->m_Entries[m].Vertices[i].m_color;

				vertexBuffer.push_back(vertex);
			}
		}
		uint32_t vertexBufferSize = vertexBuffer.size() * sizeof(Vertex);

		// Generate index buffer from loaded mesh file
		std::vector<uint32_t> indexBuffer;
		for (uint32_t m = 0; m < meshLoader->m_Entries.size(); m++)
		{
			uint32_t indexBase = indexBuffer.size();
			for (uint32_t i = 0; i < meshLoader->m_Entries[m].Indices.size(); i++)
			{
				indexBuffer.push_back(meshLoader->m_Entries[m].Indices[i] + indexBase);
			}
			scene->meshes[m].indexStart = indexBase;
			scene->meshes[m].indexCount = meshLoader->m_Entries[m].Indices.size();
		}
		uint32_t indexBufferSize = indexBuffer.size() * sizeof(uint32_t);

		// Static mesh should always be device local

		bool useStaging = true;

		if (useStaging)
		{
			struct {
				VkBuffer buffer;
				VkDeviceMemory memory;
			} vertexStaging, indexStaging;

			// Create staging buffers
			// Vertex data
			createBuffer(
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
				vertexBufferSize,
				vertexBuffer.data(),
				&vertexStaging.buffer,
				&vertexStaging.memory);
			// Index data
			createBuffer(
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
				indexBufferSize,
				indexBuffer.data(),
				&indexStaging.buffer,
				&indexStaging.memory);

			// Create device local buffers
			// Vertex buffer
			createBuffer(
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				vertexBufferSize,
				nullptr,
				&scene->vertices.buf,
				&scene->vertices.mem);
			// Index buffer
			createBuffer(
				VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				indexBufferSize,
				nullptr,
				&scene->indices.buf,
				&scene->indices.mem);

			// Copy from staging buffers
			VkCommandBuffer copyCmd = VulkanExampleBase::createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

			VkBufferCopy copyRegion = {};

			copyRegion.size = vertexBufferSize;
			vkCmdCopyBuffer(
				copyCmd,
				vertexStaging.buffer,
				scene->vertices.buf,
				1,
				&copyRegion);

			copyRegion.size = indexBufferSize;
			vkCmdCopyBuffer(
				copyCmd,
				indexStaging.buffer,
				scene->indices.buf,
				1,
				&copyRegion);

			VulkanExampleBase::flushCommandBuffer(copyCmd, queue, true);

			vkDestroyBuffer(device, vertexStaging.buffer, nullptr);
			vkFreeMemory(device, vertexStaging.memory, nullptr);
			vkDestroyBuffer(device, indexStaging.buffer, nullptr);
			vkFreeMemory(device, indexStaging.memory, nullptr);
		}
		else
		{
			// Vertex buffer
			createBuffer(
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
				vertexBufferSize,
				vertexBuffer.data(),
				&scene->vertices.buf,
				&scene->vertices.mem);
			// Index buffer
			createBuffer(
				VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
				indexBufferSize,
				indexBuffer.data(),
				&scene->indices.buf,
				&scene->indices.mem);
		}

		delete(meshLoader);
	}

	void loadScene()
	{
		loadModel(getAssetPath() + "models/treasure_smooth.dae", &scene);
		loadModel(getAssetPath() + "models/treasure_glow.dae", &sceneGlow);

		// Name the meshes
		// ASSIMP does not load mesh names from the COLLADA file used in this example
		// so we need to set them manually
		// These names are used in command buffer creation for setting debug markers
		// Scene
		std::vector<std::string> names = { "hill", "rocks", "cave", "tree", "mushroom stems", "blue mushroom caps", "red mushroom caps", "grass blades", "chest box", "chest fittings" };
		for (size_t i = 0; i < names.size(); i++)
		{
			scene.meshes[i].name = names[i];
			sceneGlow.meshes[i].name = names[i];
		}

		// Name the buffers for debugging
		// Scene
		DebugMarker::setObjectName(device, (uint64_t)scene.vertices.buf, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, "Scene vertex buffer");
		DebugMarker::setObjectName(device, (uint64_t)scene.indices.buf, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, "Scene index buffer");
		// Glow
		DebugMarker::setObjectName(device, (uint64_t)sceneGlow.vertices.buf, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, "Glow vertex buffer");
		DebugMarker::setObjectName(device, (uint64_t)sceneGlow.indices.buf, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, "Glow index buffer");
	}

	void reBuildCommandBuffers()
	{
		if (!checkCommandBuffers())
		{
			destroyCommandBuffers();
			createCommandBuffers();
		}
		buildCommandBuffers();
	}

	void buildCommandBuffers()
	{
		VkCommandBufferBeginInfo cmdBufInfo = vkTools::initializers::commandBufferBeginInfo();

		VkClearValue clearValues[2];
		clearValues[0].color = defaultClearColor;
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderPassBeginInfo = vkTools::initializers::renderPassBeginInfo();
		renderPassBeginInfo.renderPass = renderPass;
		renderPassBeginInfo.renderArea.offset.x = 0;
		renderPassBeginInfo.renderArea.offset.y = 0;
		renderPassBeginInfo.renderArea.extent.width = width;
		renderPassBeginInfo.renderArea.extent.height = height;
		renderPassBeginInfo.clearValueCount = 2;
		renderPassBeginInfo.pClearValues = clearValues;

		for (int32_t i = 0; i < drawCmdBuffers.size(); ++i)
		{
			// Set target frame buffer
			renderPassBeginInfo.framebuffer = frameBuffers[i];

			VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo));

			// Start a new debug marker region
			DebugMarker::beginRegion(drawCmdBuffers[i], "Render scene", glm::vec4(1.0f, 0.0f, 0.0f, 0.0f));

			vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport viewport = vkTools::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
			vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

			VkRect2D scissor = vkTools::initializers::rect2D(wireframe ? width / 2 : width, height, 0, 0);
			vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

			vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets.scene, 0, NULL);

			// Solid rendering

			// Start a new debug marker region
			DebugMarker::beginRegion(drawCmdBuffers[i], "Toon shading draw", glm::vec4(1.0f, 0.0f, 0.0f, 0.0f));

			vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.toonshading);
			scene.draw(drawCmdBuffers[i]);

			DebugMarker::endRegion(drawCmdBuffers[i]);

			// Wireframe rendering
			if (wireframe)
			{
				// Insert debug marker
				DebugMarker::beginRegion(drawCmdBuffers[i], "Wireframe draw", glm::vec4(1.0f, 0.0f, 0.0f, 0.0f));

				scissor.offset.x = width / 2;
				vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

				vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.wireframe);
				scene.draw(drawCmdBuffers[i]);

				DebugMarker::endRegion(drawCmdBuffers[i]);

				scissor.offset.x = 0;
				scissor.extent.width = width;
				vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);
			}

			// Post processing
			if (glow)
			{
				DebugMarker::beginRegion(drawCmdBuffers[i], "Apply post processing", glm::vec4(1.0f, 0.0f, 0.0f, 0.0f));

				vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.postprocess);
				// Full screen quad is generated by the vertex shaders, so we reuse four vertices (for four invocations) from current vertex buffer
				vkCmdDraw(drawCmdBuffers[i], 4, 1, 0, 0);

				DebugMarker::endRegion(drawCmdBuffers[i]);
			}


			vkCmdEndRenderPass(drawCmdBuffers[i]);

			// End current debug marker region
			DebugMarker::endRegion(drawCmdBuffers[i]);

			VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
		}
	}
	
	void setupVertexDescriptions()
	{
		// Binding description
		vertices.bindingDescriptions.resize(1);
		vertices.bindingDescriptions[0] =
			vkTools::initializers::vertexInputBindingDescription(
				VERTEX_BUFFER_BIND_ID,
				sizeof(Vertex),
				VK_VERTEX_INPUT_RATE_VERTEX);

		// Attribute descriptions
		// Describes memory layout and shader positions
		vertices.attributeDescriptions.resize(4);
		// Location 0 : Position
		vertices.attributeDescriptions[0] =
			vkTools::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				0,
				VK_FORMAT_R32G32B32_SFLOAT,
				0);
		// Location 1 : Normal
		vertices.attributeDescriptions[1] =
			vkTools::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				1,
				VK_FORMAT_R32G32B32_SFLOAT,
				sizeof(float) * 3);
		// Location 2 : Texture coordinates
		vertices.attributeDescriptions[2] =
			vkTools::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				2,
				VK_FORMAT_R32G32_SFLOAT,
				sizeof(float) * 6);
		// Location 3 : Color
		vertices.attributeDescriptions[3] =
			vkTools::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				3,
				VK_FORMAT_R32G32B32_SFLOAT,
				sizeof(float) * 8);

		vertices.inputState = vkTools::initializers::pipelineVertexInputStateCreateInfo();
		vertices.inputState.vertexBindingDescriptionCount = vertices.bindingDescriptions.size();
		vertices.inputState.pVertexBindingDescriptions = vertices.bindingDescriptions.data();
		vertices.inputState.vertexAttributeDescriptionCount = vertices.attributeDescriptions.size();
		vertices.inputState.pVertexAttributeDescriptions = vertices.attributeDescriptions.data();
	}

	void setupDescriptorPool()
	{
		// Example uses one ubo and one combined image sampler
		std::vector<VkDescriptorPoolSize> poolSizes =
		{
			vkTools::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1),
			vkTools::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1),
		};

		VkDescriptorPoolCreateInfo descriptorPoolInfo =
			vkTools::initializers::descriptorPoolCreateInfo(
				poolSizes.size(),
				poolSizes.data(),
				1);

		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));
	}

	void setupDescriptorSetLayout()
	{
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings =
		{
			// Binding 0 : Vertex shader uniform buffer
			vkTools::initializers::descriptorSetLayoutBinding(
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				VK_SHADER_STAGE_VERTEX_BIT,
				0),
			// Binding 1 : Fragment shader combined sampler
			vkTools::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_SHADER_STAGE_FRAGMENT_BIT,
				1),
		};

		VkDescriptorSetLayoutCreateInfo descriptorLayout =
			vkTools::initializers::descriptorSetLayoutCreateInfo(
				setLayoutBindings.data(),
				setLayoutBindings.size());

		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayout));

		VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo =
			vkTools::initializers::pipelineLayoutCreateInfo(
				&descriptorSetLayout,
				1);

		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &pipelineLayout));

		// Name for debugging
		DebugMarker::setObjectName(device, (uint64_t)pipelineLayout, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT, "Shared pipeline layout");
		DebugMarker::setObjectName(device, (uint64_t)descriptorSetLayout, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT, "Shared descriptor set layout");
	}

	void setupDescriptorSet()
	{
		VkDescriptorSetAllocateInfo allocInfo =
			vkTools::initializers::descriptorSetAllocateInfo(
				descriptorPool,
				&descriptorSetLayout,
				1);

		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.scene));

		VkDescriptorImageInfo texDescriptor =
			vkTools::initializers::descriptorImageInfo(
				offScreenFrameBuf.textureTarget.sampler,
				offScreenFrameBuf.textureTarget.view,
				VK_IMAGE_LAYOUT_GENERAL);

		std::vector<VkWriteDescriptorSet> writeDescriptorSets =
		{
			// Binding 0 : Vertex shader uniform buffer
			vkTools::initializers::writeDescriptorSet(
				descriptorSets.scene,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				0,
				&uniformData.vsScene.descriptor),
			// Binding 1 : Color map 
			vkTools::initializers::writeDescriptorSet(
				descriptorSets.scene,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				1,
				&texDescriptor)
		};

		vkUpdateDescriptorSets(device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);
	}

	void preparePipelines()
	{
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
			vkTools::initializers::pipelineInputAssemblyStateCreateInfo(
				VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
				0,
				VK_FALSE);

		VkPipelineRasterizationStateCreateInfo rasterizationState =
			vkTools::initializers::pipelineRasterizationStateCreateInfo(
				VK_POLYGON_MODE_FILL,
				VK_CULL_MODE_BACK_BIT,
				VK_FRONT_FACE_CLOCKWISE,
				0);

		VkPipelineColorBlendAttachmentState blendAttachmentState =
			vkTools::initializers::pipelineColorBlendAttachmentState(
				0xf,
				VK_FALSE);

		VkPipelineColorBlendStateCreateInfo colorBlendState =
			vkTools::initializers::pipelineColorBlendStateCreateInfo(
				1,
				&blendAttachmentState);

		VkPipelineDepthStencilStateCreateInfo depthStencilState =
			vkTools::initializers::pipelineDepthStencilStateCreateInfo(
				VK_TRUE,
				VK_TRUE,
				VK_COMPARE_OP_LESS_OR_EQUAL);

		VkPipelineViewportStateCreateInfo viewportState =
			vkTools::initializers::pipelineViewportStateCreateInfo(1, 1, 0);

		VkPipelineMultisampleStateCreateInfo multisampleState =
			vkTools::initializers::pipelineMultisampleStateCreateInfo(
				VK_SAMPLE_COUNT_1_BIT,
				0);

		std::vector<VkDynamicState> dynamicStateEnables = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};
		VkPipelineDynamicStateCreateInfo dynamicState =
			vkTools::initializers::pipelineDynamicStateCreateInfo(
				dynamicStateEnables.data(),
				dynamicStateEnables.size(),
				0);

		// Phong lighting pipeline
		// Load shaders
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

		shaderStages[0] = loadShader(getAssetPath() + "shaders/debugmarker/toon.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getAssetPath() + "shaders/debugmarker/toon.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

		VkGraphicsPipelineCreateInfo pipelineCreateInfo =
			vkTools::initializers::pipelineCreateInfo(
				pipelineLayout,
				renderPass,
				0);

		pipelineCreateInfo.pVertexInputState = &vertices.inputState;
		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
		pipelineCreateInfo.pRasterizationState = &rasterizationState;
		pipelineCreateInfo.pColorBlendState = &colorBlendState;
		pipelineCreateInfo.pMultisampleState = &multisampleState;
		pipelineCreateInfo.pViewportState = &viewportState;
		pipelineCreateInfo.pDepthStencilState = &depthStencilState;
		pipelineCreateInfo.pDynamicState = &dynamicState;
		pipelineCreateInfo.stageCount = shaderStages.size();
		pipelineCreateInfo.pStages = shaderStages.data();

		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.toonshading));

		// Color only pipeline
		shaderStages[0] = loadShader(getAssetPath() + "shaders/debugmarker/colorpass.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getAssetPath() + "shaders/debugmarker/colorpass.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.color));

		// Wire frame rendering pipeline
		rasterizationState.polygonMode = VK_POLYGON_MODE_LINE;
		rasterizationState.lineWidth = 1.0f;

		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.wireframe));

		// Post processing effect
		shaderStages[0] = loadShader(getAssetPath() + "shaders/debugmarker/postprocess.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getAssetPath() + "shaders/debugmarker/postprocess.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

		depthStencilState.depthTestEnable = VK_FALSE;
		depthStencilState.depthWriteEnable = VK_FALSE;

		rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationState.cullMode = VK_CULL_MODE_NONE;

		blendAttachmentState.colorWriteMask = 0xF;
		blendAttachmentState.blendEnable =  VK_TRUE;
		blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
		blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
		blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
		blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;

		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.postprocess));

		// Name shader moduels for debugging
		DebugMarker::setObjectName(device, (uint64_t)shaderModules[0], VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT, "Toon shading vertex shader");
		DebugMarker::setObjectName(device, (uint64_t)shaderModules[1], VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT, "Toon shading fragment shader");
		DebugMarker::setObjectName(device, (uint64_t)shaderModules[2], VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT, "Color-only vertex shader");
		DebugMarker::setObjectName(device, (uint64_t)shaderModules[3], VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT, "Color-only fragment shader");
		DebugMarker::setObjectName(device, (uint64_t)shaderModules[4], VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT, "Postprocess vertex shader");
		DebugMarker::setObjectName(device, (uint64_t)shaderModules[5], VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT, "Postprocess fragment shader");

		// Name pipelines for debugging
		DebugMarker::setObjectName(device, (uint64_t)pipelines.toonshading, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT, "Toon shading pipeline");
		DebugMarker::setObjectName(device, (uint64_t)pipelines.color, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT, "Color only pipeline");
		DebugMarker::setObjectName(device, (uint64_t)pipelines.wireframe, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT, "Wireframe rendering pipeline");
		DebugMarker::setObjectName(device, (uint64_t)pipelines.postprocess, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT, "Post processing pipeline");
	}

	// Prepare and initialize uniform buffer containing shader uniforms
	void prepareUniformBuffers()
	{
		// Vertex shader uniform buffer block
		createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			sizeof(uboVS),
			&uboVS,
			&uniformData.vsScene.buffer,
			&uniformData.vsScene.memory,
			&uniformData.vsScene.descriptor);

		// Name uniform buffer for debugging
		DebugMarker::setObjectName(device, (uint64_t)uniformData.vsScene.buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, "Scene uniform buffer block");
		// Add some random tag
		DebugMarker::setObjectTag(device, (uint64_t)uniformData.vsScene.buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, 0, sizeof(demoTag), &demoTag);

		updateUniformBuffers();
	}

	void updateUniformBuffers()
	{
		uboVS.projection = glm::perspective(glm::radians(60.0f), (float)width / (float)height, 0.1f, 256.0f);
		glm::mat4 viewMatrix = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, zoom));

		uboVS.model = viewMatrix * glm::translate(glm::mat4(), cameraPos);
		uboVS.model = glm::rotate(uboVS.model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		uboVS.model = glm::rotate(uboVS.model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		uboVS.model = glm::rotate(uboVS.model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

		uint8_t *pData;
		VK_CHECK_RESULT(vkMapMemory(device, uniformData.vsScene.memory, 0, sizeof(uboVS), 0, (void **)&pData));
		memcpy(pData, &uboVS, sizeof(uboVS));
		vkUnmapMemory(device, uniformData.vsScene.memory);
	}

	void draw()
	{
		VulkanExampleBase::prepareFrame();

		std::vector<VkCommandBuffer> submitCmdBuffers;

		// Submit offscreen rendering command buffer 
		// todo : use event to ensure that offscreen result is finished bfore render command buffer is started
		if (glow)
		{
			submitCmdBuffers.push_back(offScreenCmdBuffer);
		}
		submitCmdBuffers.push_back(drawCmdBuffers[currentBuffer]);

		submitInfo.commandBufferCount = submitCmdBuffers.size();
		submitInfo.pCommandBuffers = submitCmdBuffers.data();
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

		VulkanExampleBase::submitFrame();
	}

	void prepare()
	{
		VulkanExampleBase::prepare();
		DebugMarker::setup(device);
		loadScene();
		prepareOffscreen();
		setupVertexDescriptions();
		prepareUniformBuffers();
		setupDescriptorSetLayout();
		preparePipelines();
		setupDescriptorPool();
		setupDescriptorSet();
		buildCommandBuffers();
		buildOffscreenCommandBuffer();
		updateTextOverlay();
		prepared = true;
	}

	virtual void render()
	{
		if (!prepared)
			return;
		draw();
	}

	virtual void viewChanged()
	{
		updateUniformBuffers();
	}

	virtual void keyPressed(uint32_t keyCode)
	{
		switch (keyCode)
		{
		case 0x57:
		case GAMEPAD_BUTTON_X:
			wireframe = !wireframe;
			reBuildCommandBuffers();
			break;
		case 0x47:
		case GAMEPAD_BUTTON_A:
			glow = !glow;
			reBuildCommandBuffers();
			break;
		}
	}

	virtual void getOverlayText(VulkanTextOverlay *textOverlay)
	{
		if (DebugMarker::active)
		{
			textOverlay->addText("VK_EXT_debug_marker active", 5.0f, 85.0f, VulkanTextOverlay::alignLeft);
		}
		else
		{
			textOverlay->addText("VK_EXT_debug_marker not present", 5.0f, 85.0f, VulkanTextOverlay::alignLeft);
		}
	}
};

VulkanExample *vulkanExample;

#if defined(_WIN32)
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (vulkanExample != NULL)
	{
		vulkanExample->handleMessages(hWnd, uMsg, wParam, lParam);
	}
	return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}
#elif defined(__linux__) && !defined(__ANDROID__)
static void handleEvent(const xcb_generic_event_t *event)
{
	if (vulkanExample != NULL)
	{
		vulkanExample->handleEvent(event);
}
		}
#endif

// Main entry point
#if defined(_WIN32)
// Windows entry point
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
#elif defined(__ANDROID__)
// Android entry point
void android_main(android_app* state)
#elif defined(__linux__)
// Linux entry point
int main(const int argc, const char *argv[])
#endif
{
#if defined(__ANDROID__)
	// Removing this may cause the compiler to omit the main entry point 
	// which would make the application crash at start
	app_dummy();
#endif
	vulkanExample = new VulkanExample();
#if defined(_WIN32)
	vulkanExample->setupWindow(hInstance, WndProc);
#elif defined(__ANDROID__)
	// Attach vulkan example to global android application state
	state->userData = vulkanExample;
	state->onAppCmd = VulkanExample::handleAppCommand;
	state->onInputEvent = VulkanExample::handleAppInput;
	vulkanExample->androidApp = state;
#elif defined(__linux__)
	vulkanExample->setupWindow();
#endif
#if !defined(__ANDROID__)
	vulkanExample->initSwapchain();
	vulkanExample->prepare();
#endif
	vulkanExample->renderLoop();
	delete(vulkanExample);
#if !defined(__ANDROID__)
	return 0;
#endif
}