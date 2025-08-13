/*
* Vulkan Example - Using descriptor sets for passing data to shader stages
*
* Relevant code parts are marked with [POI]
*
* Copyright (C) 2018-2025 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "vulkanexamplebase.h"
#include "VulkanglTFModel.h"


class VulkanExample : public VulkanExampleBase
{
public:
	bool animate = true;

	struct Cube {
		struct Matrices {
			glm::mat4 projection;
			glm::mat4 view;
			glm::mat4 model;
		} matrices;
		vks::Texture2D texture;
		std::array<vks::Buffer, maxConcurrentFrames> uniformBuffers{};
		std::array<VkDescriptorSet, maxConcurrentFrames> descriptorSets{};
		glm::vec3 rotation{ 0.0f };
	};
	std::array<Cube, 2> cubes;

	vkglTF::Model model;

	VkPipeline pipeline{ VK_NULL_HANDLE };
	VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };
	VkDescriptorSetLayout descriptorSetLayout{ VK_NULL_HANDLE };

	VulkanExample() : VulkanExampleBase()
	{
		title = "Using descriptor Sets";
		camera.type = Camera::CameraType::lookat;
		camera.setPerspective(60.0f, (float)width / (float)height, 0.1f, 512.0f);
		camera.setRotation(glm::vec3(0.0f, 0.0f, 0.0f));
		camera.setTranslation(glm::vec3(0.0f, 0.0f, -5.0f));
	}

	~VulkanExample()
	{
		vkDestroyPipeline(device, pipeline, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
		for (auto& cube : cubes) {
			cube.texture.destroy();
			for (auto& buffer : cube.uniformBuffers) {
				buffer.destroy();
			}
		}
	}

	virtual void getEnabledFeatures()
	{
		if (deviceFeatures.samplerAnisotropy) {
			enabledFeatures.samplerAnisotropy = VK_TRUE;
		};
	}

	void loadAssets()
	{
		const uint32_t glTFLoadingFlags = vkglTF::FileLoadingFlags::PreTransformVertices | vkglTF::FileLoadingFlags::PreMultiplyVertexColors | vkglTF::FileLoadingFlags::FlipY;
		model.loadFromFile(getAssetPath() + "models/cube.gltf", vulkanDevice, queue, glTFLoadingFlags);
		cubes[0].texture.loadFromFile(getAssetPath() + "textures/crate01_color_height_rgba.ktx", VK_FORMAT_R8G8B8A8_UNORM, vulkanDevice, queue);
		cubes[1].texture.loadFromFile(getAssetPath() + "textures/crate02_color_height_rgba.ktx", VK_FORMAT_R8G8B8A8_UNORM, vulkanDevice, queue);
	}

	/*
		[POI] Set up descriptor sets and set layout
	*/
	void setupDescriptors()
	{
		/*

			Descriptor pool

			Actual descriptors are allocated from a descriptor pool telling the driver what types and how many
			descriptors this application will use

			An application can have multiple pools (e.g. for multiple threads) with any number of descriptor types
			as long as device limits are not surpassed

			It's good practice to allocate pools with the actual number required descriptor types and counts

		*/

		std::array<VkDescriptorPoolSize, 2> descriptorPoolSizes{};

		// Uniform buffers : 1 per object and max. frame in flight
		descriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		// Uniform buffers are duplicated, so we can update buffer n+1 on the CPU while the GPU still reads from buffer n
		descriptorPoolSizes[0].descriptorCount = static_cast<uint32_t>(cubes.size()) * maxConcurrentFrames;

		// Combined image samples : 1 per object texture
		descriptorPoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		// Images are static, and never update after initial upload, but as they use the same descriptor set as buffers, we need to duplicate the descriptor count here too
		descriptorPoolSizes[1].descriptorCount = static_cast<uint32_t>(cubes.size()) * maxConcurrentFrames;

		// Create the global descriptor pool
		VkDescriptorPoolCreateInfo descriptorPoolCI = {};
		descriptorPoolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolCI.poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size());
		descriptorPoolCI.pPoolSizes = descriptorPoolSizes.data();
		// Max. number of descriptor sets that can be allocated from this pool (one per object and per max. frame in flight)
		descriptorPoolCI.maxSets = static_cast<uint32_t>(cubes.size() * maxConcurrentFrames);

		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolCI, nullptr, &descriptorPool));

		/*

			Descriptor set layout

			The layout describes the shader bindings and types used for a certain descriptor layout and as such must match the shader bindings

			Shader bindings used in this example:

			VS:
				layout (set = 0, binding = 0) uniform UBOMatrices ...

			FS :
				layout (set = 0, binding = 1) uniform sampler2D ...;

		*/

		std::array<VkDescriptorSetLayoutBinding, 2> setLayoutBindings{};

		/*
			Binding 0: Uniform buffers (used to pass matrices)
		*/
		setLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		// Shader binding point
		setLayoutBindings[0].binding = 0;
		// Accessible from the vertex shader only (flags can be combined to make it accessible to multiple shader stages)
		setLayoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		// Binding contains one element (can be used for array bindings)
		setLayoutBindings[0].descriptorCount = 1;

		/*
			Binding 1: Combined image sampler (used to pass per object texture information)
		*/
		setLayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		setLayoutBindings[1].binding = 1;
		// Accessible from the fragment shader only
		setLayoutBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		setLayoutBindings[1].descriptorCount = 1;

		// Create the descriptor set layout
		VkDescriptorSetLayoutCreateInfo descriptorLayoutCI{};
		descriptorLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorLayoutCI.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
		descriptorLayoutCI.pBindings = setLayoutBindings.data();

		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayoutCI, nullptr, &descriptorSetLayout));

		/*

			Descriptor sets

			Using the shared descriptor set layout and the descriptor pool we will now allocate the descriptor sets.

			Descriptor sets contain the actual descriptor for the objects (buffers, images) used at render time.

		*/

		for (auto &cube: cubes) {
			// We have one uniform buffer per cube and max. fame in flight, so we also need one descriptor per that setup
			for (auto i = 0; i < cube.uniformBuffers.size(); i++) {

				// Allocates an empty descriptor set without actual descriptors from the pool using the set layout
				VkDescriptorSetAllocateInfo allocateInfo{};
				allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
				allocateInfo.descriptorPool = descriptorPool;
				allocateInfo.descriptorSetCount = 1;
				allocateInfo.pSetLayouts = &descriptorSetLayout;
				VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocateInfo, &cube.descriptorSets[i]));

				// Update the descriptor set with the actual descriptors matching shader bindings set in the layout

				std::array<VkWriteDescriptorSet, 2> writeDescriptorSets{};

				/*
					Binding 0: Object matrices Uniform buffer
				*/
				writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptorSets[0].dstSet = cube.descriptorSets[i];
				writeDescriptorSets[0].dstBinding = 0;
				writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				writeDescriptorSets[0].pBufferInfo = &cube.uniformBuffers[i].descriptor;
				writeDescriptorSets[0].descriptorCount = 1;

				/*
					Binding 1: Object texture
					
					Duplicating this descriptor per frame isn't technically required, as images are static
					This is only done to keep the sample as easy possible
					Another option is to put image descriptors into a completely separate set
				*/
				writeDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptorSets[1].dstSet = cube.descriptorSets[i];
				writeDescriptorSets[1].dstBinding = 1;
				writeDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				// Images use a different descriptor structure, so we use pImageInfo instead of pBufferInfo
				writeDescriptorSets[1].pImageInfo = &cube.texture.descriptor;
				writeDescriptorSets[1].descriptorCount = 1;

				// Execute the writes to update descriptors for this set
				// Note that it's also possible to gather all writes and only run updates once, even for multiple sets
				// This is possible because each VkWriteDescriptorSet also contains the destination set to be updated
				// For simplicity we will update once per set instead

				vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
			}
		}

	}

	void preparePipelines()
	{
		/*
			[POI] Create a pipeline layout used for our graphics pipeline
		*/
		VkPipelineLayoutCreateInfo pipelineLayoutCI{};
		pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		// The pipeline layout is based on the descriptor set layout we created above
		pipelineLayoutCI.setLayoutCount = 1;
		pipelineLayoutCI.pSetLayouts = &descriptorSetLayout;
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &pipelineLayout));

		const std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
		VkPipelineRasterizationStateCreateInfo rasterizationStateCI = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
		VkPipelineColorBlendAttachmentState blendAttachmentState = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
		VkPipelineColorBlendStateCreateInfo colorBlendStateCI = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
		VkPipelineDepthStencilStateCreateInfo depthStencilStateCI = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
		VkPipelineViewportStateCreateInfo viewportStateCI = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
		VkPipelineMultisampleStateCreateInfo multisampleStateCI = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
		VkPipelineDynamicStateCreateInfo dynamicStateCI = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables.data(), static_cast<uint32_t>(dynamicStateEnables.size()),0);
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};

		VkGraphicsPipelineCreateInfo pipelineCI = vks::initializers::pipelineCreateInfo(pipelineLayout, renderPass, 0);
		pipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
		pipelineCI.pRasterizationState = &rasterizationStateCI;
		pipelineCI.pColorBlendState = &colorBlendStateCI;
		pipelineCI.pMultisampleState = &multisampleStateCI;
		pipelineCI.pViewportState = &viewportStateCI;
		pipelineCI.pDepthStencilState = &depthStencilStateCI;
		pipelineCI.pDynamicState = &dynamicStateCI;
		pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineCI.pStages = shaderStages.data();
		pipelineCI.pVertexInputState  = vkglTF::Vertex::getPipelineVertexInputState({vkglTF::VertexComponent::Position, vkglTF::VertexComponent::Normal, vkglTF::VertexComponent::UV, vkglTF::VertexComponent::Color});

	    shaderStages[0] = loadShader(getShadersPath() + "descriptorsets/cube.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getShadersPath() + "descriptorsets/cube.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipeline));
	}

	void prepareUniformBuffers()
	{
		// Each cube has one buffer per max. frame-in-flight
		// This way we can update one buffer on the CPU while the other is being processed by the GPU
		// Another option instead of using separate buffers could be using one large buffer with separate ranges per cube and fram
		for (auto& cube : cubes) {
			for (auto& buffer : cube.uniformBuffers) {
				// Scene matrices uniform buffer
				VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &buffer, sizeof(Cube::Matrices)));
				VK_CHECK_RESULT(buffer.map());
			}
		}
	}

	void updateUniformBuffers()
	{
		cubes[0].matrices.model = glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, 0.0f, 0.0f));
		cubes[1].matrices.model = glm::translate(glm::mat4(1.0f), glm::vec3( 1.5f, 0.5f, 0.0f));

		for (auto& cube : cubes) {
			cube.matrices.projection = camera.matrices.perspective;
			cube.matrices.view = camera.matrices.view;
			cube.matrices.model = glm::rotate(cube.matrices.model, glm::radians(cube.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
			cube.matrices.model = glm::rotate(cube.matrices.model, glm::radians(cube.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
			cube.matrices.model = glm::rotate(cube.matrices.model, glm::radians(cube.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
			cube.matrices.model = glm::scale(cube.matrices.model, glm::vec3(0.25f));
			/*
				[POI] Update the uniform values seen by the GPU for the current frame
			*/
			memcpy(cube.uniformBuffers[currentBuffer].mapped, &cube.matrices, sizeof(cube.matrices));
		}
	}

	void prepare()
	{
		VulkanExampleBase::prepare();
		loadAssets();
		prepareUniformBuffers();
		setupDescriptors();
		preparePipelines();
		prepared = true;
	}

	void buildCommandBuffer()
	{
		VkCommandBuffer cmdBuffer = drawCmdBuffers[currentBuffer];
		
		VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

		VkClearValue clearValues[2]{};
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
		renderPassBeginInfo.framebuffer = frameBuffers[currentImageIndex];

		VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));

		vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		VkViewport viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
		vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

		VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);
		vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

		model.bindBuffers(cmdBuffer);

		/*
			[POI] Render cubes with separate descriptor sets
		*/
		for (auto cube : cubes) {
			// Bind the cube's descriptor set. This tells the command buffer to use the uniform buffer for the current frame and image set for this cube
			vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &cube.descriptorSets[currentBuffer], 0, nullptr);
			model.draw(cmdBuffer);
		}

		drawUI(cmdBuffer);

		vkCmdEndRenderPass(cmdBuffer);

		VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffer));
	}

	virtual void render()
	{
		if (!prepared)
			return;
		if (animate && !paused) {
			cubes[0].rotation.x += 2.5f * frameTimer;
			if (cubes[0].rotation.x > 360.0f)
				cubes[0].rotation.x -= 360.0f;
			cubes[1].rotation.y += 2.0f * frameTimer;
			if (cubes[1].rotation.y > 360.0f)
				cubes[1].rotation.y -= 360.0f;
		}
		VulkanExampleBase::prepareFrame();
		updateUniformBuffers();
		buildCommandBuffer();
		VulkanExampleBase::submitFrame();
	}

	virtual void OnUpdateUIOverlay(vks::UIOverlay *overlay)
	{
		if (overlay->header("Settings")) {
			overlay->checkBox("Animate", &animate);
		}
	}
};

VULKAN_EXAMPLE_MAIN()
