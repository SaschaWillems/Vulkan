/*
* Vulkan Example - Retrieving pipeline statistics
*
* Copyright (C) 2017 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "vulkanexamplebase.h"
#include "VulkanglTFModel.h"

#define ENABLE_VALIDATION false
#define OBJ_DIM 0.05f

class VulkanExample : public VulkanExampleBase
{
public:
	struct Models {
		std::vector<vkglTF::Model> objects;
		int32_t objectIndex = 3;
		std::vector<std::string> names;
	} models;

	struct UniformBuffers {
		vks::Buffer VS;
	} uniformBuffers;

	struct UBOVS {
		glm::mat4 projection;
		glm::mat4 modelview;
		glm::vec4 lightPos = glm::vec4(-10.0f, -10.0f, 10.0f, 1.0f);
	} uboVS;

	VkPipeline pipeline = VK_NULL_HANDLE;

	int32_t cullMode = VK_CULL_MODE_BACK_BIT;
	bool blending = false;
	bool discard = false;
	bool wireframe = false;
	bool tessellation = false;

	VkPipelineLayout pipelineLayout;
	VkDescriptorSet descriptorSet;
	VkDescriptorSetLayout descriptorSetLayout;

	VkQueryPool queryPool;

	// Vector for storing pipeline statistics results
	std::vector<uint64_t> pipelineStats;
	std::vector<std::string> pipelineStatNames;

	int32_t gridSize = 3;

	VulkanExample() : VulkanExampleBase(ENABLE_VALIDATION)
	{
		title = "Pipeline statistics";
		camera.type = Camera::CameraType::firstperson;
		camera.setPosition(glm::vec3(-3.0f, 1.0f, -2.75f));
		camera.setRotation(glm::vec3(-15.25f, -46.5f, 0.0f));
		camera.movementSpeed = 4.0f;
		camera.setPerspective(60.0f, (float)width / (float)height, 0.1f, 256.0f);
		camera.rotationSpeed = 0.25f;
	}

	~VulkanExample()
	{
		vkDestroyPipeline(device, pipeline, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
		vkDestroyQueryPool(device, queryPool, nullptr);
		uniformBuffers.VS.destroy();
	}

	virtual void getEnabledFeatures()
	{
		// Support for pipeline statistics is optional
		if (deviceFeatures.pipelineStatisticsQuery) {
			enabledFeatures.pipelineStatisticsQuery = VK_TRUE;
		}
		else {
			vks::tools::exitFatal("Selected GPU does not support pipeline statistics!", VK_ERROR_FEATURE_NOT_PRESENT);
		}
		if (deviceFeatures.fillModeNonSolid) {
			enabledFeatures.fillModeNonSolid = VK_TRUE;
		}
		if (deviceFeatures.tessellationShader) {
			enabledFeatures.tessellationShader = VK_TRUE;
		}
	}

	// Setup a query pool for storing pipeline statistics
	void setupQueryPool()
	{
		pipelineStatNames = {
			"Input assembly vertex count        ",
			"Input assembly primitives count    ",
			"Vertex shader invocations          ",
			"Clipping stage primitives processed",
			"Clipping stage primitives output    ",
			"Fragment shader invocations        "
		};
		if (deviceFeatures.tessellationShader) {
			pipelineStatNames.push_back("Tess. control shader patches       ");
			pipelineStatNames.push_back("Tess. eval. shader invocations     ");
		}
		pipelineStats.resize(pipelineStatNames.size());

		VkQueryPoolCreateInfo queryPoolInfo = {};
		queryPoolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
		// This query pool will store pipeline statistics
		queryPoolInfo.queryType = VK_QUERY_TYPE_PIPELINE_STATISTICS;
		// Pipeline counters to be returned for this pool
		queryPoolInfo.pipelineStatistics =
			VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT |
			VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT |
			VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT |
			VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT |
			VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT |
			VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT;
		if (deviceFeatures.tessellationShader) {
			queryPoolInfo.pipelineStatistics |=
				VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_CONTROL_SHADER_PATCHES_BIT |
				VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT;
		}
		queryPoolInfo.queryCount = deviceFeatures.tessellationShader ? 8 : 6;
		VK_CHECK_RESULT(vkCreateQueryPool(device, &queryPoolInfo, NULL, &queryPool));
	}

	// Retrieves the results of the pipeline statistics query submitted to the command buffer
	void getQueryResults()
	{
		uint32_t count = static_cast<uint32_t>(pipelineStats.size());
		vkGetQueryPoolResults(
			device,
			queryPool,
			0,
			1,
			count * sizeof(uint64_t),
			pipelineStats.data(),
			sizeof(uint64_t),
			VK_QUERY_RESULT_64_BIT);
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

		for (int32_t i = 0; i < drawCmdBuffers.size(); ++i) {
			renderPassBeginInfo.framebuffer = frameBuffers[i];

			VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo));

			// Reset timestamp query pool
			vkCmdResetQueryPool(drawCmdBuffers[i], queryPool, 0, static_cast<uint32_t>(pipelineStats.size()));

			vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport viewport = vks::initializers::viewport((float)width,	(float)height, 0.0f, 1.0f);
			vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

			VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);
			vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

			VkDeviceSize offsets[1] = { 0 };

			// Start capture of pipeline statistics
			vkCmdBeginQuery(drawCmdBuffers[i], queryPool, 0, 0);

			vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
			vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);
			vkCmdBindVertexBuffers(drawCmdBuffers[i], 0, 1, &models.objects[models.objectIndex].vertices.buffer, offsets);
			vkCmdBindIndexBuffer(drawCmdBuffers[i], models.objects[models.objectIndex].indices.buffer, 0, VK_INDEX_TYPE_UINT32);

			for (int32_t y = 0; y < gridSize; y++) {
				for (int32_t x = 0; x < gridSize; x++) {
					glm::vec3 pos = glm::vec3(float(x - (gridSize / 2.0f)) * 2.5f, 0.0f, float(y - (gridSize / 2.0f)) * 2.5f);
					vkCmdPushConstants(drawCmdBuffers[i], pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::vec3), &pos);
					models.objects[models.objectIndex].draw(drawCmdBuffers[i]);
				}
			}

			// End capture of pipeline statistics
			vkCmdEndQuery(drawCmdBuffers[i], queryPool, 0);

			drawUI(drawCmdBuffers[i]);

			vkCmdEndRenderPass(drawCmdBuffers[i]);

			VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
		}
	}

	void draw()
	{
		VulkanExampleBase::prepareFrame();

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

		// Read query results for displaying in next frame
		getQueryResults();

		VulkanExampleBase::submitFrame();
	}

	void loadAssets()
	{
		// Objects
		std::vector<std::string> filenames = { "sphere.gltf", "teapot.gltf", "torusknot.gltf", "venus.gltf" };
		models.names = { "Sphere", "Teapot", "Torusknot", "Venus" };
		models.objects.resize(filenames.size());
		for (size_t i = 0; i < filenames.size(); i++) {
			models.objects[i].loadFromFile(getAssetPath() + "models/" + filenames[i], vulkanDevice, queue, vkglTF::FileLoadingFlags::PreTransformVertices | vkglTF::FileLoadingFlags::FlipY);
		}
	}

	void setupDescriptorPool()
	{
		std::vector<VkDescriptorPoolSize> poolSizes = {
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3)
		};
		VkDescriptorPoolCreateInfo descriptorPoolInfo =
			vks::initializers::descriptorPoolCreateInfo(poolSizes, 3);
		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));
	}

	void setupDescriptorSetLayout()
	{
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0)
		};
		VkDescriptorSetLayoutCreateInfo descriptorLayout =
			vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayout));

		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo =
			vks::initializers::pipelineLayoutCreateInfo(&descriptorSetLayout, 1);
		VkPushConstantRange pushConstantRange = vks::initializers::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::vec3), 0);
		pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
		pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));
	}

	void setupDescriptorSets()
	{
		VkDescriptorSetAllocateInfo allocInfo =
			vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));
		std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
			vks::initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffers.VS.descriptor)
		};
		vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, NULL);
	}

	void preparePipelines()
	{
		if (pipeline != VK_NULL_HANDLE) {
			vkDestroyPipeline(device, pipeline, nullptr);
		}

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
		VkPipelineRasterizationStateCreateInfo rasterizationState = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, cullMode, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
		VkPipelineColorBlendAttachmentState blendAttachmentState = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);		
		VkPipelineColorBlendStateCreateInfo colorBlendState = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
		VkPipelineDepthStencilStateCreateInfo depthStencilState = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE,	VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
		VkPipelineViewportStateCreateInfo viewportState = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
		VkPipelineMultisampleStateCreateInfo multisampleState = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
		std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicState = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables.data(), static_cast<uint32_t>(dynamicStateEnables.size()), 0);
		VkPipelineTessellationStateCreateInfo tessellationState = vks::initializers::pipelineTessellationStateCreateInfo(3);

		VkGraphicsPipelineCreateInfo pipelineCI = vks::initializers::pipelineCreateInfo(pipelineLayout, renderPass, 0);
		pipelineCI.pInputAssemblyState = &inputAssemblyState;
		pipelineCI.pRasterizationState = &rasterizationState;
		pipelineCI.pColorBlendState = &colorBlendState;
		pipelineCI.pMultisampleState = &multisampleState;
		pipelineCI.pViewportState = &viewportState;
		pipelineCI.pDepthStencilState = &depthStencilState;
		pipelineCI.pDynamicState = &dynamicState;
		pipelineCI.pVertexInputState = vkglTF::Vertex::getPipelineVertexInputState({ vkglTF::VertexComponent::Position, vkglTF::VertexComponent::Normal, vkglTF::VertexComponent::Color });

		if (blending) {
			blendAttachmentState.blendEnable = VK_TRUE;
			blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
			blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
			depthStencilState.depthWriteEnable = VK_FALSE;
		}

		if (discard) {
			rasterizationState.rasterizerDiscardEnable = VK_TRUE;
		}

		if (wireframe) {
			rasterizationState.polygonMode = VK_POLYGON_MODE_LINE;
		}

		std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
		shaderStages.resize(tessellation ? 4 : 2);
		shaderStages[0] = loadShader(getShadersPath() + "pipelinestatistics/scene.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getShadersPath() + "pipelinestatistics/scene.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

		if (tessellation) {
			inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
			pipelineCI.pTessellationState = &tessellationState;
			shaderStages[2] = loadShader(getShadersPath() + "pipelinestatistics/scene.tesc.spv", VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);
			shaderStages[3] = loadShader(getShadersPath() + "pipelinestatistics/scene.tese.spv", VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);
		}

		pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineCI.pStages = shaderStages.data();
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipeline));
	}

	// Prepare and initialize uniform buffer containing shader uniforms
	void prepareUniformBuffers()
	{
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&uniformBuffers.VS,
			sizeof(uboVS)));

		// Map persistent
		VK_CHECK_RESULT(uniformBuffers.VS.map());

		updateUniformBuffers();
	}

	void updateUniformBuffers()
	{
		uboVS.projection = camera.matrices.perspective;
		uboVS.modelview = camera.matrices.view;
		memcpy(uniformBuffers.VS.mapped, &uboVS, sizeof(uboVS));
	}

	void prepare()
	{
		VulkanExampleBase::prepare();
		loadAssets();
		setupQueryPool();
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
	}

	virtual void viewChanged()
	{
		updateUniformBuffers();
	}

	virtual void OnUpdateUIOverlay(vks::UIOverlay *overlay)
	{
		if (overlay->header("Settings")) {
			if (overlay->comboBox("Object type", &models.objectIndex, models.names)) {
				updateUniformBuffers();
				buildCommandBuffers();
			}
			if (overlay->sliderInt("Grid size", &gridSize, 1, 10)) {
				buildCommandBuffers();
			}
			std::vector<std::string> cullModeNames = { "None", "Front", "Back", "Back and front" };
			if (overlay->comboBox("Cull mode", &cullMode, cullModeNames)) {
				preparePipelines();
				buildCommandBuffers();
			}
			if (overlay->checkBox("Blending", &blending)) {
				preparePipelines();
				buildCommandBuffers();
			}
			if (deviceFeatures.fillModeNonSolid) {
				if (overlay->checkBox("Wireframe", &wireframe)) {
					preparePipelines();
					buildCommandBuffers();
				}
			}
			if (deviceFeatures.tessellationShader) {
				if (overlay->checkBox("Tessellation", &tessellation)) {
					preparePipelines();
					buildCommandBuffers();
				}
			}
			if (overlay->checkBox("Discard", &discard)) {
				preparePipelines();
				buildCommandBuffers();
			}
		}
		if (!pipelineStats.empty()) {
			if (overlay->header("Pipeline statistics")) {
				for (auto i = 0; i < pipelineStats.size(); i++) {
					std::string caption = pipelineStatNames[i] + ": %d";
					overlay->text(caption.c_str(), pipelineStats[i]);
				}
			}
		}
	}

};

VULKAN_EXAMPLE_MAIN()