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
#include <random>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include <vulkan/vulkan.h>
#include "vulkanexamplebase.h"
#include "VulkanBuffer.hpp"
#include "VulkanTexture.hpp"
#include "VulkanModel.hpp"

#define VERTEX_BUFFER_BIND_ID 0
#define ENABLE_VALIDATION false
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

class VulkanExample : public VulkanExampleBase
{
public:
	struct {
		struct {
			vks::Texture2D smoke;
			vks::Texture2D fire;
			// Use a custom sampler to change sampler attributes required for rotating the uvs in the shader for alpha blended textures
			VkSampler sampler;
		} particles;
		struct {
			vks::Texture2D colorMap;
			vks::Texture2D normalMap;
		} floor;
	} textures;

	// Vertex layout for the models
	vks::VertexLayout vertexLayout = vks::VertexLayout({
		vks::VERTEX_COMPONENT_POSITION,
		vks::VERTEX_COMPONENT_UV,
		vks::VERTEX_COMPONENT_NORMAL,
		vks::VERTEX_COMPONENT_TANGENT,
		vks::VERTEX_COMPONENT_BITANGENT,
	});

	struct {
		vks::Model environment;
	} models;

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
	} particles;

	struct {
		vks::Buffer fire;
		vks::Buffer environment;
	} uniformBuffers;

	struct UBOVS {
		glm::mat4 projection;
		glm::mat4 model;
		glm::vec2 viewportDim;
		float pointSize = PARTICLE_SIZE;
	} uboVS;

	struct UBOEnv {
		glm::mat4 projection;
		glm::mat4 model;
		glm::mat4 normal;
		glm::vec4 lightPos = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
	} uboEnv;

	struct {
		VkPipeline particles;
		VkPipeline environment;
	} pipelines;

	VkPipelineLayout pipelineLayout;
	VkDescriptorSetLayout descriptorSetLayout;

	struct {
		VkDescriptorSet particles;
		VkDescriptorSet environment;
	} descriptorSets;

	std::vector<Particle> particleBuffer;

	std::default_random_engine rndEngine;

	VulkanExample() : VulkanExampleBase(ENABLE_VALIDATION)
	{
		zoom = -75.0f;
		rotation = { -15.0f, 45.0f, 0.0f };
		title = "CPU based particle system";
		settings.overlay = true;
		zoomSpeed *= 1.5f;
		timerSpeed *= 8.0f;
		rndEngine.seed(benchmark.active ? 0 : (unsigned)time(nullptr));
	}

	~VulkanExample()
	{
		// Clean up used Vulkan resources 
		// Note : Inherited destructor cleans up resources stored in base class

		textures.particles.smoke.destroy();
		textures.particles.fire.destroy();
		textures.floor.colorMap.destroy();
		textures.floor.normalMap.destroy();

		vkDestroyPipeline(device, pipelines.particles, nullptr);
		vkDestroyPipeline(device, pipelines.environment, nullptr);

		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

		vkUnmapMemory(device, particles.memory);
		vkDestroyBuffer(device, particles.buffer, nullptr);
		vkFreeMemory(device, particles.memory, nullptr);

		uniformBuffers.environment.destroy();
		uniformBuffers.fire.destroy();

		models.environment.destroy();

		vkDestroySampler(device, textures.particles.sampler, nullptr);
	}

	void buildCommandBuffers()
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

		for (int32_t i = 0; i < drawCmdBuffers.size(); ++i)
		{
			// Set target frame buffer
			renderPassBeginInfo.framebuffer = frameBuffers[i];

			VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo));

			vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
			vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

			VkRect2D scissor = vks::initializers::rect2D(width, height, 0,0);
			vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

			VkDeviceSize offsets[1] = { 0 };

			// Environment
			vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets.environment, 0, NULL);
			vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.environment);
			vkCmdBindVertexBuffers(drawCmdBuffers[i], VERTEX_BUFFER_BIND_ID, 1, &models.environment.vertices.buffer, offsets);
			vkCmdBindIndexBuffer(drawCmdBuffers[i], models.environment.indices.buffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(drawCmdBuffers[i], models.environment.indexCount, 1, 0, 0, 0);

			// Particle system (no index buffer)
			vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets.particles, 0, NULL);
			vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.particles);
			vkCmdBindVertexBuffers(drawCmdBuffers[i], VERTEX_BUFFER_BIND_ID, 1, &particles.buffer, offsets);
			vkCmdDraw(drawCmdBuffers[i], PARTICLE_COUNT, 1, 0, 0);

			drawUI(drawCmdBuffers[i]);

			vkCmdEndRenderPass(drawCmdBuffers[i]);

			VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
		}
	}

	float rnd(float range)
	{
		std::uniform_real_distribution<float> rndDist(0.0f, range);
		return rndDist(rndEngine);
	}

	void initParticle(Particle *particle, glm::vec3 emitterPos)
	{
		particle->vel = glm::vec4(0.0f, minVel.y + rnd(maxVel.y - minVel.y), 0.0f, 0.0f);
		particle->alpha = rnd(0.75f);
		particle->size = 1.0f + rnd(0.5f);
		particle->color = glm::vec4(1.0f);
		particle->type = PARTICLE_TYPE_FLAME;
		particle->rotation = rnd(2.0f * float(M_PI));
		particle->rotationSpeed = rnd(2.0f) - rnd(2.0f);

		// Get random sphere point
		float theta = rnd(2.0f * float(M_PI));
		float phi = rnd(float(M_PI)) - float(M_PI) / 2.0f;
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

		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			particles.size,
			&particles.buffer,
			&particles.memory,
			particleBuffer.data()));

		// Map the memory and store the pointer for reuse
		VK_CHECK_RESULT(vkMapMemory(device, particles.memory, 0, particles.size, 0, &particles.mappedMemory));
	}

	void updateParticles()
	{
		float particleTimer = frameTimer * 0.45f;
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
				particle.pos -= particle.vel * frameTimer * 1.0f;
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

	void loadAssets()
	{
		// Textures
		std::string texFormatSuffix;
		VkFormat texFormat;
		// Get supported compressed texture format
		if (vulkanDevice->features.textureCompressionBC) {
			texFormatSuffix = "_bc3_unorm";
			texFormat = VK_FORMAT_BC3_UNORM_BLOCK;
		}
		else if (vulkanDevice->features.textureCompressionASTC_LDR) {
			texFormatSuffix = "_astc_8x8_unorm";
			texFormat = VK_FORMAT_ASTC_8x8_UNORM_BLOCK;
		}
		else if (vulkanDevice->features.textureCompressionETC2) {
			texFormatSuffix = "_etc2_unorm";
			texFormat = VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
		}
		else {
			vks::tools::exitFatal("Device does not support any compressed texture format!", VK_ERROR_FEATURE_NOT_PRESENT);
		}

		// Particles
		textures.particles.smoke.loadFromFile(getAssetPath() + "textures/particle_smoke.ktx", VK_FORMAT_R8G8B8A8_UNORM, vulkanDevice, queue);
		textures.particles.fire.loadFromFile(getAssetPath() + "textures/particle_fire.ktx", VK_FORMAT_R8G8B8A8_UNORM, vulkanDevice, queue);

		// Floor
		textures.floor.colorMap.loadFromFile(getAssetPath() + "textures/fireplace_colormap" + texFormatSuffix + ".ktx", texFormat, vulkanDevice, queue);
		textures.floor.normalMap.loadFromFile(getAssetPath() + "textures/fireplace_normalmap" + texFormatSuffix + ".ktx", texFormat, vulkanDevice, queue);

		// Create a custom sampler to be used with the particle textures
		// Create sampler
		VkSamplerCreateInfo samplerCreateInfo = vks::initializers::samplerCreateInfo();
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
		samplerCreateInfo.maxLod = float(textures.particles.fire.mipLevels);
		// Enable anisotropic filtering
		samplerCreateInfo.maxAnisotropy = 8.0f;
		samplerCreateInfo.anisotropyEnable = VK_TRUE;
		// Use a different border color (than the normal texture loader) for additive blending
		samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
		VK_CHECK_RESULT(vkCreateSampler(device, &samplerCreateInfo, nullptr, &textures.particles.sampler));

		models.environment.loadFromFile(getAssetPath() + "models/fireplace.obj", vertexLayout, 10.0f, vulkanDevice, queue);
	}

	void setupDescriptorPool()
	{
		// Example uses one ubo and one image sampler
		std::vector<VkDescriptorPoolSize> poolSizes =
		{
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2),
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4)
		};

		VkDescriptorPoolCreateInfo descriptorPoolInfo =
			vks::initializers::descriptorPoolCreateInfo(
				poolSizes.size(),
				poolSizes.data(),
				2);

		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));
	}

	void setupDescriptorSetLayout()
	{
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings =
		{
			// Binding 0 : Vertex shader uniform buffer
			vks::initializers::descriptorSetLayoutBinding(
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				VK_SHADER_STAGE_VERTEX_BIT,
				0),
			// Binding 1 : Fragment shader image sampler
			vks::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_SHADER_STAGE_FRAGMENT_BIT,
				1),
			// Binding 1 : Fragment shader image sampler
			vks::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_SHADER_STAGE_FRAGMENT_BIT,
				2)
		};

		VkDescriptorSetLayoutCreateInfo descriptorLayout =
			vks::initializers::descriptorSetLayoutCreateInfo(
				setLayoutBindings.data(),
				setLayoutBindings.size());

		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayout));

		VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo =
			vks::initializers::pipelineLayoutCreateInfo(
				&descriptorSetLayout,
				1);

		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &pipelineLayout));
	}

	void setupDescriptorSets()
	{
		std::vector<VkWriteDescriptorSet> writeDescriptorSets;

		VkDescriptorSetAllocateInfo allocInfo =
			vks::initializers::descriptorSetAllocateInfo(
				descriptorPool,
				&descriptorSetLayout,
				1);

		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.particles));

		// Image descriptor for the color map texture
		VkDescriptorImageInfo texDescriptorSmoke =
			vks::initializers::descriptorImageInfo(
				textures.particles.sampler,
				textures.particles.smoke.view,
				VK_IMAGE_LAYOUT_GENERAL);
		VkDescriptorImageInfo texDescriptorFire =
			vks::initializers::descriptorImageInfo(
				textures.particles.sampler,
				textures.particles.fire.view,
				VK_IMAGE_LAYOUT_GENERAL);

		writeDescriptorSets = {
			// Binding 0: Vertex shader uniform buffer
			vks::initializers::writeDescriptorSet(
				descriptorSets.particles,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				0,
				&uniformBuffers.fire.descriptor),
			// Binding 1: Smoke texture
			vks::initializers::writeDescriptorSet(
				descriptorSets.particles,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				1,
				&texDescriptorSmoke),
			// Binding 1: Fire texture array
			vks::initializers::writeDescriptorSet(
				descriptorSets.particles,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				2,
				&texDescriptorFire)
		};

		vkUpdateDescriptorSets(device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);

		// Environment
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.environment));

		writeDescriptorSets = {
			// Binding 0: Vertex shader uniform buffer
			vks::initializers::writeDescriptorSet(
				descriptorSets.environment,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				0,
				&uniformBuffers.environment.descriptor),
			// Binding 1: Color map
			vks::initializers::writeDescriptorSet(
				descriptorSets.environment,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				1,
				&textures.floor.colorMap.descriptor),
			// Binding 2: Normal map
			vks::initializers::writeDescriptorSet(
				descriptorSets.environment,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				2,
				&textures.floor.normalMap.descriptor),
		};

		vkUpdateDescriptorSets(device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);
	}

	void preparePipelines()
	{
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
			vks::initializers::pipelineInputAssemblyStateCreateInfo(
				VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
				0,
				VK_FALSE);

		VkPipelineRasterizationStateCreateInfo rasterizationState =
			vks::initializers::pipelineRasterizationStateCreateInfo(
				VK_POLYGON_MODE_FILL,
				VK_CULL_MODE_BACK_BIT,
				VK_FRONT_FACE_CLOCKWISE,
				0);

		VkPipelineColorBlendAttachmentState blendAttachmentState =
			vks::initializers::pipelineColorBlendAttachmentState(
				0xf,
				VK_FALSE);

		VkPipelineColorBlendStateCreateInfo colorBlendState =
			vks::initializers::pipelineColorBlendStateCreateInfo(
				1,
				&blendAttachmentState);

		VkPipelineDepthStencilStateCreateInfo depthStencilState =
			vks::initializers::pipelineDepthStencilStateCreateInfo(
				VK_TRUE,
				VK_TRUE,
				VK_COMPARE_OP_LESS_OR_EQUAL);

		VkPipelineViewportStateCreateInfo viewportState =
			vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);

		VkPipelineMultisampleStateCreateInfo multisampleState =
			vks::initializers::pipelineMultisampleStateCreateInfo(
				VK_SAMPLE_COUNT_1_BIT,
				0);

		std::vector<VkDynamicState> dynamicStateEnables = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};
		VkPipelineDynamicStateCreateInfo dynamicState =
			vks::initializers::pipelineDynamicStateCreateInfo(
				dynamicStateEnables.data(),
				dynamicStateEnables.size(),
				0);

		// Load shaders
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

		VkGraphicsPipelineCreateInfo pipelineCreateInfo =
			vks::initializers::pipelineCreateInfo(
				pipelineLayout,
				renderPass,
				0);

		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
		pipelineCreateInfo.pRasterizationState = &rasterizationState;
		pipelineCreateInfo.pColorBlendState = &colorBlendState;
		pipelineCreateInfo.pMultisampleState = &multisampleState;
		pipelineCreateInfo.pViewportState = &viewportState;
		pipelineCreateInfo.pDepthStencilState = &depthStencilState;
		pipelineCreateInfo.pDynamicState = &dynamicState;
		pipelineCreateInfo.stageCount = shaderStages.size();
		pipelineCreateInfo.pStages = shaderStages.data();

		// Particle rendering pipeline
		{
			// Shaders
			shaderStages[0] = loadShader(getAssetPath() + "shaders/particlefire/particle.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
			shaderStages[1] = loadShader(getAssetPath() + "shaders/particlefire/particle.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

			// Vertex input state
			VkVertexInputBindingDescription vertexInputBinding =
				vks::initializers::vertexInputBindingDescription(VERTEX_BUFFER_BIND_ID, sizeof(Particle), VK_VERTEX_INPUT_RATE_VERTEX);

			std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = {
				vks::initializers::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 0, VK_FORMAT_R32G32B32A32_SFLOAT,	offsetof(Particle, pos)),	// Location 0: Position
				vks::initializers::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 1, VK_FORMAT_R32G32B32A32_SFLOAT,	offsetof(Particle, color)),	// Location 1: Color
				vks::initializers::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 2, VK_FORMAT_R32_SFLOAT, offsetof(Particle, alpha)),			// Location 2: Alpha			
				vks::initializers::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 3, VK_FORMAT_R32_SFLOAT, offsetof(Particle, size)),			// Location 3: Size
				vks::initializers::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 4, VK_FORMAT_R32_SFLOAT, offsetof(Particle, rotation)),		// Location 4: Rotation
				vks::initializers::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 5, VK_FORMAT_R32_SINT, offsetof(Particle, type)),				// Location 5: Particle type
			};

			VkPipelineVertexInputStateCreateInfo vertexInputState = vks::initializers::pipelineVertexInputStateCreateInfo();
			vertexInputState.vertexBindingDescriptionCount = 1;
			vertexInputState.pVertexBindingDescriptions = &vertexInputBinding;
			vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
			vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();

			pipelineCreateInfo.pVertexInputState = &vertexInputState;

			// Dont' write to depth buffer
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

			VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.particles));
		}

		// Environment rendering pipeline (normal mapped)
		{
			// Shaders
			shaderStages[0] = loadShader(getAssetPath() + "shaders/particlefire/normalmap.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
			shaderStages[1] = loadShader(getAssetPath() + "shaders/particlefire/normalmap.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

			// Vertex input state
			VkVertexInputBindingDescription vertexInputBinding =
				vks::initializers::vertexInputBindingDescription(VERTEX_BUFFER_BIND_ID, vertexLayout.stride(), VK_VERTEX_INPUT_RATE_VERTEX);

			std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = {
				vks::initializers::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 0, VK_FORMAT_R32G32B32_SFLOAT, 0),							// Location 0: Position
				vks::initializers::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 1, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 3),				// Location 1: UV
				vks::initializers::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 2, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 5),			// Location 2: Normal	
				vks::initializers::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 3, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 8),			// Location 3: Tangent
				vks::initializers::vertexInputAttributeDescription(VERTEX_BUFFER_BIND_ID, 4, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 11),			// Location 4: Bitangen
			};

			VkPipelineVertexInputStateCreateInfo vertexInputState = vks::initializers::pipelineVertexInputStateCreateInfo();
			vertexInputState.vertexBindingDescriptionCount = 1;
			vertexInputState.pVertexBindingDescriptions = &vertexInputBinding;
			vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
			vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();

			pipelineCreateInfo.pVertexInputState = &vertexInputState;

			blendAttachmentState.blendEnable = VK_FALSE;
			depthStencilState.depthWriteEnable = VK_TRUE;
			inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

			VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.environment));
		}
	}

	// Prepare and initialize uniform buffer containing shader uniforms
	void prepareUniformBuffers()
	{
		// Vertex shader uniform buffer block
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&uniformBuffers.fire,
			sizeof(uboVS)));

		// Vertex shader uniform buffer block
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&uniformBuffers.environment,
			sizeof(uboEnv)));

		// Map persistent
		VK_CHECK_RESULT(uniformBuffers.fire.map());
		VK_CHECK_RESULT(uniformBuffers.environment.map());

		updateUniformBuffers();
	}

	void updateUniformBufferLight()
	{
		// Environment
		uboEnv.lightPos.x = sin(timer * 2.0f * float(M_PI)) * 1.5f;
		uboEnv.lightPos.y = 0.0f;
		uboEnv.lightPos.z = cos(timer * 2.0f * float(M_PI)) * 1.5f;
		memcpy(uniformBuffers.environment.mapped, &uboEnv, sizeof(uboEnv));
	}

	void updateUniformBuffers()
	{
		// Vertex shader
		glm::mat4 viewMatrix = glm::mat4(1.0f);
		uboVS.projection = glm::perspective(glm::radians(60.0f), (float)width / (float)height, 0.001f, 256.0f);
		viewMatrix = glm::translate(viewMatrix, glm::vec3(0.0f, 0.0f, zoom));

		uboVS.model = glm::mat4(1.0f);
		uboVS.model = viewMatrix * glm::translate(uboVS.model, glm::vec3(0.0f, 15.0f, 0.0f));
		uboVS.model = glm::rotate(uboVS.model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		uboVS.model = glm::rotate(uboVS.model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		uboVS.model = glm::rotate(uboVS.model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

		uboVS.viewportDim = glm::vec2((float)width, (float)height);
		memcpy(uniformBuffers.fire.mapped, &uboVS, sizeof(uboVS));

		// Environment
		uboEnv.projection = uboVS.projection;
		uboEnv.model = uboVS.model;
		uboEnv.normal = glm::inverseTranspose(uboEnv.model);
		memcpy(uniformBuffers.environment.mapped, &uboEnv, sizeof(uboEnv));
	}

	void draw()
	{
		VulkanExampleBase::prepareFrame();

		// Command buffer to be sumitted to the queue
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];

		// Submit to queue
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

		VulkanExampleBase::submitFrame();
	}

	void prepare()
	{
		VulkanExampleBase::prepare();
		loadAssets();
		prepareParticles();
		prepareUniformBuffers();
		setupDescriptorSetLayout();
		preparePipelines();
		setupDescriptorPool();
		setupDescriptorSets();
		buildCommandBuffers();
		prepared = true;
	}

	virtual void render()
	{
		if (!prepared)
			return;
		draw();
		if (!paused)
		{
			updateUniformBufferLight();
			updateParticles();
		}
	}

	virtual void viewChanged()
	{
		updateUniformBuffers();
	}
};

VULKAN_EXAMPLE_MAIN()