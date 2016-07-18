/*
* Vulkan Example - Deferred shading with shadows from multiple light sources using geometry shader instancing
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
#include <glm/gtx/rotate_vector.hpp>

#include <vulkan/vulkan.h>
#include "vulkanexamplebase.h"
#include "vulkanframebuffer.hpp"

#define VERTEX_BUFFER_BIND_ID 0
#define ENABLE_VALIDATION false

// Shadowmap properties
#if defined(__ANDROID__)
#define SHADOWMAP_DIM 1024
#else
#define SHADOWMAP_DIM 2048
#endif
// 16 bits of depth is enough for such a small scene
#define SHADOWMAP_FORMAT VK_FORMAT_D32_SFLOAT_S8_UINT

#if defined(__ANDROID__)
// Use max. screen dimension as deferred framebuffer size
#define FB_DIM std::max(width,height) 
#else
#define FB_DIM 2048
#endif

// Must match the LIGHT_COUNT define in the shadow and deferred shaders
#define LIGHT_COUNT 3

// Vertex layout for this example
// todo: create class for vertex layout
std::vector<vkMeshLoader::VertexLayout> vertexLayout =
{
	vkMeshLoader::VERTEX_LAYOUT_POSITION,
	vkMeshLoader::VERTEX_LAYOUT_UV,
	vkMeshLoader::VERTEX_LAYOUT_COLOR,
	vkMeshLoader::VERTEX_LAYOUT_NORMAL,
	vkMeshLoader::VERTEX_LAYOUT_TANGENT
};

class VulkanExample : public VulkanExampleBase
{
public:
	bool debugDisplay = false;
	bool enableShadows = true;

	// Keep depth range as small as possible
	// for better shadow map precision
	float zNear = 0.1f;
	float zFar = 64.0f;
	float lightFOV = 100.0f;

	// Depth bias (and slope) are used to avoid shadowing artefacts
	float depthBiasConstant = 1.25f;
	float depthBiasSlope = 1.75f;

	struct {
		struct {
			vkTools::VulkanTexture colorMap;
			vkTools::VulkanTexture normalMap;
		} model;
		struct {
			vkTools::VulkanTexture colorMap;
			vkTools::VulkanTexture normalMap;
		} background;
	} textures;
	
	struct {
		vkMeshLoader::MeshBuffer model;
		vkMeshLoader::MeshBuffer background;
		vkMeshLoader::MeshBuffer quad;
	} meshes;

	struct {
		VkPipelineVertexInputStateCreateInfo inputState;
		std::vector<VkVertexInputBindingDescription> bindingDescriptions;
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
	} vertices;

	struct {
		glm::mat4 projection;
		glm::mat4 model;
		glm::mat4 view;
		glm::vec4 instancePos[3];
		int layer;
	} uboVS, uboOffscreenVS;

	// This UBO stores the shadow matrices for all of the light sources
	// The matrices are indexed using geometry shader instancing
	// The instancePos is used to place the models using instanced draws
	struct {
		glm::mat4 mvp[LIGHT_COUNT];
		glm::vec4 instancePos[3];
	} uboShadowGS;

	struct Light {
		glm::vec4 position;
		glm::vec4 target;
		glm::vec4 color;
		glm::mat4 viewMatrix;
	};

	struct {
		glm::vec4 viewPos;
		Light lights[LIGHT_COUNT];
		uint32_t useShadows = 1;
	} uboFragmentLights;

	struct {
		vkTools::UniformData vsFullScreen;
		vkTools::UniformData vsOffscreen;
		vkTools::UniformData fsLights;
		vkTools::UniformData uboShadowGS;
	} uniformData;

	struct {
		VkPipeline deferred;
		VkPipeline offscreen;
		VkPipeline debug;
		VkPipeline shadowpass;
	} pipelines;

	struct {
		//todo: rename, shared with deferred and shadow pass
		VkPipelineLayout deferred;
		VkPipelineLayout offscreen;
	} pipelineLayouts;

	struct {
		VkDescriptorSet model;
		VkDescriptorSet background;
		VkDescriptorSet shadow;
	} descriptorSets;

	VkDescriptorSet descriptorSet;
	VkDescriptorSetLayout descriptorSetLayout;

	struct
	{
		// Framebuffer resources for the deferred pass
		vk::Framebuffer *deferred;
		// Framebuffer resources for the shadow pass
		vk::Framebuffer *shadow;
	} frameBuffers;

	struct {
		VkCommandBuffer deferred = VK_NULL_HANDLE;
	} commandBuffers;

	// Semaphore used to synchronize between offscreen and final scene rendering
	VkSemaphore offscreenSemaphore = VK_NULL_HANDLE;

	// Device features to be enabled for this example 
	static VkPhysicalDeviceFeatures getEnabledFeatures()
	{
		VkPhysicalDeviceFeatures enabledFeatures = {};
		enabledFeatures.geometryShader = VK_TRUE;
		enabledFeatures.shaderClipDistance = VK_TRUE;
		enabledFeatures.shaderCullDistance = VK_TRUE;
		enabledFeatures.shaderTessellationAndGeometryPointSize = VK_TRUE;
		return enabledFeatures;
	}

	VulkanExample() : VulkanExampleBase(ENABLE_VALIDATION, getEnabledFeatures)
	{
		enableTextOverlay = true;
		title = "Vulkan Example - Deferred shading with shadows (2016 by Sascha Willems)";
		camera.type = Camera::CameraType::firstperson;
#if defined(__ANDROID__)
		camera.movementSpeed = 2.5f;
#else
		camera.movementSpeed = 5.0f;
		camera.rotationSpeed = 0.25f;
#endif
		camera.position = { 2.15f, 0.3f, -8.75f };
		camera.setRotation(glm::vec3(-0.75f, 12.5f, 0.0f));
		camera.setPerspective(60.0f, (float)width / (float)height, zNear, zFar);
		timerSpeed *= 0.25f;
		paused = true;
	}

	~VulkanExample()
	{
		// Frame buffers
		if (frameBuffers.deferred)
		{
			delete frameBuffers.deferred;
		}
		if (frameBuffers.shadow)
		{
			delete frameBuffers.shadow;
		}

		vkDestroyPipeline(device, pipelines.deferred, nullptr);
		vkDestroyPipeline(device, pipelines.offscreen, nullptr);
		vkDestroyPipeline(device, pipelines.shadowpass, nullptr);
		vkDestroyPipeline(device, pipelines.debug, nullptr);

		vkDestroyPipelineLayout(device, pipelineLayouts.deferred, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayouts.offscreen, nullptr);

		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

		// Meshes
		vkMeshLoader::freeMeshBufferResources(device, &meshes.model);
		vkMeshLoader::freeMeshBufferResources(device, &meshes.background);
		vkMeshLoader::freeMeshBufferResources(device, &meshes.quad);

		// Uniform buffers
		vkTools::destroyUniformData(device, &uniformData.vsOffscreen);
		vkTools::destroyUniformData(device, &uniformData.vsFullScreen);
		vkTools::destroyUniformData(device, &uniformData.fsLights);
		vkTools::destroyUniformData(device, &uniformData.uboShadowGS);

		vkFreeCommandBuffers(device, cmdPool, 1, &commandBuffers.deferred);

		// Textures
		textureLoader->destroyTexture(textures.model.colorMap);
		textureLoader->destroyTexture(textures.model.normalMap);
		textureLoader->destroyTexture(textures.background.colorMap);
		textureLoader->destroyTexture(textures.background.normalMap);

		vkDestroySemaphore(device, offscreenSemaphore, nullptr);
	}

	// Prepare a layered shadow map with each layer containing depth from a light's point of view
	// The shadow mapping pass uses geometry shader instancing to output the scene from the different
	// light sources' point of view to the layers of the depth attachment in one single pass 
	void shadowSetup()
	{
		VkCommandBuffer layoutCmd = VulkanExampleBase::createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

		frameBuffers.shadow = new vk::Framebuffer(&vulkanDevice);

		frameBuffers.shadow->width = SHADOWMAP_DIM;
		frameBuffers.shadow->height = SHADOWMAP_DIM;

		// Create a layered depth attachment for rendering the depth maps from the lights' point of view
		// Each layer corresponds to one of the lights
		// The actual output to the separate layers is done in the geometry shader using shader instancing
		// We will pass the matrices of the lights to the GS that selects the layer by the current invocation
		vk::AttachmentCreateInfo framebufferInfo = {};
		framebufferInfo.format = SHADOWMAP_FORMAT;
		framebufferInfo.width = SHADOWMAP_DIM;
		framebufferInfo.height = SHADOWMAP_DIM;
		framebufferInfo.layerCount = LIGHT_COUNT;
		framebufferInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		frameBuffers.shadow->addAttachment(framebufferInfo, layoutCmd);

		VulkanExampleBase::flushCommandBuffer(layoutCmd, queue, true);

		// Create sampler to sample from to depth attachment 
		// Used to sample in the fragment shader for shadowed rendering
		VK_CHECK_RESULT(frameBuffers.shadow->createSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE));

		// Create default renderpass for the framebuffer
		VK_CHECK_RESULT(frameBuffers.shadow->createRenderPass());
	}

	// Prepare the framebuffer for offscreen rendering with multiple attachments used as render targets inside the fragment shaders
	void deferredSetup()
	{
		VkCommandBuffer layoutCmd = VulkanExampleBase::createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

		frameBuffers.deferred = new vk::Framebuffer(&vulkanDevice);

		frameBuffers.deferred->width = FB_DIM;
		frameBuffers.deferred->height = FB_DIM;

		// Four attachments (3 color, 1 depth)
		vk::AttachmentCreateInfo framebufferInfo = {};
		framebufferInfo.width = FB_DIM;
		framebufferInfo.height = FB_DIM;
		framebufferInfo.layerCount = 1;
		framebufferInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		// Color attachments
		// Attachment 0: (World space) Positions
		framebufferInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
		frameBuffers.deferred->addAttachment(framebufferInfo, layoutCmd);

		// Attachment 1: (World space) Normals
		framebufferInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
		frameBuffers.deferred->addAttachment(framebufferInfo, layoutCmd);

		// Attachment 2: Albedo (color)
		framebufferInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
		frameBuffers.deferred->addAttachment(framebufferInfo, layoutCmd);

		// Depth attachment
		// Find a suitable depth format
		VkFormat attDepthFormat;
		VkBool32 validDepthFormat = vkTools::getSupportedDepthFormat(physicalDevice, &attDepthFormat);
		assert(validDepthFormat);

		framebufferInfo.format = attDepthFormat;
		framebufferInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		frameBuffers.deferred->addAttachment(framebufferInfo, layoutCmd);

		VulkanExampleBase::flushCommandBuffer(layoutCmd, queue, true);

		// Create sampler to sample from the color attachments
		VK_CHECK_RESULT(frameBuffers.deferred->createSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE));

		// Create default renderpass for the framebuffer
		VK_CHECK_RESULT(frameBuffers.deferred->createRenderPass());
	}

	// Put render commands for the scene into the given command buffer
	void renderScene(VkCommandBuffer cmdBuffer, bool shadow)
	{
		VkDeviceSize offsets[1] = { 0 };

		// Background
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.offscreen, 0, 1, shadow ? &descriptorSets.shadow : &descriptorSets.background, 0, NULL);
		vkCmdBindVertexBuffers(cmdBuffer, VERTEX_BUFFER_BIND_ID, 1, &meshes.background.vertices.buf, offsets);
		vkCmdBindIndexBuffer(cmdBuffer, meshes.background.indices.buf, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(cmdBuffer, meshes.background.indexCount, 1, 0, 0, 0);

		// Objects
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.offscreen, 0, 1, shadow ? &descriptorSets.shadow : &descriptorSets.model, 0, NULL);
		vkCmdBindVertexBuffers(cmdBuffer, VERTEX_BUFFER_BIND_ID, 1, &meshes.model.vertices.buf, offsets);
		vkCmdBindIndexBuffer(cmdBuffer, meshes.model.indices.buf, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(cmdBuffer, meshes.model.indexCount, 3, 0, 0, 0);
	}

	// Build a secondary command buffer for rendering the scene values to the offscreen frame buffer attachments
	void buildDeferredCommandBuffer()
	{
		if (commandBuffers.deferred == VK_NULL_HANDLE)
		{
			commandBuffers.deferred = VulkanExampleBase::createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, false);
		}

		// Create a semaphore used to synchronize offscreen rendering and usage
		VkSemaphoreCreateInfo semaphoreCreateInfo = vkTools::initializers::semaphoreCreateInfo();
		VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &offscreenSemaphore));

		VkCommandBufferBeginInfo cmdBufInfo = vkTools::initializers::commandBufferBeginInfo();

		VkRenderPassBeginInfo renderPassBeginInfo = vkTools::initializers::renderPassBeginInfo();
		std::array<VkClearValue, 4> clearValues = {};
		VkViewport viewport;
		VkRect2D scissor;

		// Shadow map generation pass first
		
		clearValues[0].depthStencil = { 1.0f, 0 };

		renderPassBeginInfo.renderPass = frameBuffers.shadow->renderPass;
		renderPassBeginInfo.framebuffer = frameBuffers.shadow->framebuffer;
		renderPassBeginInfo.renderArea.extent.width = frameBuffers.shadow->width;
		renderPassBeginInfo.renderArea.extent.height = frameBuffers.shadow->height;
		renderPassBeginInfo.clearValueCount = 1;
		renderPassBeginInfo.pClearValues = clearValues.data();

		VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffers.deferred, &cmdBufInfo));

		// Change back layout of the depth attachment after sampling in the fragment shader
		// todo: replace with subpass dependency
		vkTools::setImageLayout(
			commandBuffers.deferred,
			frameBuffers.shadow->attachments[0].image,
			VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			frameBuffers.shadow->attachments[0].subresourceRange);

		viewport = vkTools::initializers::viewport((float)frameBuffers.shadow->width, (float)frameBuffers.shadow->height, 0.0f, 1.0f);
		vkCmdSetViewport(commandBuffers.deferred, 0, 1, &viewport);

		scissor = vkTools::initializers::rect2D(frameBuffers.shadow->width, frameBuffers.shadow->height, 0, 0);
		vkCmdSetScissor(commandBuffers.deferred, 0, 1, &scissor);

		// Set depth bias (aka "Polygon offset")
		vkCmdSetDepthBias(
			commandBuffers.deferred,
			depthBiasConstant,
			0.0f,
			depthBiasSlope);

		vkCmdBeginRenderPass(commandBuffers.deferred, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(commandBuffers.deferred, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.shadowpass);
		renderScene(commandBuffers.deferred, true);
		vkCmdEndRenderPass(commandBuffers.deferred);

		// Change layout of the depth attachment for sampling in the fragment shader
		// todo: replace with subpass dependency
		vkTools::setImageLayout(
			commandBuffers.deferred,
			frameBuffers.shadow->attachments[0].image,
			VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			frameBuffers.shadow->attachments[0].subresourceRange);

		// Deferred pass second 
		// -------------------------------------------------------------------------------------------------------

		// Change back layout of the color attachments after sampling in the fragment shader
		// todo: replace with subpass dependency
		for (auto attachment : frameBuffers.deferred->attachments)
		{
			if (!attachment.hasDepth())
			{
				vkTools::setImageLayout(
					commandBuffers.deferred,
					attachment.image,
					VK_IMAGE_ASPECT_COLOR_BIT,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
			}
		}

		// Clear values for all attachments written in the fragment sahder
		clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
		clearValues[1].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
		clearValues[2].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
		clearValues[3].depthStencil = { 1.0f, 0 };

		renderPassBeginInfo.renderPass = frameBuffers.deferred->renderPass;
		renderPassBeginInfo.framebuffer = frameBuffers.deferred->framebuffer;
		renderPassBeginInfo.renderArea.extent.width = frameBuffers.deferred->width;
		renderPassBeginInfo.renderArea.extent.height = frameBuffers.deferred->height;
		renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassBeginInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffers.deferred, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		viewport = vkTools::initializers::viewport((float)frameBuffers.deferred->width, (float)frameBuffers.deferred->height, 0.0f, 1.0f);
		vkCmdSetViewport(commandBuffers.deferred, 0, 1, &viewport);

		scissor = vkTools::initializers::rect2D(frameBuffers.deferred->width, frameBuffers.deferred->height, 0, 0);
		vkCmdSetScissor(commandBuffers.deferred, 0, 1, &scissor);

		vkCmdBindPipeline(commandBuffers.deferred, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.offscreen);
		renderScene(commandBuffers.deferred, false);
		vkCmdEndRenderPass(commandBuffers.deferred);

		// Change back layout of the color attachments after sampling in the fragment shader
		// todo: replace with subpass dependency
		for (auto attachment : frameBuffers.deferred->attachments)
		{
			if (!attachment.hasDepth())
			{
				vkTools::setImageLayout(
					commandBuffers.deferred,
					attachment.image,
					VK_IMAGE_ASPECT_COLOR_BIT,
					VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			}
		}

		VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffers.deferred));
	}

	void loadTextures()
	{
		textureLoader->loadTexture(getAssetPath() + "models/armor/colormap.ktx", VK_FORMAT_BC3_UNORM_BLOCK, &textures.model.colorMap);
		textureLoader->loadTexture(getAssetPath() + "models/armor/normalmap.ktx", VK_FORMAT_BC3_UNORM_BLOCK, &textures.model.normalMap);
		textureLoader->loadTexture(getAssetPath() + "textures/pattern_57_diffuse_bc3.ktx", VK_FORMAT_BC3_UNORM_BLOCK, &textures.background.colorMap);
		textureLoader->loadTexture(getAssetPath() + "textures/pattern_57_normal_bc3.ktx", VK_FORMAT_BC3_UNORM_BLOCK, &textures.background.normalMap);
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
		clearValues[0].color = { { 0.0f, 0.0f, 0.2f, 0.0f } };
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
			renderPassBeginInfo.framebuffer = VulkanExampleBase::frameBuffers[i];

			VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo));

			vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport viewport = vkTools::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
			vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

			VkRect2D scissor = vkTools::initializers::rect2D(width, height, 0, 0);
			vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

			VkDeviceSize offsets[1] = { 0 };
			vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.deferred, 0, 1, &descriptorSet, 0, NULL);

			// Final composition as full screen quad
			vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.deferred);
			vkCmdBindVertexBuffers(drawCmdBuffers[i], VERTEX_BUFFER_BIND_ID, 1, &meshes.quad.vertices.buf, offsets);
			vkCmdBindIndexBuffer(drawCmdBuffers[i], meshes.quad.indices.buf, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(drawCmdBuffers[i], 6, 1, 0, 0, 0);

			if (debugDisplay)
			{
				// Visualize depth maps
				vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.debug);
				vkCmdDrawIndexed(drawCmdBuffers[i], 6, LIGHT_COUNT, 0, 0, 0);
			}

			vkCmdEndRenderPass(drawCmdBuffers[i]);

			VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
		}
	}

	void loadMeshes()
	{
		loadMesh(getAssetPath() + "models/armor/armor.dae", &meshes.model, vertexLayout, 1.0f);

		vkMeshLoader::MeshCreateInfo meshCreateInfo;
		meshCreateInfo.scale = glm::vec3(15.0f);
		meshCreateInfo.uvscale = glm::vec2(1.0f, 1.5f);
		meshCreateInfo.center = glm::vec3(0.0f, 2.3f, 0.0f);
		loadMesh(getAssetPath() + "models/openbox.dae", &meshes.background, vertexLayout, &meshCreateInfo);
	}

	/** @brief Create a single quad for fullscreen deferred pass and debug passes (debug pass uses instancing for light visualization) */
	void generateQuads()
	{
		struct Vertex {
			float pos[3];
			float uv[2];
			float col[3];
			float normal[3];
			float tangent[3];
		};

		std::vector<Vertex> vertexBuffer;

		vertexBuffer.push_back({ { 1.0f, 1.0f, 0.0f },{ 1.0f, 1.0f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 0.0f, 0.0f } });
		vertexBuffer.push_back({ { 0.0f, 1.0f, 0.0f },{ 0.0f, 1.0f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 0.0f, 0.0f } });
		vertexBuffer.push_back({ { 0.0f, 0.0f, 0.0f },{ 0.0f, 0.0f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 0.0f, 0.0f } });
		vertexBuffer.push_back({ { 1.0f, 0.0f, 0.0f },{ 1.0f, 0.0f },{ 1.0f, 1.0f, 1.0f },{ 0.0f, 0.0f, 0.0f } });

		createBuffer(
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			vertexBuffer.size() * sizeof(Vertex),
			vertexBuffer.data(),
			&meshes.quad.vertices.buf,
			&meshes.quad.vertices.mem);

		// Setup indices
		std::vector<uint32_t> indexBuffer = { 0,1,2, 2,3,0 };
		for (uint32_t i = 0; i < 3; ++i)
		{
			uint32_t indices[6] = { 0,1,2, 2,3,0 };
			for (auto index : indices)
			{
				indexBuffer.push_back(i * 4 + index);
			}
		}
		meshes.quad.indexCount = static_cast<uint32_t>(indexBuffer.size());

		createBuffer(
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			indexBuffer.size() * sizeof(uint32_t),
			indexBuffer.data(),
			&meshes.quad.indices.buf,
			&meshes.quad.indices.mem);
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
		vertices.attributeDescriptions.clear();
		vkMeshLoader::getVertexInputAttributeDescriptions(
			vertexLayout, 
			vertices.attributeDescriptions, 
			VERTEX_BUFFER_BIND_ID);

		vertices.inputState = vkTools::initializers::pipelineVertexInputStateCreateInfo();
		vertices.inputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertices.bindingDescriptions.size());
		vertices.inputState.pVertexBindingDescriptions = vertices.bindingDescriptions.data();
		vertices.inputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertices.attributeDescriptions.size());
		vertices.inputState.pVertexAttributeDescriptions = vertices.attributeDescriptions.data();
	}

	void setupDescriptorPool()
	{
		std::vector<VkDescriptorPoolSize> poolSizes =
		{
			vkTools::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 12), //todo: separate set layouts
			vkTools::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 16)
		};

		VkDescriptorPoolCreateInfo descriptorPoolInfo =
			vkTools::initializers::descriptorPoolCreateInfo(
				static_cast<uint32_t>(poolSizes.size()),
				poolSizes.data(),
				4);

		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));
	}

	void setupDescriptorSetLayout()
	{
		// todo: split for clarity, esp. with GS instancing
		// Deferred shading layout (Shared with debug display)
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings =
		{
			// Binding 0: Vertex shader uniform buffer
			vkTools::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT,
				0),
			// Binding 1: Position texture
			vkTools::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_SHADER_STAGE_FRAGMENT_BIT,
				1),
			// Binding 2: Normals texture
			vkTools::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_SHADER_STAGE_FRAGMENT_BIT,
				2),
			// Binding 3: Albedo texture
			vkTools::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_SHADER_STAGE_FRAGMENT_BIT,
				3),
			// Binding 4: Fragment shader uniform buffer
			vkTools::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				VK_SHADER_STAGE_FRAGMENT_BIT,
				4),
			// Binding 5: Shadow map
			vkTools::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_SHADER_STAGE_FRAGMENT_BIT,
				5),
		};

		VkDescriptorSetLayoutCreateInfo descriptorLayout =
			vkTools::initializers::descriptorSetLayoutCreateInfo(
				setLayoutBindings.data(),
				static_cast<uint32_t>(setLayoutBindings.size()));

		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayout));

		VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo =
			vkTools::initializers::pipelineLayoutCreateInfo(
				&descriptorSetLayout,
				1);

		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &pipelineLayouts.deferred));

		// Offscreen (scene) rendering pipeline layout
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &pipelineLayouts.offscreen));
	}

	void setupDescriptorSet()
	{
		std::vector<VkWriteDescriptorSet> writeDescriptorSets;

		// Textured quad descriptor set
		VkDescriptorSetAllocateInfo allocInfo =
			vkTools::initializers::descriptorSetAllocateInfo(
				descriptorPool,
				&descriptorSetLayout,
				1);

		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));

		// Image descriptors for the offscreen color attachments
		VkDescriptorImageInfo texDescriptorPosition =
			vkTools::initializers::descriptorImageInfo(
				frameBuffers.deferred->sampler,
				frameBuffers.deferred->attachments[0].view,
				VK_IMAGE_LAYOUT_GENERAL);

		VkDescriptorImageInfo texDescriptorNormal =
			vkTools::initializers::descriptorImageInfo(
				frameBuffers.deferred->sampler,
				frameBuffers.deferred->attachments[1].view,
				VK_IMAGE_LAYOUT_GENERAL);

		VkDescriptorImageInfo texDescriptorAlbedo =
			vkTools::initializers::descriptorImageInfo(
				frameBuffers.deferred->sampler,
				frameBuffers.deferred->attachments[2].view,
				VK_IMAGE_LAYOUT_GENERAL);

		VkDescriptorImageInfo texDescriptorShadowMap =
			vkTools::initializers::descriptorImageInfo(
				frameBuffers.shadow->sampler,
				frameBuffers.shadow->attachments[0].view,
				VK_IMAGE_LAYOUT_GENERAL);

		writeDescriptorSets = {
			// Binding 0: Vertex shader uniform buffer
			vkTools::initializers::writeDescriptorSet(
				descriptorSet,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				0,
				&uniformData.vsFullScreen.descriptor),
			// Binding 1: World space position texture
			vkTools::initializers::writeDescriptorSet(
				descriptorSet,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				1,
				&texDescriptorPosition),
			// Binding 2: World space normals texture
			vkTools::initializers::writeDescriptorSet(
				descriptorSet,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				2,
				&texDescriptorNormal),
			// Binding 3: Albedo texture
			vkTools::initializers::writeDescriptorSet(
				descriptorSet,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				3,
				&texDescriptorAlbedo),
			// Binding 4: Fragment shader uniform buffer
			vkTools::initializers::writeDescriptorSet(
				descriptorSet,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				4,
				&uniformData.fsLights.descriptor),
			// Binding 5: Shadow map
			vkTools::initializers::writeDescriptorSet(
				descriptorSet,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				5,
				&texDescriptorShadowMap),
		};

		vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, NULL);

		// Offscreen (scene)

		// Model
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.model));
		writeDescriptorSets =
		{
			// Binding 0: Vertex shader uniform buffer
			vkTools::initializers::writeDescriptorSet(
			descriptorSets.model,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				0,
				&uniformData.vsOffscreen.descriptor),
			// Binding 1: Color map
			vkTools::initializers::writeDescriptorSet(
				descriptorSets.model,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				1,
				&textures.model.colorMap.descriptor),
			// Binding 2: Normal map
			vkTools::initializers::writeDescriptorSet(
				descriptorSets.model,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				2,
				&textures.model.normalMap.descriptor)
		};
		vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, NULL);

		// Background
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.background));
		writeDescriptorSets =
		{
			// Binding 0: Vertex shader uniform buffer
			vkTools::initializers::writeDescriptorSet(
				descriptorSets.background,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				0,
				&uniformData.vsOffscreen.descriptor),
			// Binding 1: Color map
			vkTools::initializers::writeDescriptorSet(
				descriptorSets.background,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				1,
				&textures.background.colorMap.descriptor),
			// Binding 2: Normal map
			vkTools::initializers::writeDescriptorSet(
				descriptorSets.background,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				2,
				&textures.background.normalMap.descriptor)
		};
		vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, NULL);

		// Shadow mapping
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.shadow));
		writeDescriptorSets =
		{
			// Binding 0: Vertex shader uniform buffer
			vkTools::initializers::writeDescriptorSet(
				descriptorSets.shadow,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				0,
				&uniformData.uboShadowGS.descriptor),
		};
		vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, NULL);
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
				static_cast<uint32_t>(dynamicStateEnables.size()),
				0);

		// Final fullscreen pass pipeline
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

		shaderStages[0] = loadShader(getAssetPath() + "shaders/deferredshadows/deferred.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getAssetPath() + "shaders/deferredshadows/deferred.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

		VkGraphicsPipelineCreateInfo pipelineCreateInfo =
			vkTools::initializers::pipelineCreateInfo(
				pipelineLayouts.deferred,
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
		pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineCreateInfo.pStages = shaderStages.data();

		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.deferred));

		// Debug display pipeline
		shaderStages[0] = loadShader(getAssetPath() + "shaders/deferredshadows/debug.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getAssetPath() + "shaders/deferredshadows/debug.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.debug));

		// Offscreen pipeline
		shaderStages[0] = loadShader(getAssetPath() + "shaders/deferredshadows/mrt.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getAssetPath() + "shaders/deferredshadows/mrt.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

		// Separate render pass
		pipelineCreateInfo.renderPass = frameBuffers.deferred->renderPass;

		// Separate layout
		pipelineCreateInfo.layout = pipelineLayouts.offscreen;

		// Blend attachment states required for all color attachments
		// This is important, as color write mask will otherwise be 0x0 and you
		// won't see anything rendered to the attachment
		std::array<VkPipelineColorBlendAttachmentState, 3> blendAttachmentStates =
		{
			vkTools::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
			vkTools::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
			vkTools::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE)
		};

		colorBlendState.attachmentCount = static_cast<uint32_t>(blendAttachmentStates.size());
		colorBlendState.pAttachments = blendAttachmentStates.data();

		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.offscreen));

		// Shadow mapping pipeline
		// The shadow mapping pipeline uses geometry shader instancing (invoctations layout modifier) to output 
		// shadow maps for multiple lights sources into the different shadiw map layers in one single render pass
		std::array<VkPipelineShaderStageCreateInfo, 3> shadowStages;
		shadowStages[0] = loadShader(getAssetPath() + "shaders/deferredshadows/shadow.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shadowStages[1] = loadShader(getAssetPath() + "shaders/deferredshadows/shadow.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		shadowStages[2] = loadShader(getAssetPath() + "shaders/deferredshadows/shadow.geom.spv", VK_SHADER_STAGE_GEOMETRY_BIT);

		pipelineCreateInfo.pStages = shadowStages.data();
		pipelineCreateInfo.stageCount = static_cast<uint32_t>(shadowStages.size());
		
		// Shadow pass doesn't use any color attachments
		colorBlendState.attachmentCount = 0;
		colorBlendState.pAttachments = nullptr;
		// Cull front faces
		rasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;
		depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		// Enable depth bias
		rasterizationState.depthBiasEnable = VK_TRUE;
		// Add depth bias to dynamic state, so we can change it at runtime
		dynamicStateEnables.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);
		dynamicState =
			vkTools::initializers::pipelineDynamicStateCreateInfo(
				dynamicStateEnables.data(),
				static_cast<uint32_t>(dynamicStateEnables.size()),
				0);
		// Reset blend attachment state
		pipelineCreateInfo.renderPass = frameBuffers.shadow->renderPass;
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.shadowpass));
	}

	// Prepare and initialize uniform buffer containing shader uniforms
	void prepareUniformBuffers()
	{
		// Fullscreen vertex shader
		createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			sizeof(uboVS),
			nullptr,
			&uniformData.vsFullScreen.buffer,
			&uniformData.vsFullScreen.memory,
			&uniformData.vsFullScreen.descriptor);

		// Deferred vertex shader
		createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			sizeof(uboOffscreenVS),
			nullptr,
			&uniformData.vsOffscreen.buffer,
			&uniformData.vsOffscreen.memory,
			&uniformData.vsOffscreen.descriptor);

		// Deferred fragment shader
		createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			sizeof(uboFragmentLights),
			nullptr,
			&uniformData.fsLights.buffer,
			&uniformData.fsLights.memory,
			&uniformData.fsLights.descriptor);

		// Shadow map vertex shader (matrices from shadow's pov)
		createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			sizeof(uboShadowGS),
			nullptr,
			&uniformData.uboShadowGS.buffer,
			&uniformData.uboShadowGS.memory,
			&uniformData.uboShadowGS.descriptor);

		// Init some values
		uboOffscreenVS.instancePos[0] = glm::vec4(0.0f);
		uboOffscreenVS.instancePos[1] = glm::vec4(-4.0f, 0.0, -4.0f, 0.0f);
		uboOffscreenVS.instancePos[2] = glm::vec4(4.0f, 0.0, -4.0f, 0.0f);

		uboOffscreenVS.instancePos[1] = glm::vec4(-7.0f, 0.0, -4.0f, 0.0f);
		uboOffscreenVS.instancePos[2] = glm::vec4(4.0f, 0.0, -6.0f, 0.0f);


		// Update
		updateUniformBuffersScreen();
		updateUniformBufferDeferredMatrices();
		updateUniformBufferDeferredLights();
	}

	void updateUniformBuffersScreen()
	{
		uboVS.projection = glm::ortho(0.0f, 1.0f, 0.0f, 1.0f, -1.0f, 1.0f);
		uboVS.model = glm::mat4();

		uint8_t *pData;
		VK_CHECK_RESULT(vkMapMemory(device, uniformData.vsFullScreen.memory, 0, sizeof(uboVS), 0, (void **)&pData));
		memcpy(pData, &uboVS, sizeof(uboVS));
		vkUnmapMemory(device, uniformData.vsFullScreen.memory);
	}

	void updateUniformBufferDeferredMatrices()
	{
		uboOffscreenVS.projection = camera.matrices.perspective;
		uboOffscreenVS.view = camera.matrices.view;
		uboOffscreenVS.model = glm::mat4();

		uint8_t *pData;
		VK_CHECK_RESULT(vkMapMemory(device, uniformData.vsOffscreen.memory, 0, sizeof(uboOffscreenVS), 0, (void **)&pData));
		memcpy(pData, &uboOffscreenVS, sizeof(uboOffscreenVS));
		vkUnmapMemory(device, uniformData.vsOffscreen.memory);
	}

	// Update fragment shader light position uniform block
	void updateUniformBufferDeferredLights()
	{
		std::vector<glm::vec4> lightPositions =
		{
			glm::vec4(-14.0f, -0.5f, 15.0f, 0.0f), 
			glm::vec4(14.0f, -4.0f, 12.0f, 0.0f),
			glm::vec4(0.0f, -10.0f, 4.0f, 0.0f)
		};
		std::vector<glm::vec4> lightColors =
		{
			glm::vec4(1.0f, 0.5f, 0.5f, 0.0f),
			glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
			glm::vec4(1.0f, 1.0f, 1.0f, 0.0f),
		};
		std::vector<glm::vec4> lightTargets =
		{
			glm::vec4(-2.0f, 0.0f, 0.0f, 0.0f),
			glm::vec4(2.0f, 0.0f, 0.0f, 0.0f),
			glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
		};

		// Animate
		if (!paused)
		{
			lightPositions[0].x = -14.0f + abs(sin(glm::radians(timer * 360.0f)) * 20.0f);
			lightPositions[0].z = 15.0f + cos(glm::radians(timer *360.0f)) * 1.0f;

			lightPositions[1].x = 14.0f - abs(sin(glm::radians(timer * 360.0f)) * 2.5f);
			lightPositions[1].z = 13.0f + cos(glm::radians(timer *360.0f)) * 4.0f;

			lightPositions[2].x = 0.0f + sin(glm::radians(timer *360.0f)) * 4.0f;
			lightPositions[2].z = 4.0f + cos(glm::radians(timer *360.0f)) * 2.0f;
		}

		for (uint32_t i = 0; i < static_cast<uint32_t>(lightPositions.size()); i++)
		{
			Light *light = &uboFragmentLights.lights[i];

			light->position = lightPositions[i];
			light->color = lightColors[i];
			light->target = lightTargets[i];

			// mvp from light's pov (for shadows)
			glm::mat4 shadowProj = glm::perspective(glm::radians(lightFOV), 1.0f, zNear, zFar);
			glm::mat4 shadowView = glm::lookAt(glm::vec3(light->position), glm::vec3(light->target), glm::vec3(0.0f, 1.0f, 0.0f));
			glm::mat4 shadowModel = glm::mat4();

			uboShadowGS.mvp[i] = shadowProj * shadowView * shadowModel;
			light->viewMatrix = uboShadowGS.mvp[i];
		}

		uint8_t *pData;

		memcpy(uboShadowGS.instancePos, uboOffscreenVS.instancePos, sizeof(uboOffscreenVS.instancePos));

		VK_CHECK_RESULT(vkMapMemory(device, uniformData.uboShadowGS.memory, 0, sizeof(uboShadowGS), 0, (void **)&pData));
		memcpy(pData, &uboShadowGS, sizeof(uboShadowGS));
		vkUnmapMemory(device, uniformData.uboShadowGS.memory);

		uboFragmentLights.viewPos = glm::vec4(uboOffscreenVS.view[3]);
		uboFragmentLights.viewPos = glm::vec4(camera.position, 0.0f) * glm::vec4(-1.0f, 1.0f, -1.0f, 1.0f);;
	
		VK_CHECK_RESULT(vkMapMemory(device, uniformData.fsLights.memory, 0, sizeof(uboFragmentLights), 0, (void **)&pData));
		memcpy(pData, &uboFragmentLights, sizeof(uboFragmentLights));
		vkUnmapMemory(device, uniformData.fsLights.memory);
	}

	void draw()
	{
		VulkanExampleBase::prepareFrame();

		// Offscreen rendering

		// Wait for swap chain presentation to finish
		submitInfo.pWaitSemaphores = &semaphores.presentComplete;
		// Signal ready with offscreen semaphore
		submitInfo.pSignalSemaphores = &offscreenSemaphore;

		// Submit work

		// Shadow map pass
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers.deferred;
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

		// Scene rendering

		// Wait for offscreen semaphore
		submitInfo.pWaitSemaphores = &offscreenSemaphore;
		// Signal ready with render complete semaphpre
		submitInfo.pSignalSemaphores = &semaphores.renderComplete;

		// Submit work
		submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

		VulkanExampleBase::submitFrame();
	}

	void prepare()
	{
		VulkanExampleBase::prepare();
		loadTextures();
		generateQuads();
		loadMeshes();
		setupVertexDescriptions();
		deferredSetup();
		shadowSetup();
		prepareUniformBuffers();
		setupDescriptorSetLayout();
		preparePipelines();
		setupDescriptorPool();
		setupDescriptorSet();
		buildCommandBuffers();
		buildDeferredCommandBuffer();
		prepared = true;
	}

	virtual void render()
	{
		if (!prepared)
			return;
		draw();
		//if (!paused)
			updateUniformBufferDeferredLights();
	}

	virtual void viewChanged()
	{
		updateUniformBufferDeferredMatrices();
	}

	void toggleDebugDisplay()
	{
		debugDisplay = !debugDisplay;
		reBuildCommandBuffers();
		updateUniformBuffersScreen();
	}

	void toggleShadows()
	{
		uboFragmentLights.useShadows = !uboFragmentLights.useShadows;
		updateUniformBufferDeferredLights();
	}

	virtual void keyPressed(uint32_t keyCode)
	{
		switch (keyCode)
		{
		case 0x70:
		case GAMEPAD_BUTTON_A:
			toggleDebugDisplay();
			updateTextOverlay();
			break;
		case 0x71:
		case GAMEPAD_BUTTON_X:
			toggleShadows();
			updateTextOverlay();
			break;
		}
	}

	virtual void getOverlayText(VulkanTextOverlay *textOverlay)
	{
#if defined(__ANDROID__)
		textOverlay->addText("Press \"Button A\" to toggle debug view", 5.0f, 85.0f, VulkanTextOverlay::alignLeft);
		textOverlay->addText("Press \"Button X\" to toggle shadows", 5.0f, 100.0f, VulkanTextOverlay::alignLeft);
#else
		textOverlay->addText("Press \"F1\" to toggle debug view", 5.0f, 85.0f, VulkanTextOverlay::alignLeft);
		textOverlay->addText("Press \"F2\" to toggle shadows", 5.0f, 100.0f, VulkanTextOverlay::alignLeft);
#endif
	}
};

VULKAN_EXAMPLE_MAIN()