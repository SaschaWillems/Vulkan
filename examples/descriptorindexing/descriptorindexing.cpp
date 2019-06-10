/*
* Vulkan Example - Descriptor indexing (VK_EXT_descriptor_indexing)
*
* Relevant code parts are marked with [POI]
*
* Copyright (C) Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h> 
#include <vector>
#include <random>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vulkan/vulkan.h>
#include "vulkanexamplebase.h"
#include "VulkanTexture.hpp"
#include "VulkanBuffer.hpp"

#define ENABLE_VALIDATION false

// Vertex layout for this example
struct Vertex {
	float pos[3];
	float uv[2];
	int32_t textureIndex;
};

class VulkanExample : public VulkanExampleBase
{
public:
	// Number of array layers in texture array
	// Also used as instance count
	uint32_t layerCount;
	std::vector<vks::Texture2D> textures;

	vks::Buffer vertexBuffer;
	vks::Buffer indexBuffer;
	uint32_t indexCount;

	vks::Buffer uniformBufferVS;
	struct {
		glm::mat4 projection;
		glm::mat4 view;
		glm::mat4 model;
	} uboVS;

	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;
	VkDescriptorSet descriptorSet;
	VkDescriptorSetLayout descriptorSetLayout;

	VkPhysicalDeviceDescriptorIndexingFeaturesEXT physicalDeviceDescriptorIndexingFeatures{};

	VulkanExample() : VulkanExampleBase(ENABLE_VALIDATION)
	{
		title = "Descriptor indexing";
		settings.overlay = true;
		camera.type = Camera::CameraType::lookat;
		camera.setPosition(glm::vec3(0.0f, 0.0f, -7.5f));
		camera.setRotation(glm::vec3(-35.0f, 0.0f, 0.0f));
		camera.setPerspective(45.0f, (float)width / (float)height, 0.1f, 256.0f);
		
		/*
			[POI] Enable required extensions
		*/
		enabledInstanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
		enabledDeviceExtensions.push_back(VK_KHR_MAINTENANCE3_EXTENSION_NAME);
		enabledDeviceExtensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);

		/*
			[POI] Enable required extension features
		*/
		physicalDeviceDescriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
		physicalDeviceDescriptorIndexingFeatures.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
		physicalDeviceDescriptorIndexingFeatures.runtimeDescriptorArray = VK_TRUE;
		physicalDeviceDescriptorIndexingFeatures.descriptorBindingVariableDescriptorCount = VK_TRUE;

		deviceCreatepNextChain = &physicalDeviceDescriptorIndexingFeatures;
	}

	~VulkanExample()
	{
		//vkDestroyImageView(device, textureArray.view, nullptr);
		//vkDestroyImage(device, textureArray.image, nullptr);
		//vkDestroySampler(device, textureArray.sampler, nullptr);
		//vkFreeMemory(device, textureArray.deviceMemory, nullptr);
		vkDestroyPipeline(device, pipeline, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
		vertexBuffer.destroy();
		indexBuffer.destroy();
		uniformBufferVS.destroy();
	}

	void loadAssets()
	{
		textures.resize(7);
		for (uint32_t i = 0; i < 7; i++) {
			textures[i].loadFromFile(getAssetPath() + "textures/array/" + std::to_string(i) + ".ktx", VK_FORMAT_R8G8B8A8_UNORM, vulkanDevice, queue);
		}
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
			renderPassBeginInfo.framebuffer = frameBuffers[i];

			VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo));

			vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
			vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

			VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);
			vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

			vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);
			vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

			VkDeviceSize offsets[1] = { 0 };
			vkCmdBindVertexBuffers(drawCmdBuffers[i], 0, 1, &vertexBuffer.buffer, offsets);
			vkCmdBindIndexBuffer(drawCmdBuffers[i], indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

			vkCmdDrawIndexed(drawCmdBuffers[i], indexCount, layerCount, 0, 0, 0);

			drawUI(drawCmdBuffers[i]);

			vkCmdEndRenderPass(drawCmdBuffers[i]);

			VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
		}
	}

	void generateCube(glm::vec3 pos, float scale, std::vector<Vertex> &vertices, std::vector<uint32_t> &indices)
	{
		// Generate random per-face texture indices
		std::random_device mch;
		std::default_random_engine generator(mch());
		std::uniform_int_distribution<int32_t> rndTextureIndex(0, layerCount - 1);

		std::vector<int32_t> faceTextureIndex(6);
		for (auto &textureIndex : faceTextureIndex) {
			textureIndex = rndTextureIndex(generator);
		}

		std::cout << faceTextureIndex[0] << std::endl;

		std::vector<Vertex> cubeVertices = {
			{ { -scale, -scale,  scale }, { 0.0f, 0.0f }, faceTextureIndex[0] },
			{ {  scale, -scale,  scale }, { 1.0f, 0.0f }, faceTextureIndex[0] },
			{ {  scale,  scale,  scale }, { 1.0f, 1.0f }, faceTextureIndex[0] },
			{ { -scale,  scale,  scale }, { 0.0f, 1.0f }, faceTextureIndex[0] },

			{ {  scale,  scale,  scale }, { 0.0f, 0.0f }, faceTextureIndex[1] },
			{ {  scale,  scale, -scale }, { 1.0f, 0.0f }, faceTextureIndex[1] },
			{ {  scale, -scale, -scale }, { 1.0f, 1.0f }, faceTextureIndex[1] },
			{ {  scale, -scale,  scale }, { 0.0f, 1.0f }, faceTextureIndex[1] },
	
			{ { -scale, -scale, -scale }, { 0.0f, 0.0f }, faceTextureIndex[2] },
			{ {  scale, -scale, -scale }, { 1.0f, 0.0f }, faceTextureIndex[2] },
			{ {  scale,  scale, -scale }, { 1.0f, 1.0f }, faceTextureIndex[2] },
			{ { -scale,  scale, -scale }, { 0.0f, 1.0f }, faceTextureIndex[2] },

			{ { -scale, -scale, -scale }, { 0.0f, 0.0f }, faceTextureIndex[3] },
			{ { -scale, -scale,  scale }, { 1.0f, 0.0f }, faceTextureIndex[3] },
			{ { -scale,  scale,  scale }, { 1.0f, 1.0f }, faceTextureIndex[3] },
			{ { -scale,  scale, -scale }, { 0.0f, 1.0f }, faceTextureIndex[3] },
		
			{ {  scale,  scale,  scale }, { 0.0f, 0.0f }, faceTextureIndex[4] },
			{ { -scale,  scale,  scale }, { 1.0f, 0.0f }, faceTextureIndex[4] },
			{ { -scale,  scale, -scale }, { 1.0f, 1.0f }, faceTextureIndex[4] },
			{ {  scale,  scale, -scale }, { 0.0f, 1.0f }, faceTextureIndex[4] },

			{ { -scale, -scale, -scale }, { 0.0f, 0.0f }, faceTextureIndex[5] },
			{ {  scale, -scale, -scale }, { 1.0f, 0.0f }, faceTextureIndex[5] },
			{ {  scale, -scale,  scale }, { 1.0f, 1.0f }, faceTextureIndex[5] },
			{ { -scale, -scale,  scale }, { 0.0f, 1.0f }, faceTextureIndex[5] },

		};
		for (auto &vertex : cubeVertices) {
			vertex.pos[0] += pos.x;
			vertex.pos[1] += pos.y;
			vertex.pos[2] += pos.z;
		}

		std::vector<uint32_t> cubeIndices = {
			0,1,2, 0,2,3, 4,5,6, 4,6,7, 8,9,10, 8,10,11, 12,13,14, 12,14,15, 16,17,18, 16,18,19, 20,21,22, 20,22,23
		};
		for (auto &index : cubeIndices) {
			index += vertices.size();
		}

		vertices.insert(vertices.end(), cubeVertices.begin(), cubeVertices.end());
		indices.insert(indices.end(), cubeIndices.begin(), cubeIndices.end());
	}

	void generateBuffers()
	{
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;

		float offset = -1.5f;
		float center = (layerCount * offset) / 2.0f - (offset * 0.5f);
		//for (uint32_t i = 0; i < 7; i++) {
			//generateCube(glm::vec3(i * offset - center, 0.0f, 0.0f), 0.5f, vertices, indices);
		//}
		generateCube(glm::vec3(0.0f, 0.0f, 0.0f), 0.5f, vertices, indices);
		indexCount = static_cast<uint32_t>(indices.size());

		// For the sake of simplicity we won't stage the vertex data to the gpu memory
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&vertexBuffer,
			vertices.size() * sizeof(Vertex),
			vertices.data()));
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&indexBuffer,
			indices.size() * sizeof(uint32_t),
			indices.data()));
	}

	/*
		[POI] Set up descriptor sets and set layout
	*/
	void setupDescriptorSets()
	{
		/*
			Descriptor set layout
		*/

		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
			// Binding 0 : Vertex shader uniform buffer
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
			/*
				[POI] 
				
				Binding 1 contains a texture array that is dynamically non-uniform sampled from

				FS:
					outFragColor = texture(textures[nonuniformEXT(inTexIndex)], inUV);
			*/
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, static_cast<uint32_t>(textures.size()))
		};

		/*
			[POI] 
			
			The fragment shader will be using an unsized array, which has to be marked with a certain flag

			FS:
				layout (set = 0, binding = 1) uniform sampler2D textures[];
		*/
		VkDescriptorSetLayoutBindingFlagsCreateInfoEXT setLayoutBindingFlags{};
		setLayoutBindingFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
		setLayoutBindingFlags.bindingCount = 2;
		std::vector<VkDescriptorBindingFlagsEXT> descriptorBindingFlags = {
			0,													
			VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT
		};
		setLayoutBindingFlags.pBindingFlags = descriptorBindingFlags.data();

		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings.data(), setLayoutBindings.size());
		descriptorSetLayoutCI.pNext = &setLayoutBindingFlags;
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCI, nullptr, &descriptorSetLayout));

		/*
			Descriptor pool
		*/

		std::vector<VkDescriptorPoolSize> poolSizes = {
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1),
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, textures.size())
		};
		VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes.size(), poolSizes.data(), 2);
		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));

		/*
			Descriptor sets
		*/

		VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));

		std::vector<VkWriteDescriptorSet> writeDescriptorSets(2);

		writeDescriptorSets[0] = vks::initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBufferVS.descriptor);

		// Image descriptors for the texture array
		std::vector<VkDescriptorImageInfo> textureDescriptors(textures.size());
		for (size_t i = 0; i < textures.size(); i++) {
			textureDescriptors[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			textureDescriptors[i].sampler = textures[i].sampler;;
			textureDescriptors[i].imageView = textures[i].view;
		}

		/*
			[POI]

			Second and final descriptor is a texture array

			Unlike an array texture, these are adressed like typical arrays
		*/
		writeDescriptorSets[1] = {};
		writeDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSets[1].dstBinding = 1;
		writeDescriptorSets[1].dstArrayElement = 0;
		writeDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writeDescriptorSets[1].descriptorCount = static_cast<uint32_t>(textures.size());
		writeDescriptorSets[1].pBufferInfo = 0;
		writeDescriptorSets[1].dstSet = descriptorSet;
		writeDescriptorSets[1].pImageInfo = textureDescriptors.data();

		vkUpdateDescriptorSets(device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);
	}

	void preparePipelines()
	{

		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(&descriptorSetLayout, 1);
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
		VkPipelineRasterizationStateCreateInfo rasterizationStateCI = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
		VkPipelineColorBlendAttachmentState blendAttachmentState = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE); 
		VkPipelineColorBlendStateCreateInfo colorBlendStateCI = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
		VkPipelineDepthStencilStateCreateInfo depthStencilStateCI = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
		VkPipelineViewportStateCreateInfo viewportStateCI = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
		VkPipelineMultisampleStateCreateInfo multisampleStateCI = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
		std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicStateCI = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables.data(), dynamicStateEnables.size(), 0);

		// Vertex bindings and attributes
		VkVertexInputBindingDescription vertexInputBinding = { 0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX };
		std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = {
			{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos) },
			{ 1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv) },
			{ 2, 0, VK_FORMAT_R32_SINT, offsetof(Vertex, textureIndex) }
		};
		VkPipelineVertexInputStateCreateInfo vertexInputStateCI = vks::initializers::pipelineVertexInputStateCreateInfo();
		vertexInputStateCI.vertexBindingDescriptionCount = 1;
		vertexInputStateCI.pVertexBindingDescriptions = &vertexInputBinding;
		vertexInputStateCI.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
		vertexInputStateCI.pVertexAttributeDescriptions = vertexInputAttributes.data();

		// Instacing pipeline
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

		shaderStages[0] = loadShader(getAssetPath() + "shaders/descriptorindexing/descriptorindexing.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getAssetPath() + "shaders/descriptorindexing/descriptorindexing.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

		VkGraphicsPipelineCreateInfo pipelineCI = vks::initializers::pipelineCreateInfo(pipelineLayout, renderPass, 0);
		pipelineCI.pVertexInputState = &vertexInputStateCI;
		pipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
		pipelineCI.pRasterizationState = &rasterizationStateCI;
		pipelineCI.pColorBlendState = &colorBlendStateCI;
		pipelineCI.pMultisampleState = &multisampleStateCI;
		pipelineCI.pViewportState = &viewportStateCI;
		pipelineCI.pDepthStencilState = &depthStencilStateCI;
		pipelineCI.pDynamicState = &dynamicStateCI;
		pipelineCI.stageCount = shaderStages.size();
		pipelineCI.pStages = shaderStages.data();

		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipeline));
	}

	void prepareUniformBuffers()
	{
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&uniformBufferVS,
			sizeof(uboVS)));
		VK_CHECK_RESULT(uniformBufferVS.map());
		updateUniformBuffersCamera();
	}

	void updateUniformBuffersCamera()
	{
		uboVS.projection = camera.matrices.perspective;
		uboVS.view = camera.matrices.view;
		uboVS.model = glm::mat4(1.0f);
		memcpy(uniformBufferVS.mapped, &uboVS, sizeof(uboVS));
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
		loadAssets();
		layerCount = textures.size();
		generateBuffers();
		prepareUniformBuffers();
		setupDescriptorSets();
		preparePipelines();
		buildCommandBuffers();
		prepared = true;
	}

	virtual void render()
	{
		if (!prepared)
			return;
		draw();
		if (camera.updated)
			updateUniformBuffersCamera();
	}

};

VULKAN_EXAMPLE_MAIN()