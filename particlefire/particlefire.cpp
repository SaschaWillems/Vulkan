/*
* Vulkan Example - CPU based fire particle system 
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
#include <glm/gtc/matrix_inverse.hpp>

#include <vulkan/vulkan.h>
#include "vulkanexamplebase.h"

#define VERTEX_BUFFER_BIND_ID 0

#define PARTICLE_COUNT 512
#define PARTICLE_SIZE 10.0f

#define FLAME_RADIUS 8.0f

#define PARTICLE_TYPE_FLAME 0
#define PARTICLE_TYPE_SMOKE 1

struct Particle {
	glm::vec4 pos;
	glm::vec4 color;
	float alpha;
	float size;
	float rotation;
	uint32_t type;
	// Attributes not used in shader
	glm::vec4 vel;
	float rotationSpeed;
};

// Vertex layout for this example
std::vector<vkMeshLoader::VertexLayout> vertexLayout =
{
	vkMeshLoader::VERTEX_LAYOUT_POSITION,
	vkMeshLoader::VERTEX_LAYOUT_UV,
	vkMeshLoader::VERTEX_LAYOUT_NORMAL,
	vkMeshLoader::VERTEX_LAYOUT_TANGENT,
	vkMeshLoader::VERTEX_LAYOUT_BITANGENT
};

class VulkanExample : public CBaseVulkanGame
{
public:
	virtual int32_t			init(CVulkanFramework* pFramework)
	{
		CBaseVulkanGame::init(pFramework);
		m_pFramework->zoom = -90.0f;
		m_pFramework->rotation = { -15.0f, 45.0f, 0.0f };
		m_pFramework->title = "Vulkan Example - Particle system";
		m_pFramework->zoomSpeed *= 1.5f;
		m_pFramework->timerSpeed *= 8.0f;
		srand(time(NULL));
		// Values not set here are initialized in the base class constructor
		return 0;
	};

	struct {
		struct {
			vkTools::VulkanTexture smoke;
			vkTools::VulkanTexture fire;
			// We use a custom sampler to change some sampler
			// attributes required for rotation the uv coordinates
			// inside the shader for alpha blended textures
			VkSampler sampler;
		} particles;
		struct {
			vkTools::VulkanTexture colorMap;
			vkTools::VulkanTexture normalMap;
		} floor;
	} textures;

	struct {
		vkMeshLoader::Mesh environment;
	} meshes;

	glm::vec3 emitterPos = glm::vec3(0.0f, -FLAME_RADIUS + 2.0f, 0.0f);
	glm::vec3 minVel = glm::vec3(-3.0f, 0.5f, -3.0f);
	glm::vec3 maxVel = glm::vec3(3.0f, 7.0f, 3.0f);

	struct {
		VkBuffer buffer;
		VkDeviceMemory memory;
		// Store the mapped address of the particle data for reuse
		void *mappedMemory;
		// Size of the particle buffer in bytes
		size_t size;
		VkPipelineVertexInputStateCreateInfo inputState;
		std::vector<VkVertexInputBindingDescription> bindingDescriptions;
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
	} particles;

	struct {
		vkTools::UniformData fire;
		vkTools::UniformData environment;
	} uniformData;

	struct {
		glm::mat4 projection;
		glm::mat4 model;
		glm::vec2 viewportDim;
		float pointSize = PARTICLE_SIZE;
	} uboVS;

	struct {
		glm::mat4 projection;
		glm::mat4 model;
		glm::mat4 normal;
		glm::vec4 lightPos = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
		glm::vec4 cameraPos;
	} uboEnv;

	struct {
		VkPipeline particles;
		VkPipeline environment;
	} pipelines;

	VkPipelineLayout pipelineLayout;
	VkDescriptorSet descriptorSet;
	VkDescriptorSetLayout descriptorSetLayout;

	std::vector<Particle> particleBuffer;

	VulkanExample()
	{
	}

	virtual ~VulkanExample()
	{
		// Clean up used Vulkan resources 
		// Note : Inherited destructor cleans up resources stored in base class

		m_pFramework->textureLoader->destroyTexture(textures.particles.smoke);
		m_pFramework->textureLoader->destroyTexture(textures.particles.fire);
		m_pFramework->textureLoader->destroyTexture(textures.floor.colorMap);
		m_pFramework->textureLoader->destroyTexture(textures.floor.normalMap);

		vkDestroyPipeline(m_pFramework->device, pipelines.particles, nullptr);
		vkDestroyPipeline(m_pFramework->device, pipelines.environment, nullptr);

		vkDestroyPipelineLayout(m_pFramework->device, pipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(m_pFramework->device, descriptorSetLayout, nullptr);

		vkUnmapMemory(m_pFramework->device, particles.memory);
		vkDestroyBuffer(m_pFramework->device, particles.buffer, nullptr);
		vkFreeMemory(m_pFramework->device, particles.memory, nullptr);

		vkDestroyBuffer(m_pFramework->device, uniformData.fire.buffer, nullptr);
		vkFreeMemory(m_pFramework->device, uniformData.fire.memory, nullptr);

		vkMeshLoader::freeMeshBufferResources(m_pFramework->device, &meshes.environment.buffers);

		vkDestroySampler(m_pFramework->device, textures.particles.sampler, nullptr);
	}

	void buildCommandBuffers()
	{
		VkCommandBufferBeginInfo cmdBufInfo = vkTools::initializers::commandBufferBeginInfo();

		VkClearValue clearValues[2];
		clearValues[0].color = m_pFramework->defaultClearColor;
		clearValues[0].color = { {0.0f, 0.0f, 0.0f, 0.0f} };
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderPassBeginInfo = vkTools::initializers::renderPassBeginInfo();
		renderPassBeginInfo.renderPass = m_pFramework->renderPass;
		renderPassBeginInfo.renderArea.offset.x = 0;
		renderPassBeginInfo.renderArea.offset.y = 0;
		renderPassBeginInfo.renderArea.extent.width  = (uint32_t)m_pFramework->ScreenRect.Width;
		renderPassBeginInfo.renderArea.extent.height = (uint32_t)m_pFramework->ScreenRect.Height;
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

			// Environment
			meshes.environment.drawIndexed(m_pFramework->drawCmdBuffers[i]);

			// Particle system
			vkCmdBindDescriptorSets(m_pFramework->drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);
			vkCmdBindPipeline(m_pFramework->drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.particles);
			VkDeviceSize offsets[1] = { 0 };
			vkCmdBindVertexBuffers(m_pFramework->drawCmdBuffers[i], VERTEX_BUFFER_BIND_ID, 1, &particles.buffer, offsets);
			vkCmdDraw(m_pFramework->drawCmdBuffers[i], PARTICLE_COUNT, 1, 0, 0);

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

	float rnd(float range)
	{
		return range * (rand() / double(RAND_MAX));
	}

	void initParticle(Particle *particle, glm::vec3 emitterPos)
	{
		particle->vel = glm::vec4(0.0f, minVel.y + rnd(maxVel.y - minVel.y), 0.0f, 0.0f);
		particle->alpha = rnd(0.75f);
		particle->size = 1.0f + rnd(0.5f);
		particle->color = glm::vec4(1.0f);
		particle->type = PARTICLE_TYPE_FLAME;
		particle->rotation = rnd(2.0f * M_PI);
		particle->rotationSpeed = rnd(2.0f) - rnd(2.0f);

		// Get random sphere point
		float theta = rnd(2 * M_PI);
		float phi = rnd(M_PI) - M_PI / 2;
		float r = rnd(FLAME_RADIUS);

		particle->pos.x = r * cos(theta) * cos(phi);
		particle->pos.y = r * sin(phi);
		particle->pos.z = r * sin(theta) * cos(phi);

		particle->pos += glm::vec4(emitterPos, 0.0f);
	}

	void transitionParticle(Particle *particle)
	{
		switch (particle->type)
		{
		case PARTICLE_TYPE_FLAME:
			// Flame particles have a chance of turning into smoke
			if (rnd(1.0f) < 0.05f)
			{
				particle->alpha = 0.0f;
				particle->color = glm::vec4(0.25f + rnd(0.25f));
				particle->pos.x *= 0.5f;
				particle->pos.z *= 0.5f;
				particle->vel = glm::vec4(rnd(1.0f) - rnd(1.0f), (minVel.y * 2) + rnd(maxVel.y - minVel.y), rnd(1.0f) - rnd(1.0f), 0.0f);
				particle->size = 1.0f + rnd(0.5f);
				particle->rotationSpeed = rnd(1.0f) - rnd(1.0f);
				particle->type = PARTICLE_TYPE_SMOKE;
			}
			else
			{
				initParticle(particle, emitterPos);
			}
			break;
		case PARTICLE_TYPE_SMOKE:
			// Respawn at end of life
			initParticle(particle, emitterPos);
			break;
		}
	}

	void prepareParticles()
	{
		particleBuffer.resize(PARTICLE_COUNT);
		for (auto& particle : particleBuffer)
		{
			initParticle(&particle, emitterPos);
			particle.alpha = 1.0f - (abs(particle.pos.y) / (FLAME_RADIUS * 2.0f));
		}

		particles.size = particleBuffer.size() * sizeof(Particle);

		m_pFramework->createBuffer(
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			particles.size,
			particleBuffer.data(),
			&particles.buffer,
			&particles.memory);

		// Map the memory and store the pointer for reuse
		VkResult err = vkMapMemory(m_pFramework->device, particles.memory, 0, particles.size, 0, &particles.mappedMemory);
		assert(!err);
	}

	void updateParticles()
	{
		float particleTimer = m_pFramework->frameTimer * 0.45f;
		for (auto& particle : particleBuffer)
		{
			switch (particle.type)
			{
			case PARTICLE_TYPE_FLAME:
				particle.pos.y -= particle.vel.y * particleTimer * 3.5f;
				particle.alpha += particleTimer * 2.5f;
				particle.size -= particleTimer * 0.5f;
				break;
			case PARTICLE_TYPE_SMOKE:
				particle.pos -= particle.vel * m_pFramework->frameTimer * 1.0f;
				particle.alpha += particleTimer * 1.25f;
				particle.size += particleTimer * 0.125f;
				particle.color -= particleTimer * 0.05f;
				break;
			}
			particle.rotation += particleTimer * particle.rotationSpeed;
			// Transition particle state
			if (particle.alpha > 2.0f)
			{
				transitionParticle(&particle);
			}
		}
		size_t size = particleBuffer.size() * sizeof(Particle);
		memcpy(particles.mappedMemory, particleBuffer.data(), size);
	}

	void loadTextures()
	{
		// Particles
		m_pFramework->textureLoader->loadTexture(
			m_pFramework->getAssetPath() + "textures/particle_smoke.ktx",
			VK_FORMAT_BC3_UNORM_BLOCK,
			&textures.particles.smoke);
		m_pFramework->textureLoader->loadTexture(
			m_pFramework->getAssetPath() + "textures/particle_fire.ktx",
			VK_FORMAT_BC3_UNORM_BLOCK,
			&textures.particles.fire);

		// Floor
		m_pFramework->textureLoader->loadTexture(
			m_pFramework->getAssetPath() + "textures/fireplace_colormap_bc3.ktx",
			VK_FORMAT_BC3_UNORM_BLOCK,
			&textures.floor.colorMap);
		m_pFramework->textureLoader->loadTexture(
			m_pFramework->getAssetPath() + "textures/fireplace_normalmap_bc3.ktx",
			VK_FORMAT_BC3_UNORM_BLOCK,
			&textures.floor.normalMap);

		// Create a custom sampler to be used with the particle textures
		// Create sampler
		VkSamplerCreateInfo samplerCreateInfo = vkTools::initializers::samplerCreateInfo();
		samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		// Different address mode
		samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		samplerCreateInfo.addressModeV = samplerCreateInfo.addressModeU;
		samplerCreateInfo.addressModeW = samplerCreateInfo.addressModeU;
		samplerCreateInfo.mipLodBias = 0.0f;
		samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
		samplerCreateInfo.minLod = 0.0f;
		// Both particle textures have the same number of mip maps
		samplerCreateInfo.maxLod = textures.particles.fire.mipLevels;
		// Enable anisotropic filtering
		samplerCreateInfo.maxAnisotropy = 8;
		samplerCreateInfo.anisotropyEnable = VK_TRUE;
		// Use a different border color (than the normal texture loader) for additive blending
		samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
		VkResult err = vkCreateSampler(m_pFramework->device, &samplerCreateInfo, nullptr, &textures.particles.sampler);
		assert(!err);
	}

	void loadMeshes()
	{
		m_pFramework->loadMesh(m_pFramework->getAssetPath() + "models/fireplace.obj", &meshes.environment.buffers, vertexLayout, 10.0f);
		meshes.environment.setupVertexInputState(vertexLayout);
	}

	void setupVertexDescriptions()
	{
		// Binding description
		particles.bindingDescriptions.resize(1);
		particles.bindingDescriptions[0] =
			vkTools::initializers::vertexInputBindingDescription(
				VERTEX_BUFFER_BIND_ID,
				sizeof(Particle),
				VK_VERTEX_INPUT_RATE_VERTEX);

		// Attribute descriptions
		// Describes memory layout and shader positions
		// Location 0 : Position
		particles.attributeDescriptions.push_back(
			vkTools::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				0,
				VK_FORMAT_R32G32B32A32_SFLOAT,
				0));
		// Location 1 : Color
		particles.attributeDescriptions.push_back(
			vkTools::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				1,
				VK_FORMAT_R32G32B32A32_SFLOAT,
				sizeof(float) * 4));
		// Location 2 : Alpha
		particles.attributeDescriptions.push_back(
			vkTools::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				2,
				VK_FORMAT_R32_SFLOAT,
				sizeof(float) * 8));
		// Location 3 : Size
		particles.attributeDescriptions.push_back(
			vkTools::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				3,
				VK_FORMAT_R32_SFLOAT,
				sizeof(float) * 9));
		// Location 4 : Rotation
		particles.attributeDescriptions.push_back(
			vkTools::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				4,
				VK_FORMAT_R32_SFLOAT,
				sizeof(float) * 10));
		// Location 5 : Type
		particles.attributeDescriptions.push_back(
			vkTools::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				5,
				VK_FORMAT_R32_SINT,
				sizeof(float) * 11));

		particles.inputState = vkTools::initializers::pipelineVertexInputStateCreateInfo();
		particles.inputState.vertexBindingDescriptionCount = particles.bindingDescriptions.size();
		particles.inputState.pVertexBindingDescriptions = particles.bindingDescriptions.data();
		particles.inputState.vertexAttributeDescriptionCount = particles.attributeDescriptions.size();
		particles.inputState.pVertexAttributeDescriptions = particles.attributeDescriptions.data();
	}

	void setupDescriptorPool()
	{
		// Example uses one ubo and one image sampler
		std::vector<VkDescriptorPoolSize> poolSizes =
		{
			vkTools::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2),
			vkTools::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4)
		};

		VkDescriptorPoolCreateInfo descriptorPoolInfo =
			vkTools::initializers::descriptorPoolCreateInfo(
				poolSizes.size(),
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
				1),
			// Binding 1 : Fragment shader image sampler
			vkTools::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_SHADER_STAGE_FRAGMENT_BIT,
				2)
		};

		VkDescriptorSetLayoutCreateInfo descriptorLayout =
			vkTools::initializers::descriptorSetLayoutCreateInfo(
				setLayoutBindings.data(),
				setLayoutBindings.size());

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
		VkDescriptorSetAllocateInfo allocInfo =
			vkTools::initializers::descriptorSetAllocateInfo(
				m_pFramework->descriptorPool,
				&descriptorSetLayout,
				1);

		VkResult vkRes = vkAllocateDescriptorSets(m_pFramework->device, &allocInfo, &descriptorSet);
		assert(!vkRes);

		// Image descriptor for the color map texture
		VkDescriptorImageInfo texDescriptorSmoke =
			vkTools::initializers::descriptorImageInfo(
				textures.particles.sampler,
				textures.particles.smoke.view,
				VK_IMAGE_LAYOUT_GENERAL);
		VkDescriptorImageInfo texDescriptorFire =
			vkTools::initializers::descriptorImageInfo(
				textures.particles.sampler,
				textures.particles.fire.view,
				VK_IMAGE_LAYOUT_GENERAL);

		std::vector<VkWriteDescriptorSet> writeDescriptorSets =
		{
			// Binding 0 : Vertex shader uniform buffer
			vkTools::initializers::writeDescriptorSet(
			descriptorSet,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				0,
				&uniformData.fire.descriptor),
			// Binding 1 : Smoke texture
			vkTools::initializers::writeDescriptorSet(
				descriptorSet,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				1,
				&texDescriptorSmoke),
			// Binding 1 : Fire texture array
			vkTools::initializers::writeDescriptorSet(
				descriptorSet,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				2,
				&texDescriptorFire)
		};

		vkUpdateDescriptorSets(m_pFramework->device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);

		// Environment
		vkRes = vkAllocateDescriptorSets(m_pFramework->device, &allocInfo, &meshes.environment.descriptorSet);
		assert(!vkRes);

		VkDescriptorImageInfo texDescriptorColorMap =
			vkTools::initializers::descriptorImageInfo(
				textures.floor.colorMap.sampler,
				textures.floor.colorMap.view,
				VK_IMAGE_LAYOUT_GENERAL);
		VkDescriptorImageInfo texDescriptorNormalMap =
			vkTools::initializers::descriptorImageInfo(
				textures.floor.normalMap.sampler,
				textures.floor.normalMap.view,
				VK_IMAGE_LAYOUT_GENERAL);

		writeDescriptorSets.clear();

		// Binding 0 : Vertex shader uniform buffer
		writeDescriptorSets.push_back(
			vkTools::initializers::writeDescriptorSet(
				meshes.environment.descriptorSet,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				0,
				&uniformData.environment.descriptor));
		// Binding 1 : Color map
		writeDescriptorSets.push_back(
			vkTools::initializers::writeDescriptorSet(
				meshes.environment.descriptorSet,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				1,
				&texDescriptorColorMap));
		// Binding 2 : Normal map
		writeDescriptorSets.push_back(
			vkTools::initializers::writeDescriptorSet(
				meshes.environment.descriptorSet,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				2,
				&texDescriptorNormalMap));

		vkUpdateDescriptorSets(m_pFramework->device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);
	}

	void preparePipelines()
	{
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
			vkTools::initializers::pipelineInputAssemblyStateCreateInfo(
				VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
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

		// Load shaders
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

		shaderStages[0] = m_pFramework->loadShader(m_pFramework->getAssetPath() + "shaders/particlefire/particle.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = m_pFramework->loadShader(m_pFramework->getAssetPath() + "shaders/particlefire/particle.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

		VkGraphicsPipelineCreateInfo pipelineCreateInfo =
			vkTools::initializers::pipelineCreateInfo(
				pipelineLayout,
				m_pFramework->renderPass,
				0);

		pipelineCreateInfo.pVertexInputState = &particles.inputState;
		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
		pipelineCreateInfo.pRasterizationState = &rasterizationState;
		pipelineCreateInfo.pColorBlendState = &colorBlendState;
		pipelineCreateInfo.pMultisampleState = &multisampleState;
		pipelineCreateInfo.pViewportState = &viewportState;
		pipelineCreateInfo.pDepthStencilState = &depthStencilState;
		pipelineCreateInfo.pDynamicState = &dynamicState;
		pipelineCreateInfo.stageCount = shaderStages.size();
		pipelineCreateInfo.pStages = shaderStages.data();

		depthStencilState.depthWriteEnable = VK_FALSE;

		// Premulitplied alpha
		blendAttachmentState.blendEnable = VK_TRUE;
		blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
		blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
		blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		VkResult err = vkCreateGraphicsPipelines(m_pFramework->device, m_pFramework->pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.particles);
		assert(!err);

		// Environment rendering pipeline (normal mapped)
		shaderStages[0] = m_pFramework->loadShader(m_pFramework->getAssetPath() + "shaders/particlefire/normalmap.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = m_pFramework->loadShader(m_pFramework->getAssetPath() + "shaders/particlefire/normalmap.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		pipelineCreateInfo.pVertexInputState = &meshes.environment.vertexInputState;
		blendAttachmentState.blendEnable = VK_FALSE;
		depthStencilState.depthWriteEnable = VK_TRUE;
		inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		err = vkCreateGraphicsPipelines(m_pFramework->device, m_pFramework->pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.environment);
		assert(!err);
		meshes.environment.pipeline = pipelines.environment;
		meshes.environment.pipelineLayout = pipelineLayout;
	}

	// Prepare and initialize uniform buffer containing shader uniforms
	void prepareUniformBuffers()
	{
		// Vertex shader uniform buffer block
		m_pFramework->createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			sizeof(uboVS),
			&uboVS,
			&uniformData.fire.buffer,
			&uniformData.fire.memory,
			&uniformData.fire.descriptor);

		// Vertex shader uniform buffer block
		m_pFramework->createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			sizeof(uboEnv),
			&uboEnv,
			&uniformData.environment.buffer,
			&uniformData.environment.memory,
			&uniformData.environment.descriptor);

		updateUniformBuffers();
	}

	void updateUniformBufferLight()
	{
		// Environment
		uboEnv.lightPos.x = sin(m_pFramework->timer * 2 * M_PI) * 1.5f;
		uboEnv.lightPos.y = 0.0f;
		uboEnv.lightPos.z = cos(m_pFramework->timer * 2 * M_PI) * 1.5f;
		uint8_t *pData;
		VkResult err = vkMapMemory(m_pFramework->device, uniformData.environment.memory, 0, sizeof(uboEnv), 0, (void **)&pData);
		assert(!err);
		memcpy(pData, &uboEnv, sizeof(uboEnv));
		vkUnmapMemory(m_pFramework->device, uniformData.environment.memory);
	}

	void updateUniformBuffers()
	{
		// Vertex shader
		glm::mat4 viewMatrix = glm::mat4();
		uboVS.projection = glm::perspective(glm::radians(60.0f), (float)m_pFramework->ScreenRect.Width / (float)m_pFramework->ScreenRect.Height, 0.001f, 256.0f);
		viewMatrix = glm::translate(viewMatrix, glm::vec3(0.0f, 0.0f, m_pFramework->zoom));

		uboVS.model = glm::mat4();
		uboVS.model = viewMatrix * glm::translate(uboVS.model, glm::vec3(0.0f, 15.0f, 0.0f));
		uboVS.model = glm::rotate(uboVS.model, glm::radians(m_pFramework->rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		uboVS.model = glm::rotate(uboVS.model, glm::radians(m_pFramework->rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		uboVS.model = glm::rotate(uboVS.model, glm::radians(m_pFramework->rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

		uboVS.viewportDim = glm::vec2((float)m_pFramework->ScreenRect.Width, (float)m_pFramework->ScreenRect.Height);
		
		uint8_t *pData;
		VkResult err = vkMapMemory(m_pFramework->device, uniformData.fire.memory, 0, sizeof(uboVS), 0, (void **)&pData);
		assert(!err);
		memcpy(pData, &uboVS, sizeof(uboVS));
		vkUnmapMemory(m_pFramework->device, uniformData.fire.memory);

		// Environment
		uboEnv.projection = uboVS.projection;
		uboEnv.model = uboVS.model;
		uboEnv.normal = glm::inverseTranspose(uboEnv.model);
		uboEnv.cameraPos = glm::vec4(0.0, 0.0, m_pFramework->zoom, 0.0);
		err = vkMapMemory(m_pFramework->device, uniformData.environment.memory, 0, sizeof(uboEnv), 0, (void **)&pData);
		assert(!err);
		memcpy(pData, &uboEnv, sizeof(uboEnv));
		vkUnmapMemory(m_pFramework->device, uniformData.environment.memory);
	}

	virtual int32_t	prepare()
	{
		CBaseVulkanGame::prepare();
		loadTextures();
		prepareParticles();
		setupVertexDescriptions();
		prepareUniformBuffers();
		setupDescriptorSetLayout();
		loadMeshes();
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
		if (!m_pFramework->paused)
		{
			updateUniformBufferLight();
			updateParticles();
		}
		return 0;
	}

	virtual void viewChanged()
	{
		updateUniformBuffers();
	}

	virtual void	keyPressed(uint32_t keyCode)
	{
	}

};

DEFINE_VULKAN_GAME_CREATE_AND_RELEASE_FUNCTIONS()