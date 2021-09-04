/*
* Vulkan Example - Using inline uniform blocks for passing data to shader stages at descriptor setup

* Note: Requires a device that supports the VK_EXT_inline_uniform_block extension
*
* Relevant code parts are marked with [POI]
*
* Copyright (C) 2018-2021 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "vulkanexamplebase.h"
#include "VulkanglTFModel.h"

#define ENABLE_VALIDATION false

float rnd() {
	return ((float)rand() / (RAND_MAX));
}

class VulkanExample : public VulkanExampleBase
{
public:
	VkPhysicalDeviceInlineUniformBlockFeaturesEXT enabledInlineUniformBlockFeatures{};

	vkglTF::Model model;

	struct Object {
		struct  Material {
			float roughness;
			float metallic;
			float r, g, b;
			float ambient;
		} material;
		VkDescriptorSet descriptorSet;
		void setRandomMaterial() {
			material.r = rnd();
			material.g = rnd();
			material.b = rnd();
			material.ambient = 0.0025f;
			material.roughness = glm::clamp(rnd(), 0.005f, 1.0f);
			material.metallic = glm::clamp(rnd(), 0.005f, 1.0f);
		}
	};
	std::array<Object, 16> objects;

	struct {
		vks::Buffer scene;
	} uniformBuffers;

	struct UBOMatrices {
		glm::mat4 projection;
		glm::mat4 model;
		glm::mat4 view;
		glm::vec3 camPos;
	} uboMatrices;

	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;
	VkDescriptorSet descriptorSet;

	struct DescriptorSetLaysts {
		VkDescriptorSetLayout scene;
		VkDescriptorSetLayout object;
	} descriptorSetLayouts;

	VulkanExample() : VulkanExampleBase(ENABLE_VALIDATION)
	{
		title = "Inline uniform blocks";
		camera.type = Camera::CameraType::firstperson;
		camera.setPosition(glm::vec3(0.0f, 0.0f, -10.0f));
		camera.setRotation(glm::vec3(0.0, 0.0f, 0.0f));
		camera.setPerspective(60.0f, (float)width / (float)height, 0.1f, 256.0f);
		camera.movementSpeed = 4.0f;
		camera.rotationSpeed = 0.25f;

		srand((unsigned int)time(0));

		/*
			[POI] Enable extensions required for inline uniform blocks
		*/
		enabledDeviceExtensions.push_back(VK_EXT_INLINE_UNIFORM_BLOCK_EXTENSION_NAME);
		enabledDeviceExtensions.push_back(VK_KHR_MAINTENANCE1_EXTENSION_NAME);
		enabledInstanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	}

	~VulkanExample()
	{
		vkDestroyPipeline(device, pipeline, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, descriptorSetLayouts.scene, nullptr);
		vkDestroyDescriptorSetLayout(device, descriptorSetLayouts.object, nullptr);
		uniformBuffers.scene.destroy();
	}

	void buildCommandBuffers()
	{
		VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

		VkClearValue clearValues[2];
		clearValues[0].color = { { 0.15f, 0.15f, 0.15f, 1.0f } };
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

			VkDeviceSize offsets[1] = { 0 };

			// Render objects
			vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

			uint32_t objcount = static_cast<uint32_t>(objects.size());
			for (uint32_t x = 0; x < objcount; x++) {
				/*
					[POI] Bind descriptor sets
					Set 0 = Scene matrices:
					Set 1 = Object inline uniform block (In shader pbr.frag: layout (set = 1, binding = 0) uniform UniformInline ... )
				*/
				std::vector<VkDescriptorSet> descriptorSets = {
					descriptorSet,
					objects[x].descriptorSet
				};
				vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 2, descriptorSets.data(), 0, nullptr);

				glm::vec3 pos = glm::vec3(sin(glm::radians(x * (360.0f / objcount))), cos(glm::radians(x * (360.0f / objcount))), 0.0f) * 3.5f;

				vkCmdPushConstants(drawCmdBuffers[i], pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::vec3), &pos);
				model.draw(drawCmdBuffers[i]);
			}
			drawUI(drawCmdBuffers[i]);

			vkCmdEndRenderPass(drawCmdBuffers[i]);

			VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
		}
	}

	void loadAssets()
	{
		model.loadFromFile(getAssetPath() + "models/sphere.gltf", vulkanDevice, queue);

		// Setup random materials for every object in the scene
		for (uint32_t i = 0; i < objects.size(); i++) {
			objects[i].setRandomMaterial();
		}
	}

	void setupDescriptorSetLayout()
	{
		// Scene
		{
			std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
				vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0),
			};
			VkDescriptorSetLayoutCreateInfo descriptorLayoutCI = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
			VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayoutCI, nullptr, &descriptorSetLayouts.scene));
		}

		// Objects
		{
			std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
				/*
					[POI] Setup inline uniform block for set 1 at binding 0 (see fragment shader)
					Descriptor count for an inline uniform block contains data sizes of the block (last parameter)
				*/
				vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(Object::Material)),
			};
			VkDescriptorSetLayoutCreateInfo descriptorLayoutCI = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
			VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayoutCI, nullptr, &descriptorSetLayouts.object));
		}

		/*
			[POI] Pipeline layout
		*/
		std::vector<VkDescriptorSetLayout> setLayouts = {
			descriptorSetLayouts.scene, // Set 0 = Scene matrices
			descriptorSetLayouts.object // Set 1 = Object inline uniform block
		};
		VkPipelineLayoutCreateInfo pipelineLayoutCI = vks::initializers::pipelineLayoutCreateInfo(setLayouts.data(), static_cast<uint32_t>(setLayouts.size()));

		std::vector<VkPushConstantRange> pushConstantRanges = {
			vks::initializers::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::vec3), 0),
		};
		pipelineLayoutCI.pushConstantRangeCount = 1;
		pipelineLayoutCI.pPushConstantRanges = pushConstantRanges.data();

		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &pipelineLayout));
	}

	void setupDescriptorSets()
	{
		// Pool
		std::vector<VkDescriptorPoolSize> poolSizes = {
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1),
			/* [POI] Allocate inline uniform blocks */
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT, static_cast<uint32_t>(objects.size()) * sizeof(Object::Material)),
		};
		VkDescriptorPoolCreateInfo descriptorPoolCI = vks::initializers::descriptorPoolCreateInfo(poolSizes, static_cast<uint32_t>(objects.size()) + 1);

		/*
			[POI] New structure that has to be chained into the descriptor pool's createinfo if you want to allocate inline uniform blocks
		*/
		VkDescriptorPoolInlineUniformBlockCreateInfoEXT descriptorPoolInlineUniformBlockCreateInfo{};
		descriptorPoolInlineUniformBlockCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_INLINE_UNIFORM_BLOCK_CREATE_INFO_EXT;
		descriptorPoolInlineUniformBlockCreateInfo.maxInlineUniformBlockBindings = static_cast<uint32_t>(objects.size());
		descriptorPoolCI.pNext = &descriptorPoolInlineUniformBlockCreateInfo;

		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolCI, nullptr, &descriptorPool));

		// Sets

		// Scene
		VkDescriptorSetAllocateInfo descriptorAllocateInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayouts.scene, 1);
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &descriptorAllocateInfo, &descriptorSet));

		std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
			vks::initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffers.scene.descriptor),
		};
		vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);

		// Objects
		for (auto &object : objects) {
			VkDescriptorSetAllocateInfo descriptorAllocateInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayouts.object, 1);
			VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &descriptorAllocateInfo, &object.descriptorSet));

			/*
				[POI] New structure that defines size and data of the inline uniform block needs to be chained into the write descriptor set
				We will be using this inline uniform block to pass per-object material information to the fragment shader
			*/
			VkWriteDescriptorSetInlineUniformBlockEXT writeDescriptorSetInlineUniformBlock{};
			writeDescriptorSetInlineUniformBlock.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_INLINE_UNIFORM_BLOCK_EXT;
			writeDescriptorSetInlineUniformBlock.dataSize = sizeof(Object::Material);
			// Uniform data for the inline block
			writeDescriptorSetInlineUniformBlock.pData = &object.material;

			/*
				[POI] Setup the inline uniform block
			*/
			VkWriteDescriptorSet writeDescriptorSet{};
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT;
			writeDescriptorSet.dstSet = object.descriptorSet;
			writeDescriptorSet.dstBinding = 0;
			// Descriptor count for an inline uniform block contains data sizes of the block(last parameter)
			writeDescriptorSet.descriptorCount = sizeof(Object::Material);
			// Chain inline uniform block structure
			writeDescriptorSet.pNext = &writeDescriptorSetInlineUniformBlock;

			vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
		}
	}

	void preparePipelines()
	{
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
		VkPipelineRasterizationStateCreateInfo rasterizationStateCI = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
		VkPipelineColorBlendAttachmentState blendAttachmentState = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
		VkPipelineColorBlendStateCreateInfo colorBlendStateCI = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
		VkPipelineDepthStencilStateCreateInfo depthStencilStateCI = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
		VkPipelineViewportStateCreateInfo viewportStateCI = vks::initializers::pipelineViewportStateCreateInfo(1, 1);
		VkPipelineMultisampleStateCreateInfo multisampleStateCI = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);
		std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicStateCI = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

		VkGraphicsPipelineCreateInfo pipelineCI = vks::initializers::pipelineCreateInfo(pipelineLayout, renderPass);
		pipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
		pipelineCI.pRasterizationState = &rasterizationStateCI;
		pipelineCI.pColorBlendState = &colorBlendStateCI;
		pipelineCI.pMultisampleState = &multisampleStateCI;
		pipelineCI.pViewportState = &viewportStateCI;
		pipelineCI.pDepthStencilState = &depthStencilStateCI;
		pipelineCI.pDynamicState = &dynamicStateCI;
		pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineCI.pStages = shaderStages.data();
		pipelineCI.pVertexInputState = vkglTF::Vertex::getPipelineVertexInputState({ vkglTF::VertexComponent::Position, vkglTF::VertexComponent::Normal });

		shaderStages[0] = loadShader(getShadersPath() + "inlineuniformblocks/pbr.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getShadersPath() + "inlineuniformblocks/pbr.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipeline));
	}

	void prepareUniformBuffers()
	{
		VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &uniformBuffers.scene, sizeof(uboMatrices)));
		VK_CHECK_RESULT(uniformBuffers.scene.map());
		updateUniformBuffers();
	}

	void updateUniformBuffers()
	{
		uboMatrices.projection = camera.matrices.perspective;
		uboMatrices.view = camera.matrices.view;
		uboMatrices.model = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f));
		uboMatrices.camPos = camera.position * glm::vec3(-1.0f, 1.0f, -1.0f);
		memcpy(uniformBuffers.scene.mapped, &uboMatrices, sizeof(uboMatrices));
	}

	void draw()
	{
		VulkanExampleBase::prepareFrame();

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

		VulkanExampleBase::submitFrame();
	}

	void getEnabledFeatures()
	{
		// Enable the inline uniform block feature using the dedicated physical device structure
		enabledInlineUniformBlockFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INLINE_UNIFORM_BLOCK_FEATURES_EXT;
		enabledInlineUniformBlockFeatures.inlineUniformBlock = VK_TRUE;
		deviceCreatepNextChain = &enabledInlineUniformBlockFeatures;
	}

	void prepare()
	{
		VulkanExampleBase::prepare();
		loadAssets();
		prepareUniformBuffers();
		setupDescriptorSetLayout();
		preparePipelines();
		setupDescriptorSets();
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

	/*
		[POI] Update descriptor sets at runtime
	*/
	void updateMaterials() {
		// Setup random materials for every object in the scene
		for (uint32_t i = 0; i < objects.size(); i++) {
			objects[i].setRandomMaterial();
		}

		for (auto &object : objects) {
			/*
				[POI] New structure that defines size and data of the inline uniform block needs to be chained into the write descriptor set
				We will be using this inline uniform block to pass per-object material information to the fragment shader
			*/
			VkWriteDescriptorSetInlineUniformBlockEXT writeDescriptorSetInlineUniformBlock{};
			writeDescriptorSetInlineUniformBlock.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_INLINE_UNIFORM_BLOCK_EXT;
			writeDescriptorSetInlineUniformBlock.dataSize = sizeof(Object::Material);
			// Uniform data for the inline block
			writeDescriptorSetInlineUniformBlock.pData = &object.material;

			/*
				[POI] Update the object's inline uniform block
			*/
			VkWriteDescriptorSet writeDescriptorSet{};
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT;
			writeDescriptorSet.dstSet = object.descriptorSet;
			writeDescriptorSet.dstBinding = 0;
			writeDescriptorSet.descriptorCount = sizeof(Object::Material);
			writeDescriptorSet.pNext = &writeDescriptorSetInlineUniformBlock;

			vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
		}
	}

	virtual void OnUpdateUIOverlay(vks::UIOverlay *overlay)
	{
		if (overlay->button("Randomize")) {
			updateMaterials();
		}
	}

};

VULKAN_EXAMPLE_MAIN()