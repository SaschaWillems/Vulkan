/*
* Vulkan Example - Textured quad
*
* Note :
*	This is a basic android example. It may be integrated into the other examples at some point in the future.
*	Until then this serves as a starting point for using Vulkan on Android, with some of the functionality required
*	already moved to the example base classes (e.g. swap chain)
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include <assert.h>
#include "vulkanandroid.h"
#include "vulkanswapchain.hpp" 
#include "vulkanandroidbase.hpp"
#include <android/asset_manager.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "AndroidProject1.NativeActivity", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "AndroidProject1.NativeActivity", __VA_ARGS__))

#define VERTEX_BUFFER_BIND_ID 0

struct saved_state {
	glm::vec3 rotation;
	float zoom;
};

class VulkanExample : public VulkanAndroidExampleBase
{
public:
	int animating;
	struct saved_state state;

	// Vulkan
	struct Vertex {
		float pos[3];
		float uv[2];
	};

	struct Texture {
		VkSampler sampler;
		VkImage image;
		VkImageLayout imageLayout;
		VkDeviceMemory deviceMemory;
		VkImageView view;
		uint32_t width, height;
		uint32_t mipLevels;
	} texture;

	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSet descriptorSet;
	VkPipelineLayout pipelineLayout;

	struct {
		VkBuffer buf;
		VkDeviceMemory mem;
		VkPipelineVertexInputStateCreateInfo inputState;
		std::vector<VkVertexInputBindingDescription> bindingDescriptions;
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
	} vertices;

	struct {
		int count;
		VkBuffer buf;
		VkDeviceMemory mem;
	} indices;

	struct {
		VkBuffer buffer;
		VkDeviceMemory memory;
		VkDescriptorBufferInfo descriptor;
	}  uniformDataVS;

	struct {
		glm::mat4 projection;
		glm::mat4 model;
	} uboVS;

	struct {
		VkPipeline solid;
	} pipelines;

	void loadTexture(const char* fileName, VkFormat format, bool forceLinearTiling)
	{
		VkFormatProperties formatProperties;
		VkResult err;

		AAsset* asset = AAssetManager_open(app->activity->assetManager, fileName, AASSET_MODE_STREAMING);
		assert(asset);
		size_t size = AAsset_getLength(asset);
		assert(size > 0);

		void *textureData = malloc(size);
		AAsset_read(asset, textureData, size);
		AAsset_close(asset);

		gli::texture2D tex2D(gli::load((const char*)textureData, size));
		assert(!tex2D.empty());

		texture.width = tex2D[0].dimensions().x;
		texture.height = tex2D[0].dimensions().y;
		texture.mipLevels = tex2D.levels();

		// Get device properites for the requested texture format
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);

		// Only use linear tiling if requested (and supported by the device)
		// Support for linear tiling is mostly limited, so prefer to use
		// optimal tiling instead
		// On most implementations linear tiling will only support a very
		// limited amount of formats and features (mip maps, cubemaps, arrays, etc.)
		VkBool32 useStaging = true;

		// Only use linear tiling if forced
		if (forceLinearTiling)
		{
			// Don't use linear if format is not supported for (linear) shader sampling
			useStaging = !(formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);
		}

		VkImageCreateInfo imageCreateInfo = vkTools::initializers::imageCreateInfo();
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.format = format;
		imageCreateInfo.mipLevels = 1;
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
		imageCreateInfo.usage = (useStaging) ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : VK_IMAGE_USAGE_SAMPLED_BIT;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfo.flags = 0;
		imageCreateInfo.extent = { texture.width, texture.height, 1 };

		VkMemoryAllocateInfo memAllocInfo = vkTools::initializers::memoryAllocateInfo();
		VkMemoryRequirements memReqs;

		startSetupCommandBuffer();

		if (useStaging)
		{
			// Load all available mip levels into linear textures
			// and copy to optimal tiling target
			struct MipLevel {
				VkImage image;
				VkDeviceMemory memory;
			};
			std::vector<MipLevel> mipLevels;
			mipLevels.resize(texture.mipLevels);

			// Copy mip levels
			for (uint32_t level = 0; level < texture.mipLevels; ++level)
			{
				imageCreateInfo.extent.width = tex2D[level].dimensions().x;
				imageCreateInfo.extent.height = tex2D[level].dimensions().y;
				imageCreateInfo.extent.depth = 1;

				err = vkCreateImage(device, &imageCreateInfo, nullptr, &mipLevels[level].image);
				assert(!err);

				vkGetImageMemoryRequirements(device, mipLevels[level].image, &memReqs);
				memAllocInfo.allocationSize = memReqs.size;
				getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memAllocInfo.memoryTypeIndex);
				err = vkAllocateMemory(device, &memAllocInfo, nullptr, &mipLevels[level].memory);
				assert(!err);
				err = vkBindImageMemory(device, mipLevels[level].image, mipLevels[level].memory, 0);
				assert(!err);

				VkImageSubresource subRes = {};
				subRes.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

				VkSubresourceLayout subResLayout;
				void *data;

				vkGetImageSubresourceLayout(device, mipLevels[level].image, &subRes, &subResLayout);
				assert(!err);
				err = vkMapMemory(device, mipLevels[level].memory, 0, memReqs.size, 0, &data);
				assert(!err);
				size_t levelSize = tex2D[level].size();
				memcpy(data, tex2D[level].data(), levelSize);
				vkUnmapMemory(device, mipLevels[level].memory);

				LOGW("setImageLayout %d", 1);

				// Image barrier for linear image (base)
				// Linear image will be used as a source for the copy
				vkTools::setImageLayout(
					setupCmdBuffer,
					mipLevels[level].image,
					VK_IMAGE_ASPECT_COLOR_BIT,
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
			}

			// Setup texture as blit target with optimal tiling
			imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			imageCreateInfo.mipLevels = texture.mipLevels;
			imageCreateInfo.extent = { texture.width, texture.height, 1 };

			err = vkCreateImage(device, &imageCreateInfo, nullptr, &texture.image);
			assert(!err);

			vkGetImageMemoryRequirements(device, texture.image, &memReqs);

			memAllocInfo.allocationSize = memReqs.size;

			getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memAllocInfo.memoryTypeIndex);
			err = vkAllocateMemory(device, &memAllocInfo, nullptr, &texture.deviceMemory);
			assert(!err);
			err = vkBindImageMemory(device, texture.image, texture.deviceMemory, 0);
			assert(!err);

			// Image barrier for optimal image (target)
			// Optimal image will be used as destination for the copy
			vkTools::setImageLayout(
				setupCmdBuffer,
				texture.image,
				VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

			// Copy mip levels one by one
			for (uint32_t level = 0; level < texture.mipLevels; ++level)
			{
				// Copy region for image blit
				VkImageCopy copyRegion = {};

				copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				copyRegion.srcSubresource.baseArrayLayer = 0;
				copyRegion.srcSubresource.mipLevel = 0;
				copyRegion.srcSubresource.layerCount = 1;
				copyRegion.srcOffset = { 0, 0, 0 };

				copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				copyRegion.dstSubresource.baseArrayLayer = 0;
				// Set mip level to copy the linear image to
				copyRegion.dstSubresource.mipLevel = level;
				copyRegion.dstSubresource.layerCount = 1;
				copyRegion.dstOffset = { 0, 0, 0 };

				copyRegion.extent.width = tex2D[level].dimensions().x;
				copyRegion.extent.height = tex2D[level].dimensions().y;
				copyRegion.extent.depth = 1;

				// Put image copy into command buffer
				vkCmdCopyImage(
					setupCmdBuffer,
					mipLevels[level].image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1, &copyRegion);

				// Change texture image layout to shader read after the copy
				texture.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				vkTools::setImageLayout(
					setupCmdBuffer,
					texture.image,
					VK_IMAGE_ASPECT_COLOR_BIT,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					texture.imageLayout);
			}

			flushSetupCommandBuffer();

			// Clean up linear images 
			// No longer required after mip levels
			// have been transformed over to optimal tiling
			for (auto& level : mipLevels)
			{
				vkDestroyImage(device, level.image, nullptr);
				vkFreeMemory(device, level.memory, nullptr);
			}
		}
		else
		{
			// Prefer using optimal tiling, as linear tiling 
			// may support only a small set of features 
			// depending on implementation (e.g. no mip maps, only one layer, etc.)

			VkImage mappableImage;
			VkDeviceMemory mappableMemory;

			// Load mip map level 0 to linear tiling image
			err = vkCreateImage(device, &imageCreateInfo, nullptr, &mappableImage);
			assert(!err);

			// Get memory requirements for this image 
			// like size and alignment
			vkGetImageMemoryRequirements(device, mappableImage, &memReqs);
			// Set memory allocation size to required memory size
			memAllocInfo.allocationSize = memReqs.size;

			// Get memory type that can be mapped to host memory
			getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memAllocInfo.memoryTypeIndex);

			// Allocate host memory
			err = vkAllocateMemory(device, &memAllocInfo, nullptr, &mappableMemory);
			assert(!err);

			// Bind allocated image for use
			err = vkBindImageMemory(device, mappableImage, mappableMemory, 0);
			assert(!err);

			// Get sub resource layout
			// Mip map count, array layer, etc.
			VkImageSubresource subRes = {};
			subRes.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

			VkSubresourceLayout subResLayout;
			void *data;

			// Get sub resources layout 
			// Includes row pitch, size offsets, etc.
			vkGetImageSubresourceLayout(device, mappableImage, &subRes, &subResLayout);
			assert(!err);

			// Map image memory
			err = vkMapMemory(device, mappableMemory, 0, memReqs.size, 0, &data);
			assert(!err);

			// Copy image data into memory
			memcpy(data, tex2D[subRes.mipLevel].data(), tex2D[subRes.mipLevel].size());

			vkUnmapMemory(device, mappableMemory);

			// Linear tiled images don't need to be staged
			// and can be directly used as textures
			texture.image = mappableImage;
			texture.deviceMemory = mappableMemory;
			texture.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			// Setup image memory barrier
			vkTools::setImageLayout(
				setupCmdBuffer,
				texture.image,
				VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
				texture.imageLayout);

			flushSetupCommandBuffer();
		}


		// Create sampler
		// In Vulkan textures are accessed by samplers
		// This separates all the sampling information from the 
		// texture data
		// This means you could have multiple sampler objects
		// for the same texture with different settings
		// Similar to the samplers available with OpenGL 3.3
		VkSamplerCreateInfo sampler = vkTools::initializers::samplerCreateInfo();
		sampler.magFilter = VK_FILTER_LINEAR;
		sampler.minFilter = VK_FILTER_LINEAR;
		sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler.addressModeV = sampler.addressModeU;
		sampler.addressModeW = sampler.addressModeU;
		sampler.mipLodBias = 0.0f;
		sampler.compareOp = VK_COMPARE_OP_NEVER;
		sampler.minLod = 0.0f;
		// Max level-of-detail should match mip level count
		sampler.maxLod = (useStaging) ? (float)texture.mipLevels : 0.0f;
		// Enable anisotropic filtering
		sampler.maxAnisotropy = 8;
		sampler.anisotropyEnable = VK_TRUE;
		sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		err = vkCreateSampler(device, &sampler, nullptr, &texture.sampler);
		assert(!err);

		// Create image view
		// Textures are not directly accessed by the shaders and
		// are abstracted by image views containing additional
		// information and sub resource ranges
		VkImageViewCreateInfo view = vkTools::initializers::imageViewCreateInfo();
		view.image = VK_NULL_HANDLE;
		view.viewType = VK_IMAGE_VIEW_TYPE_2D;
		view.format = format;
		view.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		view.subresourceRange.baseMipLevel = 0;
		view.subresourceRange.baseArrayLayer = 0;
		view.subresourceRange.layerCount = 1;
		// Linear tiling usually won't support mip maps
		// Only set mip map count if optimal tiling is used
		view.subresourceRange.levelCount = (useStaging) ? texture.mipLevels : 1;
		view.image = texture.image;
		err = vkCreateImageView(device, &view, nullptr, &texture.view);
		assert(!err);
	}

	// Free staging resources used while creating a texture
	void destroyTextureImage(struct Texture texture)
	{
		vkDestroyImage(device, texture.image, nullptr);
		vkFreeMemory(device, texture.deviceMemory, nullptr);
	}

	void initVulkan()
	{
		VulkanAndroidExampleBase::initVulkan();

		loadTexture(
			"textures/vulkan_android_robot.ktx", 
			VK_FORMAT_R8G8B8A8_UNORM, 
			false);

		prepareVertices();
		prepareUniformBuffers();
		setupDescriptorSetLayout();
		preparePipelines();
		setupDescriptorPool();
		setupDescriptorSet();

		buildCommandBuffers();

		state.zoom = -5.0f;
		state.rotation = glm::vec3();

		prepared = true;
	}

	void cleanupVulkan()
	{
		prepared = false;
		vkDestroyPipeline(device, pipelines.solid, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
		vkDestroyBuffer(device, vertices.buf, nullptr);
		vkFreeMemory(device, vertices.mem, nullptr);
		vkDestroyBuffer(device, indices.buf, nullptr);
		vkFreeMemory(device, indices.mem, nullptr);
		vkDestroyBuffer(device, uniformDataVS.buffer, nullptr);
		vkFreeMemory(device, uniformDataVS.memory, nullptr);

		destroyTextureImage(texture);

		VulkanExample::cleanUpVulkan();
	}

	void prepareVertices()
	{
		// Setup vertices
		std::vector<Vertex> vertexBuffer;
		vertexBuffer.push_back({ { 1.0f,  1.0f, 0.0f },{ 1.0f, 1.0f } });
		vertexBuffer.push_back({ { -1.0f,  1.0f, 0.0f },{ 0.0f, 1.0f } });
		vertexBuffer.push_back({ { -1.0f, -1.0f, 0.0f },{ 0.0f, 0.0f } });
		vertexBuffer.push_back({ { 1.0f, -1.0f, 0.0f },{ 1.0f, 0.0f } });
		int vertexBufferSize = vertexBuffer.size() * sizeof(Vertex);

		// Setup indices
		std::vector<uint32_t> indexBuffer;
		indexBuffer.push_back(0);
		indexBuffer.push_back(1);
		indexBuffer.push_back(2);
		indexBuffer.push_back(2);
		indexBuffer.push_back(3);
		indexBuffer.push_back(0);
		int indexBufferSize = indexBuffer.size() * sizeof(uint32_t);

		VkMemoryAllocateInfo memAlloc = {};
		memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memAlloc.pNext = NULL;
		memAlloc.allocationSize = 0;
		memAlloc.memoryTypeIndex = 0;
		VkMemoryRequirements memReqs;

		VkResult err;
		void *data;

		// Generate vertex buffer
		//	Setup
		VkBufferCreateInfo bufInfo = {};
		bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufInfo.pNext = NULL;
		bufInfo.size = vertexBufferSize;
		bufInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		bufInfo.flags = 0;
		//	Copy vertex data to VRAM
		memset(&vertices, 0, sizeof(vertices));
		err = vkCreateBuffer(device, &bufInfo, nullptr, &vertices.buf);
		assert(!err);
		vkGetBufferMemoryRequirements(device, vertices.buf, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memAlloc.memoryTypeIndex);
		vkAllocateMemory(device, &memAlloc, nullptr, &vertices.mem);
		assert(!err);
		err = vkMapMemory(device, vertices.mem, 0, memAlloc.allocationSize, 0, &data);
		assert(!err);
		memcpy(data, vertexBuffer.data(), vertexBufferSize);
		vkUnmapMemory(device, vertices.mem);
		assert(!err);
		err = vkBindBufferMemory(device, vertices.buf, vertices.mem, 0);
		assert(!err);

		// Generate index buffer
		//	Setup
		VkBufferCreateInfo indexbufferInfo = {};
		indexbufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		indexbufferInfo.pNext = NULL;
		indexbufferInfo.size = indexBufferSize;
		indexbufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		indexbufferInfo.flags = 0;
		// Copy index data to VRAM
		memset(&indices, 0, sizeof(indices));
		err = vkCreateBuffer(device, &bufInfo, nullptr, &indices.buf);
		assert(!err);
		vkGetBufferMemoryRequirements(device, indices.buf, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memAlloc.memoryTypeIndex);
		err = vkAllocateMemory(device, &memAlloc, nullptr, &indices.mem);
		assert(!err);
		err = vkMapMemory(device, indices.mem, 0, indexBufferSize, 0, &data);
		assert(!err);
		memcpy(data, indexBuffer.data(), indexBufferSize);
		vkUnmapMemory(device, indices.mem);
		err = vkBindBufferMemory(device, indices.buf, indices.mem, 0);
		assert(!err);
		indices.count = indexBuffer.size();

		// Binding description
		vertices.bindingDescriptions.resize(1);
		vertices.bindingDescriptions[0] =
			vkTools::initializers::vertexInputBindingDescription(
				VERTEX_BUFFER_BIND_ID,
				sizeof(Vertex),
				VK_VERTEX_INPUT_RATE_VERTEX);

		// Attribute descriptions
		// Describes memory layout and shader positions
		vertices.attributeDescriptions.resize(2);
		// Location 0 : Position
		vertices.attributeDescriptions[0] =
			vkTools::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				0,
				VK_FORMAT_R32G32B32_SFLOAT,
				0);
		// Location 1 : UV
		vertices.attributeDescriptions[1] =
			vkTools::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				1,
				VK_FORMAT_R32G32_SFLOAT,
				sizeof(float) * 3);

		// Assign to vertex buffer
		vertices.inputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertices.inputState.pNext = NULL;
		vertices.inputState.vertexBindingDescriptionCount = vertices.bindingDescriptions.size();
		vertices.inputState.pVertexBindingDescriptions = vertices.bindingDescriptions.data();
		vertices.inputState.vertexAttributeDescriptionCount = vertices.attributeDescriptions.size();
		vertices.inputState.pVertexAttributeDescriptions = vertices.attributeDescriptions.data();
	}

	void updateUniformBuffers()
	{
		// Update matrices
		uboVS.projection = glm::perspective(glm::radians(60.0f), (float)width / (float)height, 0.1f, 256.0f);

		glm::mat4 viewMatrix = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, state.zoom));

		uboVS.model = viewMatrix;
		uboVS.model = glm::rotate(uboVS.model, glm::radians(state.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		uboVS.model = glm::rotate(uboVS.model, glm::radians(state.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		uboVS.model = glm::rotate(uboVS.model, glm::radians(state.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

		// Map uniform buffer and update it
		uint8_t *pData;
		VkResult err = vkMapMemory(device, uniformDataVS.memory, 0, sizeof(uboVS), 0, (void **)&pData);
		assert(!err);
		memcpy(pData, &uboVS, sizeof(uboVS));
		vkUnmapMemory(device, uniformDataVS.memory);
		assert(!err);
	}

	void prepareUniformBuffers()
	{
		// Prepare and initialize uniform buffer containing shader uniforms
		VkMemoryRequirements memReqs;

		// Vertex shader uniform buffer block
		VkBufferCreateInfo bufferInfo = {};
		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.pNext = NULL;
		allocInfo.allocationSize = 0;
		allocInfo.memoryTypeIndex = 0;
		VkResult err;

		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = sizeof(uboVS);
		bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

		// Create a new buffer
		err = vkCreateBuffer(device, &bufferInfo, nullptr, &uniformDataVS.buffer);
		assert(!err);
		// Get memory requirements including size, alignment and memory type 
		vkGetBufferMemoryRequirements(device, uniformDataVS.buffer, &memReqs);
		allocInfo.allocationSize = memReqs.size;
		// Gets the appropriate memory type for this type of buffer allocation
		// Only memory types that are visible to the host
		getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &allocInfo.memoryTypeIndex);
		// Allocate memory for the uniform buffer
		err = vkAllocateMemory(device, &allocInfo, nullptr, &(uniformDataVS.memory));
		assert(!err);
		// Bind memory to buffer
		err = vkBindBufferMemory(device, uniformDataVS.buffer, uniformDataVS.memory, 0);
		assert(!err);

		// Store information in the uniform's descriptor
		uniformDataVS.descriptor.buffer = uniformDataVS.buffer;
		uniformDataVS.descriptor.offset = 0;
		uniformDataVS.descriptor.range = sizeof(uboVS);

		updateUniformBuffers();
	}

	void preparePipelines()
	{
		VkResult err;

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
			vkTools::initializers::pipelineInputAssemblyStateCreateInfo(
				VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
				0,
				VK_FALSE);

		VkPipelineRasterizationStateCreateInfo rasterizationState =
			vkTools::initializers::pipelineRasterizationStateCreateInfo(
				VK_POLYGON_MODE_FILL,
				VK_CULL_MODE_NONE,
				VK_FRONT_FACE_COUNTER_CLOCKWISE,
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

		std::vector<VkDynamicState> dynamicStateEnables;
		dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);
		dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);

		VkPipelineDynamicStateCreateInfo dynamicState =
			vkTools::initializers::pipelineDynamicStateCreateInfo(
				dynamicStateEnables.data(),
				dynamicStateEnables.size(),
				0);

		// Rendering pipeline
		// Load shaders
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

		shaderStages[0] = loadShader("shaders/texture.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader("shaders/texture.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

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
		pipelineCreateInfo.renderPass = renderPass;

		err = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.solid);
		assert(!err);
	}

	void setupDescriptorPool()
	{
		std::vector<VkDescriptorPoolSize> poolSizes;
		poolSizes.push_back(vkTools::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1));
		poolSizes.push_back(vkTools::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1));

		VkDescriptorPoolCreateInfo descriptorPoolInfo =
			vkTools::initializers::descriptorPoolCreateInfo(
				poolSizes.size(),
				poolSizes.data(),
				2);

		VkResult vkRes = vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool);
		assert(!vkRes);
	}

	void setupDescriptorSetLayout()
	{
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings;
		setLayoutBindings.push_back(
			// Binding 0 : Vertex shader uniform buffer
			vkTools::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				VK_SHADER_STAGE_VERTEX_BIT,
				0));
		setLayoutBindings.push_back(
			// Binding 1 : Fragment shader image sampler
			vkTools::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_SHADER_STAGE_FRAGMENT_BIT,
				1));

		VkDescriptorSetLayoutCreateInfo descriptorLayout =
			vkTools::initializers::descriptorSetLayoutCreateInfo(
				setLayoutBindings.data(),
				setLayoutBindings.size());

		VkResult err = vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayout);
		assert(!err);

		VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo =
			vkTools::initializers::pipelineLayoutCreateInfo(
				&descriptorSetLayout,
				1);

		err = vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &pipelineLayout);
		assert(!err);
	}

	void setupDescriptorSet()
	{
		VkDescriptorSetAllocateInfo allocInfo =
			vkTools::initializers::descriptorSetAllocateInfo(
				descriptorPool,
				&descriptorSetLayout,
				1);

		VkResult vkRes = vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet);
		assert(!vkRes);

		// Image descriptor for the color map texture
		VkDescriptorImageInfo texDescriptor =
			vkTools::initializers::descriptorImageInfo(
				texture.sampler,
				texture.view,
				VK_IMAGE_LAYOUT_GENERAL);

		std::vector<VkWriteDescriptorSet> writeDescriptorSets;
		writeDescriptorSets.push_back(
			// Binding 0 : Vertex shader uniform buffer
			vkTools::initializers::writeDescriptorSet(
				descriptorSet,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				0,
				&uniformDataVS.descriptor));
		writeDescriptorSets.push_back(
			// Binding 1 : Fragment shader texture sampler
			vkTools::initializers::writeDescriptorSet(
				descriptorSet,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				1,
				&texDescriptor));

		vkUpdateDescriptorSets(device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);
	}

	void buildCommandBuffers()
	{
		VkCommandBufferBeginInfo cmdBufInfo = {};
		cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBufInfo.pNext = NULL;

		VkClearValue clearValues[2];
		clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderPassBeginInfo = {};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.pNext = NULL;
		renderPassBeginInfo.renderPass = renderPass;
		renderPassBeginInfo.renderArea.offset.x = 0;
		renderPassBeginInfo.renderArea.offset.y = 0;
		renderPassBeginInfo.renderArea.extent.width = width;
		renderPassBeginInfo.renderArea.extent.height = height;
		renderPassBeginInfo.clearValueCount = 2;
		renderPassBeginInfo.pClearValues = clearValues;

		VkResult err;

		for (int32_t i = 0; i < drawCmdBuffers.size(); ++i)
		{
			// Set target frame buffer
			renderPassBeginInfo.framebuffer = frameBuffers[i];

			err = vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo);
			assert(!err);

			vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			// Update dynamic viewport state
			VkViewport viewport = {};
			viewport.height = (float)height;
			viewport.width = (float)width;
			viewport.minDepth = (float) 0.0f;
			viewport.maxDepth = (float) 1.0f;
			vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

			// Update dynamic scissor state
			VkRect2D scissor = {};
			scissor.extent.width = width;
			scissor.extent.height = height;
			scissor.offset.x = 0;
			scissor.offset.y = 0;
			vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

			// Bind descriptor sets describing shader binding points
			vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);

			// Bind the rendering pipeline (including the shaders)
			vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.solid);

			// Bind triangle vertices
			VkDeviceSize offsets[1] = { 0 };
			vkCmdBindVertexBuffers(drawCmdBuffers[i], VERTEX_BUFFER_BIND_ID, 1, &vertices.buf, offsets);

			// Bind triangle indices
			vkCmdBindIndexBuffer(drawCmdBuffers[i], indices.buf, 0, VK_INDEX_TYPE_UINT32);

			// Draw indexed triangle
			vkCmdDrawIndexed(drawCmdBuffers[i], indices.count, 1, 0, 0, 1);

			vkCmdEndRenderPass(drawCmdBuffers[i]);

			err = vkEndCommandBuffer(drawCmdBuffers[i]);
			assert(!err);
		}
	}

	void draw()
	{
		VkResult err;

		// Get next image in the swap chain (back/front buffer)
		err = swapChain.acquireNextImage(semaphores.presentComplete, &currentBuffer);
		assert(!err);

		submitPostPresentBarrier(swapChain.buffers[currentBuffer].image);

		VkPipelineStageFlags pipelineStages = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		VkSubmitInfo submitInfo = vkTools::initializers::submitInfo();
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &semaphores.presentComplete;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];
		submitInfo.pWaitDstStageMask = &pipelineStages;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &semaphores.submitSignal;

		// Submit to the graphics queue
		err = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
		assert(!err);

		submitPrePresentBarrier(swapChain.buffers[currentBuffer].image);

		// Present the current buffer to the swap chain
		// This will display the image
		err = swapChain.queuePresent(queue, currentBuffer, semaphores.submitSignal);
		assert(!err);
	}


};

static int32_t handleInput(struct android_app* app, AInputEvent* event)
{
	struct VulkanExample* vulkanExample = (struct VulkanExample*)app->userData;
	if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION)
	{
		// todo
		return 1;
	}
	return 0;
}

static void handleCommand(struct android_app* app, int32_t cmd)
{
	VulkanExample* vulkanExample = (VulkanExample*)app->userData;
	switch (cmd)
	{
	case APP_CMD_SAVE_STATE:
		vulkanExample->app->savedState = malloc(sizeof(struct saved_state));
		*((struct saved_state*)vulkanExample->app->savedState) = vulkanExample->state;
		vulkanExample->app->savedStateSize = sizeof(struct saved_state);
		break;
	case APP_CMD_INIT_WINDOW:
		if (vulkanExample->app->window != NULL)
		{
			vulkanExample->initVulkan();
			assert(vulkanExample->prepared);
		}
		break;
	case APP_CMD_LOST_FOCUS:
		vulkanExample->animating = 0;
		break;
	}
}

/**
* This is the main entry point of a native application that is using
* android_native_app_glue.  It runs in its own thread, with its own
* event loop for receiving input events and doing other things.
*/
void android_main(struct android_app* state)
{
	VulkanExample *engine = new VulkanExample();

	//memset(&engine, 0, sizeof(engine));
	state->userData = engine;
	state->onAppCmd = handleCommand;
	state->onInputEvent = handleInput;
	engine->app = state;

	engine->animating = 1;

	// loop waiting for stuff to do.

	while (1)
	{
		// Read all pending events.
		int ident;
		int events;
		struct android_poll_source* source;

		while ((ident = ALooper_pollAll(engine->animating ? 0 : -1, NULL, &events, (void**)&source)) >= 0)
		{
			if (source != NULL)
			{
				source->process(state, source);
			}

			if (state->destroyRequested != 0)
			{
				engine->cleanupVulkan();
				return;
			}
		}

		// Render frame
		if (engine->prepared)
		{
			if (engine->animating)
			{
				// Update rotation
				engine->state.rotation.y += 0.5f;
				if (engine->state.rotation.y > 360.0f)
				{
					engine->state.rotation.y -= 360.0f;

				}
				engine->updateUniformBuffers();
			}
			engine->draw();
		}
	}
}
