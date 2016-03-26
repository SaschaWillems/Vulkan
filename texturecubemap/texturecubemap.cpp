/*
* Vulkan Example - Cube map texture loading and displaying
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

#include <gli/gli.hpp>

#include <vulkan/vulkan.h>
#include "vulkanexamplebase.h"

#define VERTEX_BUFFER_BIND_ID 0
#define ENABLE_VALIDATION false

// Vertex layout for this example
std::vector<vkMeshLoader::VertexLayout> vertexLayout =
{
	vkMeshLoader::VERTEX_LAYOUT_POSITION,
	vkMeshLoader::VERTEX_LAYOUT_NORMAL,
	vkMeshLoader::VERTEX_LAYOUT_UV
};

class VulkanExample : public CBaseVulkanGame
{
public:
	virtual int32_t			init(CVulkanFramework* pFramework)
	{
		CBaseVulkanGame::init(pFramework);
		m_pFramework->zoom = -4.0f;
		m_pFramework->rotationSpeed = 0.25f;
		m_pFramework->rotation = { -2.25f, -35.0f, 0.0f };
		m_pFramework->title = "Vulkan Example - Cube map";
		// Values not set here are initialized in the base class constructor
		return 0;
	};

	vkTools::VulkanTexture cubeMap;

	struct {
		VkPipelineVertexInputStateCreateInfo inputState;
		std::vector<VkVertexInputBindingDescription> bindingDescriptions;
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
	} vertices;

	struct {
		vkMeshLoader::MeshBuffer skybox, object;
	} meshes;

	struct {
		vkTools::UniformData objectVS;
		vkTools::UniformData skyboxVS;
	} uniformData;

	struct {
		glm::mat4 projection;
		glm::mat4 model;
	} uboVS;

	struct {
		VkPipeline skybox;
		VkPipeline reflect;
	} pipelines;

	struct {
		VkDescriptorSet object;
		VkDescriptorSet skybox;
	} descriptorSets;

	VkPipelineLayout pipelineLayout;
	VkDescriptorSetLayout descriptorSetLayout;

	VulkanExample()
	{
	}

	~VulkanExample()
	{
		// Clean up used Vulkan resources 
		// Note : Inherited destructor cleans up resources stored in base class

		// Clean up texture resources
		vkDestroyImageView(m_pFramework->device, cubeMap.view, nullptr);
		vkDestroyImage(m_pFramework->device, cubeMap.image, nullptr);
		vkDestroySampler(m_pFramework->device, cubeMap.sampler, nullptr);
		vkFreeMemory(m_pFramework->device, cubeMap.deviceMemory, nullptr);

		vkDestroyPipeline(m_pFramework->device, pipelines.skybox, nullptr);
		vkDestroyPipeline(m_pFramework->device, pipelines.reflect, nullptr);

		vkDestroyPipelineLayout(m_pFramework->device, pipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(m_pFramework->device, descriptorSetLayout, nullptr);

		vkMeshLoader::freeMeshBufferResources(m_pFramework->device, &meshes.object);
		vkMeshLoader::freeMeshBufferResources(m_pFramework->device, &meshes.skybox);

		vkTools::destroyUniformData(m_pFramework->device, &uniformData.objectVS);
		vkTools::destroyUniformData(m_pFramework->device, &uniformData.skyboxVS);
	}

	void loadCubemap(std::string filename, VkFormat format, bool forceLinearTiling)
	{
#if defined(__ANDROID__)
		// Textures are stored inside the apk on Android (compressed)
		// So they need to be loaded via the asset manager
		AAsset* asset = AAssetManager_open(androidApp->activity->assetManager, filename.c_str(), AASSET_MODE_STREAMING);
		assert(asset);
		size_t size = AAsset_getLength(asset);
		assert(size > 0);

		void *textureData = malloc(size);
		AAsset_read(asset, textureData, size);
		AAsset_close(asset);

		gli::textureCube texCube(gli::load((const char*)textureData, size));
#else
		gli::textureCube texCube(gli::load(filename));
#endif

		assert(!texCube.empty());

		cubeMap.width =  (uint32_t)texCube[0].dimensions().x;
		cubeMap.height = (uint32_t)texCube[0].dimensions().y;

		// Get device properites for the requested texture format
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(m_pFramework->physicalDevice, format, &formatProperties);

		VkImageCreateInfo imageCreateInfo = vkTools::initializers::imageCreateInfo();
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.format = format;
		imageCreateInfo.extent = { cubeMap.width, cubeMap.height, 1 };
		imageCreateInfo.mipLevels = 1;
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
		imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
		imageCreateInfo.flags = 0;

		VkMemoryAllocateInfo memAllocInfo = vkTools::initializers::memoryAllocateInfo();
		VkMemoryRequirements memReqs;

		struct {
			VkImage image;
			VkDeviceMemory memory;
		} cubeFace[6];

		// Allocate command buffer for image copies and layouts
		VkCommandBuffer cmdBuffer;
		VkCommandBufferAllocateInfo cmdBufAlllocatInfo =
			vkTools::initializers::commandBufferAllocateInfo(
				m_pFramework->cmdPool,
				VK_COMMAND_BUFFER_LEVEL_PRIMARY,
				1);
		VkResult err = vkAllocateCommandBuffers(m_pFramework->device, &cmdBufAlllocatInfo, &cmdBuffer);
		assert(!err);

		VkCommandBufferBeginInfo cmdBufInfo =
			vkTools::initializers::commandBufferBeginInfo();

		err = vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo);
		assert(!err);

		// Load separate cube map faces into linear tiled textures
		for (uint32_t face = 0; face < 6; ++face)
		{
			err = vkCreateImage(m_pFramework->device, &imageCreateInfo, nullptr, &cubeFace[face].image);
			assert(!err);

			vkGetImageMemoryRequirements(m_pFramework->device, cubeFace[face].image, &memReqs);
			memAllocInfo.allocationSize = memReqs.size;
			m_pFramework->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memAllocInfo.memoryTypeIndex);
			err = vkAllocateMemory(m_pFramework->device, &memAllocInfo, nullptr, &cubeFace[face].memory);
			assert(!err);
			err = vkBindImageMemory(m_pFramework->device, cubeFace[face].image, cubeFace[face].memory, 0);
			assert(!err);

			VkImageSubresource subRes = {};
			subRes.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

			VkSubresourceLayout subResLayout;
			void *data;

			vkGetImageSubresourceLayout(m_pFramework->device, cubeFace[face].image, &subRes, &subResLayout);
			assert(!err);
			err = vkMapMemory(m_pFramework->device, cubeFace[face].memory, 0, memReqs.size, 0, &data);
			assert(!err);
			memcpy(data, texCube[face][subRes.mipLevel].data(), texCube[face][subRes.mipLevel].size());
			vkUnmapMemory(m_pFramework->device, cubeFace[face].memory);

			// Image barrier for linear image (base)
			// Linear image will be used as a source for the copy
			vkTools::setImageLayout(
				cmdBuffer,
				cubeFace[face].image,
				VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_PREINITIALIZED,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		}
		
		// Transfer cube map faces to optimal tiling

		// Setup texture as blit target with optimal tiling
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
		imageCreateInfo.arrayLayers = 6;

		err = vkCreateImage(m_pFramework->device, &imageCreateInfo, nullptr, &cubeMap.image);
		assert(!err);

		vkGetImageMemoryRequirements(m_pFramework->device, cubeMap.image, &memReqs);

		memAllocInfo.allocationSize = memReqs.size;

		m_pFramework->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memAllocInfo.memoryTypeIndex);
		err = vkAllocateMemory(m_pFramework->device, &memAllocInfo, nullptr, &cubeMap.deviceMemory);
		assert(!err);
		err = vkBindImageMemory(m_pFramework->device, cubeMap.image, cubeMap.deviceMemory, 0);
		assert(!err);

		// Image barrier for optimal image (target)
		// Optimal image will be used as destination for the copy

		// Set initial layout for all array layers of the optimal (target) tiled texture
		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount = 1;
		subresourceRange.layerCount = 6;

		vkTools::setImageLayout(
			cmdBuffer,
			cubeMap.image,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_PREINITIALIZED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			subresourceRange);

		// Copy cube map faces one by one
		for (uint32_t face = 0; face < 6; ++face)
		{
			// Copy region for image blit
			VkImageCopy copyRegion = {};

			copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copyRegion.srcSubresource.baseArrayLayer = 0;
			copyRegion.srcSubresource.mipLevel = 0;
			copyRegion.srcSubresource.layerCount = 1;
			copyRegion.srcOffset = { 0, 0, 0 };

			copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copyRegion.dstSubresource.baseArrayLayer = face;
			copyRegion.dstSubresource.mipLevel = 0;
			copyRegion.dstSubresource.layerCount = 1;
			copyRegion.dstOffset = { 0, 0, 0 };

			copyRegion.extent.width = cubeMap.width;
			copyRegion.extent.height = cubeMap.height;
			copyRegion.extent.depth = 1;

			// Put image copy into command buffer
			vkCmdCopyImage(
				cmdBuffer,
				cubeFace[face].image, 
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				cubeMap.image, 
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &copyRegion);
		}

		// Change texture image layout to shader read after all faces have been copied
		cubeMap.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		vkTools::setImageLayout(
			cmdBuffer,
			cubeMap.image,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			cubeMap.imageLayout,
			subresourceRange);

		err = vkEndCommandBuffer(cmdBuffer);
		assert(!err);

		VkFence nullFence = { VK_NULL_HANDLE };

		// Submit command buffer to graphis queue
		VkSubmitInfo submitInfo = vkTools::initializers::submitInfo();
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cmdBuffer;

		err = vkQueueSubmit(m_pFramework->queue, 1, &submitInfo, nullFence);
		assert(!err);

		err = vkQueueWaitIdle(m_pFramework->queue);
		assert(!err);

		// Create sampler
		VkSamplerCreateInfo sampler = vkTools::initializers::samplerCreateInfo();
		sampler.magFilter = VK_FILTER_LINEAR;
		sampler.minFilter = VK_FILTER_LINEAR;
		sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler.addressModeV = sampler.addressModeU;
		sampler.addressModeW = sampler.addressModeU;
		sampler.mipLodBias = 0.0f;
		sampler.maxAnisotropy = 8;
		sampler.compareOp = VK_COMPARE_OP_NEVER;
		sampler.minLod = 0.0f;
		sampler.maxLod = 0.0f;
		sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		err = vkCreateSampler(m_pFramework->device, &sampler, nullptr, &cubeMap.sampler);
		assert(!err);

		// Create image view
		VkImageViewCreateInfo view = vkTools::initializers::imageViewCreateInfo();
		view.image = VK_NULL_HANDLE;
		view.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
		view.format = format;
		view.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		view.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		view.subresourceRange.layerCount = 6;
		view.image = cubeMap.image;
		err = vkCreateImageView(m_pFramework->device, &view, nullptr, &cubeMap.view);
		assert(!err);

		// Cleanup
		for (auto& face : cubeFace)
		{
			vkDestroyImage(m_pFramework->device, face.image, nullptr);
			vkFreeMemory(m_pFramework->device, face.memory, nullptr);
		}
	}

	void buildCommandBuffers()
	{
		VkCommandBufferBeginInfo cmdBufInfo = vkTools::initializers::commandBufferBeginInfo();

		VkClearValue clearValues[2];
		clearValues[0].color = m_pFramework->defaultClearColor;
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderPassBeginInfo = vkTools::initializers::renderPassBeginInfo();
		renderPassBeginInfo.renderPass = m_pFramework->renderPass;
		renderPassBeginInfo.renderArea.offset.x = 0;
		renderPassBeginInfo.renderArea.offset.y = 0;
		renderPassBeginInfo.renderArea.extent.width  = m_pFramework->ScreenRect.Width;
		renderPassBeginInfo.renderArea.extent.height = m_pFramework->ScreenRect.Height;
		renderPassBeginInfo.clearValueCount = 2;
		renderPassBeginInfo.pClearValues = clearValues;

		VkResult err;

		for (int32_t i = 0; i < m_pFramework->drawCmdBuffers.size(); ++i)
		{
			// Set target frame buffer
			renderPassBeginInfo.framebuffer = m_pFramework->frameBuffers[i];

			err = vkBeginCommandBuffer(m_pFramework->drawCmdBuffers[i], &cmdBufInfo);
			assert(!err);

			vkCmdBeginRenderPass(m_pFramework->drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport viewport = vkTools::initializers::viewport(
				(float)m_pFramework->ScreenRect.Width,
				(float)m_pFramework->ScreenRect.Height,
				0.0f,
				1.0f);
			vkCmdSetViewport(m_pFramework->drawCmdBuffers[i], 0, 1, &viewport);

			VkRect2D scissor = vkTools::initializers::rect2D(
				m_pFramework->ScreenRect.Width,
				m_pFramework->ScreenRect.Height,
				0,
				0);
			vkCmdSetScissor(m_pFramework->drawCmdBuffers[i], 0, 1, &scissor);

			VkDeviceSize offsets[1] = { 0 };

			// Skybox
			vkCmdBindDescriptorSets(m_pFramework->drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets.skybox, 0, NULL);
			vkCmdBindVertexBuffers(m_pFramework->drawCmdBuffers[i], VERTEX_BUFFER_BIND_ID, 1, &meshes.skybox.vertices.buf, offsets);
			vkCmdBindIndexBuffer(m_pFramework->drawCmdBuffers[i], meshes.skybox.indices.buf, 0, VK_INDEX_TYPE_UINT32);
			vkCmdBindPipeline(m_pFramework->drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.skybox);
			vkCmdDrawIndexed(m_pFramework->drawCmdBuffers[i], meshes.skybox.indexCount, 1, 0, 0, 0);

			// 3D object
			vkCmdBindDescriptorSets(m_pFramework->drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets.object, 0, NULL);
			vkCmdBindVertexBuffers(m_pFramework->drawCmdBuffers[i], VERTEX_BUFFER_BIND_ID, 1, &meshes.object.vertices.buf, offsets);
			vkCmdBindIndexBuffer(m_pFramework->drawCmdBuffers[i], meshes.object.indices.buf, 0, VK_INDEX_TYPE_UINT32);
			vkCmdBindPipeline(m_pFramework->drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.reflect);
			vkCmdDrawIndexed(m_pFramework->drawCmdBuffers[i], meshes.object.indexCount, 1, 0, 0, 0);

			vkCmdEndRenderPass(m_pFramework->drawCmdBuffers[i]);

			err = vkEndCommandBuffer(m_pFramework->drawCmdBuffers[i]);
			assert(!err);
		}
	}

	void draw()
	{
		VkResult err;

		// Get next image in the swap chain (back/front buffer)
		err = m_pFramework->swapChain.acquireNextImage(m_pFramework->semaphores.presentComplete, &m_pFramework->currentBuffer);
		assert(!err);

		m_pFramework->submitPostPresentBarrier(m_pFramework->swapChain.buffers[m_pFramework->currentBuffer].image);

		// Command buffer to be sumitted to the queue
		m_pFramework->submitInfo.commandBufferCount = 1;
		m_pFramework->submitInfo.pCommandBuffers = &m_pFramework->drawCmdBuffers[m_pFramework->currentBuffer];

		// Submit to queue
		err = vkQueueSubmit(m_pFramework->queue, 1, &m_pFramework->submitInfo, VK_NULL_HANDLE);
		assert(!err);

		m_pFramework->submitPrePresentBarrier(m_pFramework->swapChain.buffers[m_pFramework->currentBuffer].image);

		err = m_pFramework->swapChain.queuePresent(m_pFramework->queue, m_pFramework->currentBuffer, m_pFramework->semaphores.renderComplete);
		assert(!err);

		err = vkQueueWaitIdle(m_pFramework->queue);
		assert(!err);
	}

	void loadMeshes()
	{
		m_pFramework->loadMesh(m_pFramework->getAssetPath() + "models/sphere.obj", &meshes.object, vertexLayout, 0.05f);
		m_pFramework->loadMesh(m_pFramework->getAssetPath() + "models/cube.obj", &meshes.skybox, vertexLayout, 0.05f);
	}

	void setupVertexDescriptions()
	{
		// Binding description
		vertices.bindingDescriptions.resize(1);
		vertices.bindingDescriptions[0] =
			vkTools::initializers::vertexInputBindingDescription(
				VERTEX_BUFFER_BIND_ID,
				vkMeshLoader::vertexSize(vertexLayout),
				VK_VERTEX_INPUT_RATE_VERTEX);

		// Attribute descriptions
		// Describes memory layout and shader positions
		vertices.attributeDescriptions.resize(3);
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
				sizeof(float) * 5);

		vertices.inputState = vkTools::initializers::pipelineVertexInputStateCreateInfo();
		vertices.inputState.vertexBindingDescriptionCount = (uint32_t)vertices.bindingDescriptions.size();
		vertices.inputState.pVertexBindingDescriptions = vertices.bindingDescriptions.data();
		vertices.inputState.vertexAttributeDescriptionCount = (uint32_t)vertices.attributeDescriptions.size();
		vertices.inputState.pVertexAttributeDescriptions = vertices.attributeDescriptions.data();
	}

	void setupDescriptorPool()
	{
		std::vector<VkDescriptorPoolSize> poolSizes =
		{
			vkTools::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2),
			vkTools::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2)
		};

		VkDescriptorPoolCreateInfo descriptorPoolInfo = 
			vkTools::initializers::descriptorPoolCreateInfo(
				(uint32_t)poolSizes.size(),
				poolSizes.data(),
				2);

		VkResult vkRes = vkCreateDescriptorPool(m_pFramework->device, &descriptorPoolInfo, nullptr, &m_pFramework->descriptorPool);
		assert(!vkRes);
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
			// Binding 1 : Fragment shader image sampler
			vkTools::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 
				VK_SHADER_STAGE_FRAGMENT_BIT, 
				1)
		};

		VkDescriptorSetLayoutCreateInfo descriptorLayout = 
			vkTools::initializers::descriptorSetLayoutCreateInfo(
				setLayoutBindings.data(),
				(uint32_t)setLayoutBindings.size());

		VkResult err = vkCreateDescriptorSetLayout(m_pFramework->device, &descriptorLayout, nullptr, &descriptorSetLayout);
		assert(!err);

		VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo =
			vkTools::initializers::pipelineLayoutCreateInfo(
				&descriptorSetLayout,
				1);

		err = vkCreatePipelineLayout(m_pFramework->device, &pPipelineLayoutCreateInfo, nullptr, &pipelineLayout);
		assert(!err);
	}

	void setupDescriptorSets()
	{
		// Image descriptor for the cube map texture
		VkDescriptorImageInfo cubeMapDescriptor =
			vkTools::initializers::descriptorImageInfo(
				cubeMap.sampler,
				cubeMap.view,
				VK_IMAGE_LAYOUT_GENERAL);

		VkDescriptorSetAllocateInfo allocInfo =
			vkTools::initializers::descriptorSetAllocateInfo(
				m_pFramework->descriptorPool,
				&descriptorSetLayout,
				1);

		// 3D object descriptor set
		VkResult vkRes = vkAllocateDescriptorSets(m_pFramework->device, &allocInfo, &descriptorSets.object);
		assert(!vkRes);

		std::vector<VkWriteDescriptorSet> writeDescriptorSets =
		{
			// Binding 0 : Vertex shader uniform buffer
			vkTools::initializers::writeDescriptorSet(
				descriptorSets.object, 
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 
				0, 
				&uniformData.objectVS.descriptor),
			// Binding 1 : Fragment shader cubemap sampler
			vkTools::initializers::writeDescriptorSet(
				descriptorSets.object, 
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 
				1, 
				&cubeMapDescriptor)
		};
		vkUpdateDescriptorSets(m_pFramework->device, (uint32_t)writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);

		// Sky box descriptor set
		vkRes = vkAllocateDescriptorSets(m_pFramework->device, &allocInfo, &descriptorSets.skybox);
		assert(!vkRes);

		writeDescriptorSets =
		{
			// Binding 0 : Vertex shader uniform buffer
			vkTools::initializers::writeDescriptorSet(
				descriptorSets.skybox,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				0,
				&uniformData.skyboxVS.descriptor),
			// Binding 1 : Fragment shader cubemap sampler
			vkTools::initializers::writeDescriptorSet(
				descriptorSets.skybox,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				1,
				&cubeMapDescriptor)
		};
		vkUpdateDescriptorSets(m_pFramework->device, (uint32_t)writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);
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
				VK_FALSE,
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
				(uint32_t)dynamicStateEnables.size(),
				0);

		// Skybox pipeline (background cube)
		std::array<VkPipelineShaderStageCreateInfo,2> shaderStages;

		shaderStages[0] = m_pFramework->loadShader(m_pFramework->getAssetPath() + "shaders/cubemap/skybox.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = m_pFramework->loadShader(m_pFramework->getAssetPath() + "shaders/cubemap/skybox.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

		VkGraphicsPipelineCreateInfo pipelineCreateInfo =
			vkTools::initializers::pipelineCreateInfo(
				pipelineLayout,
				m_pFramework->renderPass,
				0);

		pipelineCreateInfo.pVertexInputState = &vertices.inputState;
		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
		pipelineCreateInfo.pRasterizationState = &rasterizationState;
		pipelineCreateInfo.pColorBlendState = &colorBlendState;
		pipelineCreateInfo.pMultisampleState = &multisampleState;
		pipelineCreateInfo.pViewportState = &viewportState;
		pipelineCreateInfo.pDepthStencilState = &depthStencilState;
		pipelineCreateInfo.pDynamicState = &dynamicState;
		pipelineCreateInfo.stageCount = (uint32_t)shaderStages.size();
		pipelineCreateInfo.pStages = shaderStages.data();

		VkResult err = vkCreateGraphicsPipelines(m_pFramework->device, m_pFramework->pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.skybox);
		assert(!err);

		// Cube map reflect pipeline
		shaderStages[0] = m_pFramework->loadShader(m_pFramework->getAssetPath() + "shaders/cubemap/reflect.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = m_pFramework->loadShader(m_pFramework->getAssetPath() + "shaders/cubemap/reflect.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		depthStencilState.depthWriteEnable = VK_TRUE;
		err = vkCreateGraphicsPipelines(m_pFramework->device, m_pFramework->pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.reflect);
		assert(!err);
	}

	// Prepare and initialize uniform buffer containing shader uniforms
	void prepareUniformBuffers()
	{
		// 3D objact 
		m_pFramework->createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			sizeof(uboVS),
			&uboVS,
			&uniformData.objectVS.buffer,
			&uniformData.objectVS.memory,
			&uniformData.objectVS.descriptor);

		// Skybox
		m_pFramework->createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			sizeof(uboVS),
			&uboVS,
			&uniformData.skyboxVS.buffer,
			&uniformData.skyboxVS.memory,
			&uniformData.skyboxVS.descriptor);
	}

	void updateUniformBuffers()
	{
		// 3D object
		glm::mat4 viewMatrix = glm::mat4();

		uboVS.projection = glm::perspective(deg_to_rad(60.0f), (float)m_pFramework->ScreenRect.Width / (float)m_pFramework->ScreenRect.Height, 0.001f, 256.0f);
		viewMatrix = glm::translate(viewMatrix, glm::vec3(0.0f, 0.0f, m_pFramework->zoom));

		uboVS.model = glm::mat4();
		uboVS.model = viewMatrix * glm::translate(uboVS.model, glm::vec3(0, 0, 0));
		uboVS.model = glm::rotate(uboVS.model, glm::radians(m_pFramework->rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		uboVS.model = glm::rotate(uboVS.model, glm::radians(m_pFramework->rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		uboVS.model = glm::rotate(uboVS.model, glm::radians(m_pFramework->rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

		uint8_t *pData;
		VkResult err = vkMapMemory(m_pFramework->device, uniformData.objectVS.memory, 0, sizeof(uboVS), 0, (void **)&pData);
		assert(!err);
		memcpy(pData, &uboVS, sizeof(uboVS));
		vkUnmapMemory(m_pFramework->device, uniformData.objectVS.memory);

		// Skysphere
		viewMatrix = glm::mat4();
		uboVS.projection = glm::perspective(deg_to_rad(60.0f), (float)m_pFramework->ScreenRect.Width / (float)m_pFramework->ScreenRect.Height, 0.001f, 256.0f);

		uboVS.model = glm::mat4();
		uboVS.model = viewMatrix * glm::translate(uboVS.model, glm::vec3(0, 0, 0));
		uboVS.model = glm::rotate(uboVS.model, glm::radians(m_pFramework->rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		uboVS.model = glm::rotate(uboVS.model, glm::radians(m_pFramework->rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		uboVS.model = glm::rotate(uboVS.model, glm::radians(m_pFramework->rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

		err = vkMapMemory(m_pFramework->device, uniformData.skyboxVS.memory, 0, sizeof(uboVS), 0, (void **)&pData);
		assert(!err);
		memcpy(pData, &uboVS, sizeof(uboVS));
		vkUnmapMemory(m_pFramework->device, uniformData.skyboxVS.memory);
	}

	int32_t	prepare()
	{
		//CVulkanFramework::prepare();
		loadMeshes();
		setupVertexDescriptions();
		prepareUniformBuffers();
		loadCubemap(
			m_pFramework->getAssetPath() + "textures/cubemap_yokohama.ktx",
			VK_FORMAT_BC3_UNORM_BLOCK,
			false);
		setupDescriptorSetLayout();
		preparePipelines();
		setupDescriptorPool();
		setupDescriptorSets();
		buildCommandBuffers();
		m_pFramework->prepared = true;
		return 0;
	}

	virtual int32_t render()
	{
		if (!m_pFramework->prepared)
			return 1;
		vkDeviceWaitIdle(m_pFramework->device);
		draw();
		vkDeviceWaitIdle(m_pFramework->device);
		updateUniformBuffers();
		return 0;
	}

	virtual void viewChanged()
	{
		updateUniformBuffers();
	}

	virtual void keyPressed(uint32_t keyCode)
	{
	}
};

DEFINE_VULKAN_GAME_CREATE_AND_RELEASE_FUNCTIONS()
