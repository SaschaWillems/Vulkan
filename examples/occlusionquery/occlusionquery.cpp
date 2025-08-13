/*
* Vulkan Example - Using occlusion query for visibility testing
*
* Copyright (C) 2016-2025 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "vulkanexamplebase.h"
#include "VulkanglTFModel.h"

class VulkanExample : public VulkanExampleBase
{
public:

	struct {
		vkglTF::Model teapot;
		vkglTF::Model plane;
		vkglTF::Model sphere;
	} models;

	struct UniformBuffers {
		vks::Buffer occluder;
		vks::Buffer teapot;
		vks::Buffer sphere;
	};
	std::array<UniformBuffers, maxConcurrentFrames> uniformBuffers;

	struct UniformData {
		glm::mat4 projection;
		glm::mat4 view;
		glm::mat4 model;
		glm::vec4 color{ glm::vec4(0.0f) };
		glm::vec4 lightPos{ glm::vec4(10.0f, -10.0f, 10.0f, 1.0f) };
		float visible{ false };
	} uniformData;

	VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };
	struct {
		VkPipeline solid;
		VkPipeline occluder;
		// Pipeline with basic shaders used for occlusion pass
		VkPipeline simple;
	} pipelines;

	VkDescriptorSetLayout descriptorSetLayout{ VK_NULL_HANDLE };
	struct DescriptorSets {
		VkDescriptorSet occluder;
		VkDescriptorSet teapot;
		VkDescriptorSet sphere;
	};
	std::array<DescriptorSets, maxConcurrentFrames> descriptorSets;

	// Pool that stores all occlusion queries
	VkQueryPool queryPool;

	// Passed query samples
	uint64_t passedSamples[2] = { 1,1 };

	VulkanExample() : VulkanExampleBase()
	{
		title = "Occlusion queries";
		camera.type = Camera::CameraType::lookat;
		camera.setPosition(glm::vec3(0.0f, 0.0f, -7.5f));
		camera.setRotation(glm::vec3(0.0f, -123.75f, 0.0f));
		camera.setRotationSpeed(0.5f);
		camera.setPerspective(60.0f, (float)width / (float)height, 1.0f, 256.0f);
	}

	~VulkanExample()
	{
		if (device) {
			vkDestroyPipeline(device, pipelines.solid, nullptr);
			vkDestroyPipeline(device, pipelines.occluder, nullptr);
			vkDestroyPipeline(device, pipelines.simple, nullptr);
			vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
			vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
			vkDestroyQueryPool(device, queryPool, nullptr);
			for (auto& buffer : uniformBuffers) {
				buffer.sphere.destroy();
				buffer.teapot.destroy();
				buffer.occluder.destroy();
			}
		}
	}

	// Create a query pool for storing the occlusion query result
	void setupQueryPool()
	{
		VkQueryPoolCreateInfo queryPoolInfo = {};
		queryPoolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
		queryPoolInfo.queryType = VK_QUERY_TYPE_OCCLUSION;
		queryPoolInfo.queryCount = 2;
		VK_CHECK_RESULT(vkCreateQueryPool(device, &queryPoolInfo, NULL, &queryPool));
	}

	void loadAssets()
	{
		const uint32_t glTFLoadingFlags = vkglTF::FileLoadingFlags::PreTransformVertices | vkglTF::FileLoadingFlags::PreMultiplyVertexColors | vkglTF::FileLoadingFlags::FlipY;
		models.plane.loadFromFile(getAssetPath() + "models/plane_z.gltf", vulkanDevice, queue, glTFLoadingFlags);
		models.teapot.loadFromFile(getAssetPath() + "models/teapot.gltf", vulkanDevice, queue, glTFLoadingFlags);
		models.sphere.loadFromFile(getAssetPath() + "models/sphere.gltf", vulkanDevice, queue, glTFLoadingFlags);
	}

	void setupDescriptors()
	{
		// Pool
		std::vector<VkDescriptorPoolSize> poolSizes = {
			// One uniform buffer block for each mesh
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, maxConcurrentFrames * 3)
		};
		VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, maxConcurrentFrames * 3);
		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));

		// Layout
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
			// Binding 0 : Vertex shader uniform buffer
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0)
		};
		VkDescriptorSetLayoutCreateInfo descriptorLayout = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayout));

		// Sets per frame, just like the buffers themselves
		for (auto i = 0; i < uniformBuffers.size(); i++) {
			VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);
			// Occluder (plane)
			VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets[i].occluder));
			std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
				vks::initializers::writeDescriptorSet(descriptorSets[i].occluder, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffers[i].occluder.descriptor)
			};
			vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
			// Teapot
			VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets[i].teapot));
			writeDescriptorSets = {
				vks::initializers::writeDescriptorSet(descriptorSets[i].teapot, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffers[i].teapot.descriptor)
			};
			vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
			// Sphere
			VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets[i].sphere));
			writeDescriptorSets = {
				vks::initializers::writeDescriptorSet(descriptorSets[i].sphere, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffers[i].sphere.descriptor)
			};
			vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
		}
	}

	void preparePipelines()
	{
		// Layout
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(&descriptorSetLayout, 1);
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

		// Pipelines
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
		VkPipelineRasterizationStateCreateInfo rasterizationState = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
		VkPipelineColorBlendAttachmentState blendAttachmentState = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
		VkPipelineColorBlendStateCreateInfo colorBlendState = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
		VkPipelineDepthStencilStateCreateInfo depthStencilState = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
		VkPipelineViewportStateCreateInfo viewportState = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
		VkPipelineMultisampleStateCreateInfo multisampleState = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
		std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicState = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};

		VkGraphicsPipelineCreateInfo pipelineCI = vks::initializers::pipelineCreateInfo(pipelineLayout, renderPass, 0);
		pipelineCI.pInputAssemblyState = &inputAssemblyState;
		pipelineCI.pRasterizationState = &rasterizationState;
		pipelineCI.pColorBlendState = &colorBlendState;
		pipelineCI.pMultisampleState = &multisampleState;
		pipelineCI.pViewportState = &viewportState;
		pipelineCI.pDepthStencilState = &depthStencilState;
		pipelineCI.pDynamicState = &dynamicState;
		pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineCI.pStages = shaderStages.data();
		pipelineCI.pVertexInputState = vkglTF::Vertex::getPipelineVertexInputState({ vkglTF::VertexComponent::Position, vkglTF::VertexComponent::Normal, vkglTF::VertexComponent::Color });;

		// Solid rendering pipeline
		shaderStages[0] = loadShader(getShadersPath() + "occlusionquery/mesh.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getShadersPath() + "occlusionquery/mesh.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.solid));

		// Basic pipeline for coloring occluded objects
		shaderStages[0] = loadShader(getShadersPath() + "occlusionquery/simple.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getShadersPath() + "occlusionquery/simple.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		rasterizationState.cullMode = VK_CULL_MODE_NONE;
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.simple));

		// Visual pipeline for the occluder
		shaderStages[0] = loadShader(getShadersPath() + "occlusionquery/occluder.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getShadersPath() + "occlusionquery/occluder.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		// Enable blending
		blendAttachmentState.blendEnable = VK_TRUE;
		blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
		blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
		blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.occluder));
	}

	// Prepare and initialize uniform buffer containing shader uniforms
	void prepareUniformBuffers()
	{
		for (auto& buffer : uniformBuffers) {
			// Occluder
			VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &buffer.occluder, sizeof(UniformData)));
			VK_CHECK_RESULT(buffer.occluder.map());
			// Teapot
			VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &buffer.teapot, sizeof(UniformData)));
			VK_CHECK_RESULT(buffer.teapot.map());
			// Sphere
			VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &buffer.sphere, sizeof(UniformData)));
			VK_CHECK_RESULT(buffer.sphere.map());
		}
	}

	void updateUniformBuffers()
	{
		uniformData.projection = camera.matrices.perspective;
		uniformData.view = camera.matrices.view;

		// Occluder
		uniformData.visible = 1.0f;
		uniformData.model = glm::scale(glm::mat4(1.0f), glm::vec3(6.0f));
		uniformData.color = glm::vec4(0.0f, 0.0f, 1.0f, 0.5f);
		memcpy(uniformBuffers[currentBuffer].occluder.mapped, &uniformData, sizeof(uniformData));

		// Teapot
		// Toggle color depending on visibility
		uniformData.visible = (passedSamples[0] > 0) ? 1.0f : 0.0f;
		uniformData.model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -3.0f));
		uniformData.color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
		memcpy(uniformBuffers[currentBuffer].teapot.mapped, &uniformData, sizeof(uniformData));

		// Sphere
		// Toggle color depending on visibility
		uniformData.visible = (passedSamples[1] > 0) ? 1.0f : 0.0f;
		uniformData.model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 3.0f));
		uniformData.color = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
		memcpy(uniformBuffers[currentBuffer].sphere.mapped, &uniformData, sizeof(uniformData));
	}

	void prepare()
	{
		VulkanExampleBase::prepare();
		loadAssets();
		setupQueryPool();
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

		// Reset query pool
		// Must be done outside of render pass
		vkCmdResetQueryPool(cmdBuffer, queryPool, 0, 2);

		vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = vks::initializers::viewport(
			(float)width,
			(float)height,
			0.0f,
			1.0f);
		vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

		VkRect2D scissor = vks::initializers::rect2D(
			width,
			height,
			0,
			0);
		vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

		glm::mat4 modelMatrix = glm::mat4(1.0f);

		// Occlusion pass
		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.simple);

		// Occluder first
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentBuffer].occluder, 0, nullptr);
		models.plane.draw(cmdBuffer);

		// Teapot
		vkCmdBeginQuery(cmdBuffer, queryPool, 0, VK_FLAGS_NONE);
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentBuffer].teapot, 0, nullptr);
		models.teapot.draw(cmdBuffer);
		vkCmdEndQuery(cmdBuffer, queryPool, 0);

		// Sphere
		vkCmdBeginQuery(cmdBuffer, queryPool, 1, VK_FLAGS_NONE);
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentBuffer].sphere, 0, nullptr);
		models.sphere.draw(cmdBuffer);
		vkCmdEndQuery(cmdBuffer, queryPool, 1);

		// Visible pass
		// Clear color and depth attachments
		VkClearAttachment clearAttachments[2] = {};

		clearAttachments[0].aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		clearAttachments[0].clearValue.color = defaultClearColor;
		clearAttachments[0].colorAttachment = 0;

		clearAttachments[1].aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		clearAttachments[1].clearValue.depthStencil = { 1.0f, 0 };

		VkClearRect clearRect = {};
		clearRect.layerCount = 1;
		clearRect.rect.offset = { 0, 0 };
		clearRect.rect.extent = { width, height };

		vkCmdClearAttachments(
			cmdBuffer,
			2,
			clearAttachments,
			1,
			&clearRect);

		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.solid);

		// Teapot
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentBuffer].teapot, 0, nullptr);
		models.teapot.draw(cmdBuffer);

		// Sphere
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentBuffer].sphere, 0, nullptr);
		models.sphere.draw(cmdBuffer);

		// Occluder
		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.occluder);
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentBuffer].occluder, 0, nullptr);
		models.plane.draw(cmdBuffer);

		drawUI(cmdBuffer);

		vkCmdEndRenderPass(cmdBuffer);

		VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffer));
	}

	virtual void render()
	{
		if (!prepared)
			return;
		VulkanExampleBase::prepareFrame();
		updateUniformBuffers();
		buildCommandBuffer();
		VulkanExampleBase::submitFrame();
		// vkGetQueryResults copies the query results into a host visible buffer that we can display
		vkGetQueryPoolResults(
			device,
			queryPool,
			0,
			2,
			sizeof(passedSamples),
			passedSamples,
			sizeof(uint64_t),
			// Store results a 64 bit values and wait until the results have been finished
			// If you don't want to wait, you can use VK_QUERY_RESULT_WITH_AVAILABILITY_BIT
			// which also returns the state of the result (ready) in the result
			VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
	}

	virtual void OnUpdateUIOverlay(vks::UIOverlay *overlay)
	{
		if (overlay->header("Occlusion query results")) {
			overlay->text("Teapot: %d samples passed", passedSamples[0]);
			overlay->text("Sphere: %d samples passed", passedSamples[1]);
		}
	}

};

VULKAN_EXAMPLE_MAIN()
