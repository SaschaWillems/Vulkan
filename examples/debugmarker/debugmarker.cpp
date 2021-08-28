/*
* Vulkan Example - Example for VK_EXT_debug_marker extension. To be used in conjunction with a debugging app like RenderDoc (https://renderdoc.org)
*
* Copyright (C) by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

/*
 * Note: This sample is deprecated!
 * An updated version using VK_EXT_debug_utils along with an in-depth tutorial is available in the official Khronos Vulkan Samples repository at
 * https://github.com/KhronosGroup/Vulkan-Samples/blob/master/samples/extensions/debug_utils.
 */

#include "vulkanexamplebase.h"
#include "VulkanglTFModel.h"

#define ENABLE_VALIDATION false

// Offscreen properties
#define OFFSCREEN_DIM 256
#define OFFSCREEN_FORMAT VK_FORMAT_R8G8B8A8_UNORM
#define OFFSCREEN_FILTER VK_FILTER_LINEAR;

// Setup and functions for the VK_EXT_debug_marker_extension
// Extension spec can be found at https://github.com/KhronosGroup/Vulkan-Docs/blob/1.0-VK_EXT_debug_marker/doc/specs/vulkan/appendices/VK_EXT_debug_marker.txt
// Note that the extension will only be present if run from an offline debugging application
namespace DebugMarker
{
	bool active = false;
	bool extensionPresent = false;

	PFN_vkDebugMarkerSetObjectTagEXT vkDebugMarkerSetObjectTag = VK_NULL_HANDLE;
	PFN_vkDebugMarkerSetObjectNameEXT vkDebugMarkerSetObjectName = VK_NULL_HANDLE;
	PFN_vkCmdDebugMarkerBeginEXT vkCmdDebugMarkerBegin = VK_NULL_HANDLE;
	PFN_vkCmdDebugMarkerEndEXT vkCmdDebugMarkerEnd = VK_NULL_HANDLE;
	PFN_vkCmdDebugMarkerInsertEXT vkCmdDebugMarkerInsert = VK_NULL_HANDLE;

	// Get function pointers for the debug report extensions from the device
	void setup(VkDevice device, VkPhysicalDevice physicalDevice)
	{
		// Check if the debug marker extension is present (which is the case if run from a graphics debugger)
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> extensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, extensions.data());
		for (auto extension : extensions) {
			if (strcmp(extension.extensionName, VK_EXT_DEBUG_MARKER_EXTENSION_NAME) == 0) {
				extensionPresent = true;
				break;
			}
		}

		if (extensionPresent) {
			// The debug marker extension is not part of the core, so function pointers need to be loaded manually
			vkDebugMarkerSetObjectTag = (PFN_vkDebugMarkerSetObjectTagEXT)vkGetDeviceProcAddr(device, "vkDebugMarkerSetObjectTagEXT");
			vkDebugMarkerSetObjectName = (PFN_vkDebugMarkerSetObjectNameEXT)vkGetDeviceProcAddr(device, "vkDebugMarkerSetObjectNameEXT");
			vkCmdDebugMarkerBegin = (PFN_vkCmdDebugMarkerBeginEXT)vkGetDeviceProcAddr(device, "vkCmdDebugMarkerBeginEXT");
			vkCmdDebugMarkerEnd = (PFN_vkCmdDebugMarkerEndEXT)vkGetDeviceProcAddr(device, "vkCmdDebugMarkerEndEXT");
			vkCmdDebugMarkerInsert = (PFN_vkCmdDebugMarkerInsertEXT)vkGetDeviceProcAddr(device, "vkCmdDebugMarkerInsertEXT");
			// Set flag if at least one function pointer is present
			active = (vkDebugMarkerSetObjectName != VK_NULL_HANDLE);
		}
		else {
			std::cout << "Warning: " << VK_EXT_DEBUG_MARKER_EXTENSION_NAME << " not present, debug markers are disabled.";
			std::cout << "Try running from inside a Vulkan graphics debugger (e.g. RenderDoc)" << std::endl;
		}
	}

	// Sets the debug name of an object
	// All Objects in Vulkan are represented by their 64-bit handles which are passed into this function
	// along with the object type
	void setObjectName(VkDevice device, uint64_t object, VkDebugReportObjectTypeEXT objectType, const char *name)
	{
		// Check for valid function pointer (may not be present if not running in a debugging application)
		if (active)
		{
			VkDebugMarkerObjectNameInfoEXT nameInfo = {};
			nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
			nameInfo.objectType = objectType;
			nameInfo.object = object;
			nameInfo.pObjectName = name;
			vkDebugMarkerSetObjectName(device, &nameInfo);
		}
	}

	// Set the tag for an object
	void setObjectTag(VkDevice device, uint64_t object, VkDebugReportObjectTypeEXT objectType, uint64_t name, size_t tagSize, const void* tag)
	{
		// Check for valid function pointer (may not be present if not running in a debugging application)
		if (active)
		{
			VkDebugMarkerObjectTagInfoEXT tagInfo = {};
			tagInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_TAG_INFO_EXT;
			tagInfo.objectType = objectType;
			tagInfo.object = object;
			tagInfo.tagName = name;
			tagInfo.tagSize = tagSize;
			tagInfo.pTag = tag;
			vkDebugMarkerSetObjectTag(device, &tagInfo);
		}
	}

	// Start a new debug marker region
	void beginRegion(VkCommandBuffer cmdbuffer, const char* pMarkerName, glm::vec4 color)
	{
		// Check for valid function pointer (may not be present if not running in a debugging application)
		if (active)
		{
			VkDebugMarkerMarkerInfoEXT markerInfo = {};
			markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
			memcpy(markerInfo.color, &color[0], sizeof(float) * 4);
			markerInfo.pMarkerName = pMarkerName;
			vkCmdDebugMarkerBegin(cmdbuffer, &markerInfo);
		}
	}

	// Insert a new debug marker into the command buffer
	void insert(VkCommandBuffer cmdbuffer, std::string markerName, glm::vec4 color)
	{
		// Check for valid function pointer (may not be present if not running in a debugging application)
		if (active)
		{
			VkDebugMarkerMarkerInfoEXT markerInfo = {};
			markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
			memcpy(markerInfo.color, &color[0], sizeof(float) * 4);
			markerInfo.pMarkerName = markerName.c_str();
			vkCmdDebugMarkerInsert(cmdbuffer, &markerInfo);
		}
	}

	// End the current debug marker region
	void endRegion(VkCommandBuffer cmdBuffer)
	{
		// Check for valid function (may not be present if not running in a debugging application)
		if (vkCmdDebugMarkerEnd)
		{
			vkCmdDebugMarkerEnd(cmdBuffer);
		}
	}
};

struct Scene {

	vkglTF::Model model;
	std::vector<std::string> modelPartNames;

	void draw(VkCommandBuffer cmdBuffer)
	{
		VkDeviceSize offsets[1] = { 0 };
		model.bindBuffers(cmdBuffer);
		for (auto i = 0; i < model.nodes.size(); i++)
		{
			// Add debug marker the name of this glTF node
			DebugMarker::insert(cmdBuffer, "Draw \"" + model.nodes[i]->name + "\"", glm::vec4(0.0f));
			model.drawNode(model.nodes[i], cmdBuffer);
		}
	}

	void loadFromFile(std::string filename, vks::VulkanDevice* vulkanDevice, VkQueue queue)
	{
		const uint32_t glTFLoadingFlags = vkglTF::FileLoadingFlags::PreTransformVertices | vkglTF::FileLoadingFlags::PreMultiplyVertexColors | vkglTF::FileLoadingFlags::FlipY;
		model.loadFromFile(filename, vulkanDevice, queue, glTFLoadingFlags);
	}
};

class VulkanExample : public VulkanExampleBase
{
public:
	bool wireframe = true;
	bool glow = true;

	Scene scene, sceneGlow;

	vks::Buffer uniformBuffer;

	struct UBOVS {
		glm::mat4 projection;
		glm::mat4 model;
		glm::vec4 lightPos = glm::vec4(0.0f, 5.0f, 15.0f, 1.0f);
	} uboVS;

	struct Pipelines {
		VkPipeline toonshading;
		VkPipeline color;
		VkPipeline wireframe = VK_NULL_HANDLE;
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
	struct OffscreenPass {
		int32_t width, height;
		VkFramebuffer frameBuffer;
		FrameBufferAttachment color, depth;
		VkRenderPass renderPass;
		VkSampler sampler;
		VkDescriptorImageInfo descriptor;
	} offscreenPass;

	// Random tag data
	struct DemoTag {
		const char name[17] = "debug marker tag";
	} demoTag;

	VulkanExample() : VulkanExampleBase(ENABLE_VALIDATION)
	{
		title = "Debugging with VK_EXT_debug_marker";
		camera.setRotation(glm::vec3(-4.35f, 16.25f, 0.0f));
		camera.setRotationSpeed(0.5f);
		camera.setPosition(glm::vec3(0.1f, 1.1f, -8.5f));
		camera.setPerspective(60.0f, (float)width / (float)height, 0.1f, 256.0f);
	}

	// Enable physical device features required for this example
	virtual void getEnabledFeatures()
	{
		// Fill mode non solid is required for wireframe display
		if (deviceFeatures.fillModeNonSolid) {
			enabledFeatures.fillModeNonSolid = VK_TRUE;
		};
		wireframe = deviceFeatures.fillModeNonSolid;
	}

	~VulkanExample()
	{
		// Clean up used Vulkan resources
		// Note : Inherited destructor cleans up resources stored in base class
		vkDestroyPipeline(device, pipelines.toonshading, nullptr);
		vkDestroyPipeline(device, pipelines.color, nullptr);
		vkDestroyPipeline(device, pipelines.postprocess, nullptr);
		if (pipelines.wireframe != VK_NULL_HANDLE) {
			vkDestroyPipeline(device, pipelines.wireframe, nullptr);
		}

		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

		uniformBuffer.destroy();

		// Offscreen
		// Color attachment
		vkDestroyImageView(device, offscreenPass.color.view, nullptr);
		vkDestroyImage(device, offscreenPass.color.image, nullptr);
		vkFreeMemory(device, offscreenPass.color.mem, nullptr);

		// Depth attachment
		vkDestroyImageView(device, offscreenPass.depth.view, nullptr);
		vkDestroyImage(device, offscreenPass.depth.image, nullptr);
		vkFreeMemory(device, offscreenPass.depth.mem, nullptr);

		vkDestroyRenderPass(device, offscreenPass.renderPass, nullptr);
		vkDestroySampler(device, offscreenPass.sampler, nullptr);
		vkDestroyFramebuffer(device, offscreenPass.frameBuffer, nullptr);
	}

	// Prepare a texture target and framebuffer for offscreen rendering
	void prepareOffscreen()
	{
		offscreenPass.width = OFFSCREEN_DIM;
		offscreenPass.height = OFFSCREEN_DIM;

		// Find a suitable depth format
		VkFormat fbDepthFormat;
		VkBool32 validDepthFormat = vks::tools::getSupportedDepthFormat(physicalDevice, &fbDepthFormat);
		assert(validDepthFormat);

		// Color attachment
		VkImageCreateInfo image = vks::initializers::imageCreateInfo();
		image.imageType = VK_IMAGE_TYPE_2D;
		image.format = OFFSCREEN_FORMAT;
		image.extent.width = offscreenPass.width;
		image.extent.height = offscreenPass.height;
		image.extent.depth = 1;
		image.mipLevels = 1;
		image.arrayLayers = 1;
		image.samples = VK_SAMPLE_COUNT_1_BIT;
		image.tiling = VK_IMAGE_TILING_OPTIMAL;
		// We will sample directly from the color attachment
		image.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		VkMemoryAllocateInfo memAlloc = vks::initializers::memoryAllocateInfo();
		VkMemoryRequirements memReqs;

		VK_CHECK_RESULT(vkCreateImage(device, &image, nullptr, &offscreenPass.color.image));
		vkGetImageMemoryRequirements(device, offscreenPass.color.image, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		memAlloc.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &offscreenPass.color.mem));
		VK_CHECK_RESULT(vkBindImageMemory(device, offscreenPass.color.image, offscreenPass.color.mem, 0));

		VkImageViewCreateInfo colorImageView = vks::initializers::imageViewCreateInfo();
		colorImageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
		colorImageView.format = OFFSCREEN_FORMAT;
		colorImageView.subresourceRange = {};
		colorImageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		colorImageView.subresourceRange.baseMipLevel = 0;
		colorImageView.subresourceRange.levelCount = 1;
		colorImageView.subresourceRange.baseArrayLayer = 0;
		colorImageView.subresourceRange.layerCount = 1;
		colorImageView.image = offscreenPass.color.image;
		VK_CHECK_RESULT(vkCreateImageView(device, &colorImageView, nullptr, &offscreenPass.color.view));

		// Create sampler to sample from the attachment in the fragment shader
		VkSamplerCreateInfo samplerInfo = vks::initializers::samplerCreateInfo();
		samplerInfo.magFilter = OFFSCREEN_FILTER;
		samplerInfo.minFilter = OFFSCREEN_FILTER;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeV = samplerInfo.addressModeU;
		samplerInfo.addressModeW = samplerInfo.addressModeU;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.maxAnisotropy = 1.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 1.0f;
		samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		VK_CHECK_RESULT(vkCreateSampler(device, &samplerInfo, nullptr, &offscreenPass.sampler));

		// Depth stencil attachment
		image.format = fbDepthFormat;
		image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

		VK_CHECK_RESULT(vkCreateImage(device, &image, nullptr, &offscreenPass.depth.image));
		vkGetImageMemoryRequirements(device, offscreenPass.depth.image, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		memAlloc.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &offscreenPass.depth.mem));
		VK_CHECK_RESULT(vkBindImageMemory(device, offscreenPass.depth.image, offscreenPass.depth.mem, 0));

		VkImageViewCreateInfo depthStencilView = vks::initializers::imageViewCreateInfo();
		depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
		depthStencilView.format = fbDepthFormat;
		depthStencilView.flags = 0;
		depthStencilView.subresourceRange = {};
		depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		depthStencilView.subresourceRange.baseMipLevel = 0;
		depthStencilView.subresourceRange.levelCount = 1;
		depthStencilView.subresourceRange.baseArrayLayer = 0;
		depthStencilView.subresourceRange.layerCount = 1;
		depthStencilView.image = offscreenPass.depth.image;
		VK_CHECK_RESULT(vkCreateImageView(device, &depthStencilView, nullptr, &offscreenPass.depth.view));

		// Create a separate render pass for the offscreen rendering as it may differ from the one used for scene rendering

		std::array<VkAttachmentDescription, 2> attchmentDescriptions = {};
		// Color attachment
		attchmentDescriptions[0].format = OFFSCREEN_FORMAT;
		attchmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
		attchmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attchmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attchmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attchmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attchmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attchmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		// Depth attachment
		attchmentDescriptions[1].format = fbDepthFormat;
		attchmentDescriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
		attchmentDescriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attchmentDescriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attchmentDescriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attchmentDescriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attchmentDescriptions[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attchmentDescriptions[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
		VkAttachmentReference depthReference = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

		VkSubpassDescription subpassDescription = {};
		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescription.colorAttachmentCount = 1;
		subpassDescription.pColorAttachments = &colorReference;
		subpassDescription.pDepthStencilAttachment = &depthReference;

		// Use subpass dependencies for layout transitions
		std::array<VkSubpassDependency, 2> dependencies;

		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		dependencies[1].srcSubpass = 0;
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		// Create the actual renderpass
		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attchmentDescriptions.size());
		renderPassInfo.pAttachments = attchmentDescriptions.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpassDescription;
		renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
		renderPassInfo.pDependencies = dependencies.data();

		VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &offscreenPass.renderPass));

		VkImageView attachments[2];
		attachments[0] = offscreenPass.color.view;
		attachments[1] = offscreenPass.depth.view;

		VkFramebufferCreateInfo fbufCreateInfo = vks::initializers::framebufferCreateInfo();
		fbufCreateInfo.renderPass = offscreenPass.renderPass;
		fbufCreateInfo.attachmentCount = 2;
		fbufCreateInfo.pAttachments = attachments;
		fbufCreateInfo.width = offscreenPass.width;
		fbufCreateInfo.height = offscreenPass.height;
		fbufCreateInfo.layers = 1;

		VK_CHECK_RESULT(vkCreateFramebuffer(device, &fbufCreateInfo, nullptr, &offscreenPass.frameBuffer));

		// Fill a descriptor for later use in a descriptor set
		offscreenPass.descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		offscreenPass.descriptor.imageView = offscreenPass.color.view;
		offscreenPass.descriptor.sampler = offscreenPass.sampler;

		// Name some objects for debugging
		DebugMarker::setObjectName(device, (uint64_t)offscreenPass.color.image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, "Off-screen color framebuffer");
		DebugMarker::setObjectName(device, (uint64_t)offscreenPass.depth.image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, "Off-screen depth framebuffer");
		DebugMarker::setObjectName(device, (uint64_t)offscreenPass.sampler, VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT, "Off-screen framebuffer default sampler");
	}

	void loadAssets()
	{
		scene.loadFromFile(getAssetPath() + "models/treasure_smooth.gltf", vulkanDevice, queue);
		sceneGlow.loadFromFile(getAssetPath() + "models/treasure_glow.gltf", vulkanDevice, queue);
		// Name the buffers for debugging
		// Scene
		DebugMarker::setObjectName(device, (uint64_t)scene.model.vertices.buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, "Scene vertex buffer");
		DebugMarker::setObjectName(device, (uint64_t)scene.model.indices.buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, "Scene index buffer");
		// Glow
		DebugMarker::setObjectName(device, (uint64_t)sceneGlow.model.vertices.buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, "Glow vertex buffer");
		DebugMarker::setObjectName(device, (uint64_t)sceneGlow.model.indices.buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, "Glow index buffer");
	}

	void buildCommandBuffers()
	{
		VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();
		VkClearValue clearValues[2];
		VkViewport viewport;
		VkRect2D scissor;
		VkDeviceSize offsets[1] = { 0 };

		for (int32_t i = 0; i < drawCmdBuffers.size(); ++i)
		{
			VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo));

			/*
				First render pass: Offscreen rendering
			*/
			if (glow)
			{
				VkClearValue clearValues[2];
				clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
				clearValues[1].depthStencil = { 1.0f, 0 };

				VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
				renderPassBeginInfo.renderPass = offscreenPass.renderPass;
				renderPassBeginInfo.framebuffer = offscreenPass.frameBuffer;
				renderPassBeginInfo.renderArea.extent.width = offscreenPass.width;
				renderPassBeginInfo.renderArea.extent.height = offscreenPass.height;
				renderPassBeginInfo.clearValueCount = 2;
				renderPassBeginInfo.pClearValues = clearValues;

				// Start a new debug marker region
				DebugMarker::beginRegion(drawCmdBuffers[i], "Off-screen scene rendering", glm::vec4(1.0f, 0.78f, 0.05f, 1.0f));

				vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

				VkViewport viewport = vks::initializers::viewport((float)offscreenPass.width, (float)offscreenPass.height, 0.0f, 1.0f);
				vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

				VkRect2D scissor = vks::initializers::rect2D(offscreenPass.width, offscreenPass.height, 0, 0);
				vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

				vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets.scene, 0, NULL);
				vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.color);

				sceneGlow.draw(drawCmdBuffers[i]);

				vkCmdEndRenderPass(drawCmdBuffers[i]);

				DebugMarker::endRegion(drawCmdBuffers[i]);
			}

			/*
				Note: Explicit synchronization is not required between the render pass, as this is done implicit via sub pass dependencies
			*/

			/*
				Second render pass: Scene rendering with applied bloom
			*/
			{
				clearValues[0].color = defaultClearColor;
				clearValues[1].depthStencil = { 1.0f, 0 };

				VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
				renderPassBeginInfo.renderPass = renderPass;
				renderPassBeginInfo.framebuffer = frameBuffers[i];
				renderPassBeginInfo.renderArea.extent.width = width;
				renderPassBeginInfo.renderArea.extent.height = height;
				renderPassBeginInfo.clearValueCount = 2;
				renderPassBeginInfo.pClearValues = clearValues;

				// Start a new debug marker region
				DebugMarker::beginRegion(drawCmdBuffers[i], "Render scene", glm::vec4(0.5f, 0.76f, 0.34f, 1.0f));

				vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

				VkViewport viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
				vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

				VkRect2D scissor = vks::initializers::rect2D(wireframe ? width / 2 : width, height, 0, 0);
				vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

				vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets.scene, 0, NULL);

				// Solid rendering

				// Start a new debug marker region
				DebugMarker::beginRegion(drawCmdBuffers[i], "Toon shading draw", glm::vec4(0.78f, 0.74f, 0.9f, 1.0f));

				vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.toonshading);
				scene.draw(drawCmdBuffers[i]);

				DebugMarker::endRegion(drawCmdBuffers[i]);

				// Wireframe rendering
				if (wireframe)
				{
					// Insert debug marker
					DebugMarker::beginRegion(drawCmdBuffers[i], "Wireframe draw", glm::vec4(0.53f, 0.78f, 0.91f, 1.0f));

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
					DebugMarker::beginRegion(drawCmdBuffers[i], "Apply post processing", glm::vec4(0.93f, 0.89f, 0.69f, 1.0f));

					vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.postprocess);
					// Full screen quad is generated by the vertex shaders, so we reuse four vertices (for four invocations) from current vertex buffer
					vkCmdDraw(drawCmdBuffers[i], 4, 1, 0, 0);

					DebugMarker::endRegion(drawCmdBuffers[i]);
				}

				drawUI(drawCmdBuffers[i]);

				vkCmdEndRenderPass(drawCmdBuffers[i]);

				// End current debug marker region
				DebugMarker::endRegion(drawCmdBuffers[i]);

			}

			VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
		}
	}

	void setupDescriptorPool()
	{
		// Example uses one ubo and one combined image sampler
		std::vector<VkDescriptorPoolSize> poolSizes = {
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1),
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1),
		};
		VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes.size(), poolSizes.data(), 1);
		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));
	}

	void setupDescriptorSetLayout()
	{
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
			// Binding 0 : Vertex shader uniform buffer
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
			// Binding 1 : Fragment shader combined sampler
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
		};
		VkDescriptorSetLayoutCreateInfo descriptorLayout = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings.data(), setLayoutBindings.size());
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayout));

		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(&descriptorSetLayout, 1);
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

		// Name for debugging
		DebugMarker::setObjectName(device, (uint64_t)pipelineLayout, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT, "Shared pipeline layout");
		DebugMarker::setObjectName(device, (uint64_t)descriptorSetLayout, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT, "Shared descriptor set layout");
	}

	void setupDescriptorSet()
	{
		VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.scene));

		std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
			// Binding 0 : Vertex shader uniform buffer
			vks::initializers::writeDescriptorSet(descriptorSets.scene, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffer.descriptor),
			// Binding 1 : Color map
			vks::initializers::writeDescriptorSet(descriptorSets.scene, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &offscreenPass.descriptor)
		};
		vkUpdateDescriptorSets(device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);
	}

	void preparePipelines()
	{
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
		VkPipelineRasterizationStateCreateInfo rasterizationStateCI = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
		VkPipelineColorBlendAttachmentState blendAttachmentState = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
		VkPipelineColorBlendStateCreateInfo colorBlendStateCI = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
		VkPipelineDepthStencilStateCreateInfo depthStencilStateCI = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
		VkPipelineViewportStateCreateInfo viewportStateCI = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
		VkPipelineMultisampleStateCreateInfo multisampleStateCI = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
		std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR	};
		VkPipelineDynamicStateCreateInfo dynamicStateCI =	vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables.data(), dynamicStateEnables.size(), 0);
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

		VkGraphicsPipelineCreateInfo pipelineCI = vks::initializers::pipelineCreateInfo(pipelineLayout, renderPass);
		pipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
		pipelineCI.pRasterizationState = &rasterizationStateCI;
		pipelineCI.pColorBlendState = &colorBlendStateCI;
		pipelineCI.pMultisampleState = &multisampleStateCI;
		pipelineCI.pViewportState = &viewportStateCI;
		pipelineCI.pDepthStencilState = &depthStencilStateCI;
		pipelineCI.pDynamicState = &dynamicStateCI;
		pipelineCI.stageCount = shaderStages.size();
		pipelineCI.pStages = shaderStages.data();
		pipelineCI.pVertexInputState            = vkglTF::Vertex::getPipelineVertexInputState({vkglTF::VertexComponent::Position, vkglTF::VertexComponent::Normal, vkglTF::VertexComponent::UV, vkglTF::VertexComponent::Color});

		// Toon shading pipeline
		shaderStages[0] = loadShader(getShadersPath() + "debugmarker/toon.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getShadersPath() + "debugmarker/toon.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.toonshading));

		// Color only pipeline
		shaderStages[0] = loadShader(getShadersPath() + "debugmarker/colorpass.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getShadersPath() + "debugmarker/colorpass.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		pipelineCI.renderPass = offscreenPass.renderPass;
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.color));

		// Wire frame rendering pipeline
		if (deviceFeatures.fillModeNonSolid)
		{
			rasterizationStateCI.polygonMode = VK_POLYGON_MODE_LINE;
			pipelineCI.renderPass = renderPass;
			VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.wireframe));
		}

		// Post processing effect
		shaderStages[0] = loadShader(getShadersPath() + "debugmarker/postprocess.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getShadersPath() + "debugmarker/postprocess.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		depthStencilStateCI.depthTestEnable = VK_FALSE;
		depthStencilStateCI.depthWriteEnable = VK_FALSE;
		rasterizationStateCI.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationStateCI.cullMode = VK_CULL_MODE_NONE;
		blendAttachmentState.colorWriteMask = 0xF;
		blendAttachmentState.blendEnable =  VK_TRUE;
		blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
		blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
		blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
		blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.postprocess));

		// Name shader modules for debugging
		// Shader module count starts at 2 when UI overlay in base class is enabled
		uint32_t moduleIndex = settings.overlay ? 2 : 0;
		DebugMarker::setObjectName(device, (uint64_t)shaderModules[moduleIndex + 0], VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT, "Toon shading vertex shader");
		DebugMarker::setObjectName(device, (uint64_t)shaderModules[moduleIndex + 1], VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT, "Toon shading fragment shader");
		DebugMarker::setObjectName(device, (uint64_t)shaderModules[moduleIndex + 2], VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT, "Color-only vertex shader");
		DebugMarker::setObjectName(device, (uint64_t)shaderModules[moduleIndex + 3], VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT, "Color-only fragment shader");
		DebugMarker::setObjectName(device, (uint64_t)shaderModules[moduleIndex + 4], VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT, "Postprocess vertex shader");
		DebugMarker::setObjectName(device, (uint64_t)shaderModules[moduleIndex + 5], VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT, "Postprocess fragment shader");

		// Name pipelines for debugging
		DebugMarker::setObjectName(device, (uint64_t)pipelines.toonshading, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT, "Toon shading pipeline");
		DebugMarker::setObjectName(device, (uint64_t)pipelines.color, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT, "Color only pipeline");

		if (deviceFeatures.fillModeNonSolid)
		{
			DebugMarker::setObjectName(device, (uint64_t)pipelines.wireframe, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT, "Wireframe rendering pipeline");
		}

		DebugMarker::setObjectName(device, (uint64_t)pipelines.postprocess, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT, "Post processing pipeline");
	}

	// Prepare and initialize uniform buffer containing shader uniforms
	void prepareUniformBuffers()
	{
		// Vertex shader uniform buffer block
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&uniformBuffer,
			sizeof(uboVS)));

		// Map persistent
		VK_CHECK_RESULT(uniformBuffer.map());


		// Name uniform buffer for debugging
		DebugMarker::setObjectName(device, (uint64_t)uniformBuffer.buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, "Scene uniform buffer block");
		// Add some random tag
		DebugMarker::setObjectTag(device, (uint64_t)uniformBuffer.buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, 0, sizeof(demoTag), &demoTag);

		updateUniformBuffers();
	}

	void updateUniformBuffers()
	{
		uboVS.projection = camera.matrices.perspective;
		uboVS.model = camera.matrices.view;
		memcpy(uniformBuffer.mapped, &uboVS, sizeof(uboVS));
	}

	void draw()
	{
		VulkanExampleBase::prepareFrame();
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
		VulkanExampleBase::submitFrame();
	}

	void prepare()
	{
		VulkanExampleBase::prepare();
		DebugMarker::setup(device, physicalDevice);
		loadAssets();
		prepareOffscreen();
		prepareUniformBuffers();
		setupDescriptorSetLayout();
		preparePipelines();
		setupDescriptorPool();
		setupDescriptorSet();
		buildCommandBuffers();
		prepared = true;
	}

	virtual void render()
	{
		if (!prepared)
			return;
		draw();
		if (camera.updated)
			updateUniformBuffers();
	}


	virtual void OnUpdateUIOverlay(vks::UIOverlay *overlay)
	{
		if (overlay->header("Info")) {
			overlay->text("VK_EXT_debug_marker %s", (DebugMarker::active ? "active" : "not present"));
		}
		if (overlay->header("Settings")) {
			if (overlay->checkBox("Glow", &glow)) {
				buildCommandBuffers();
			}
			if (deviceFeatures.fillModeNonSolid) {
				if (overlay->checkBox("Wireframe", &wireframe)) {
					buildCommandBuffers();
				}
			}
		}
	}
};

VULKAN_EXAMPLE_MAIN()