/*
* Vulkan Example - Multi threaded command buffer generation and rendering
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "vulkanexamplebase.h"

#include "threadpool.hpp"
#include "frustum.hpp"

#include "VulkanglTFModel.h"

#define ENABLE_VALIDATION false

class VulkanExample : public VulkanExampleBase
{
public:
	bool displayStarSphere = true;

	struct {
		vkglTF::Model ufo;
		vkglTF::Model starSphere;
	} models;

	// Shared matrices used for thread push constant blocks
	struct {
		glm::mat4 projection;
		glm::mat4 view;
	} matrices;

	struct {
		VkPipeline phong;
		VkPipeline starsphere;
	} pipelines;

	VkPipelineLayout pipelineLayout;

	VkCommandBuffer primaryCommandBuffer;

	// Secondary scene command buffers used to store backdrop and user interface
	struct SecondaryCommandBuffers {
		VkCommandBuffer background;
		VkCommandBuffer ui;
	} secondaryCommandBuffers;

	// Number of animated objects to be renderer
	// by using threads and secondary command buffers
	uint32_t numObjectsPerThread;

	// Multi threaded stuff
	// Max. number of concurrent threads
	uint32_t numThreads;

	// Use push constants to update shader
	// parameters on a per-thread base
	struct ThreadPushConstantBlock {
		glm::mat4 mvp;
		glm::vec3 color;
	};

	struct ObjectData {
		glm::mat4 model;
		glm::vec3 pos;
		glm::vec3 rotation;
		float rotationDir;
		float rotationSpeed;
		float scale;
		float deltaT;
		float stateT = 0;
		bool visible = true;
	};

	struct ThreadData {
		VkCommandPool commandPool;
		// One command buffer per render object
		std::vector<VkCommandBuffer> commandBuffer;
		// One push constant block per render object
		std::vector<ThreadPushConstantBlock> pushConstBlock;
		// Per object information (position, rotation, etc.)
		std::vector<ObjectData> objectData;
	};
	std::vector<ThreadData> threadData;

	vks::ThreadPool threadPool;

	// Fence to wait for all command buffers to finish before
	// presenting to the swap chain
	VkFence renderFence = {};

	// View frustum for culling invisible objects
	vks::Frustum frustum;

	std::default_random_engine rndEngine;

	VulkanExample() : VulkanExampleBase(ENABLE_VALIDATION)
	{
		title = "Multi threaded command buffer";
		camera.type = Camera::CameraType::lookat;
		camera.setPosition(glm::vec3(0.0f, -0.0f, -32.5f));
		camera.setRotation(glm::vec3(0.0f));
		camera.setRotationSpeed(0.5f);
		camera.setPerspective(60.0f, (float)width / (float)height, 0.1f, 256.0f);
		// Get number of max. concurrent threads
		numThreads = std::thread::hardware_concurrency();
		assert(numThreads > 0);
#if defined(__ANDROID__)
		LOGD("numThreads = %d", numThreads);
#else
		std::cout << "numThreads = " << numThreads << std::endl;
#endif
		threadPool.setThreadCount(numThreads);
		numObjectsPerThread = 512 / numThreads;
		rndEngine.seed(benchmark.active ? 0 : (unsigned)time(nullptr));
	}

	~VulkanExample()
	{
		// Clean up used Vulkan resources
		// Note : Inherited destructor cleans up resources stored in base class
		vkDestroyPipeline(device, pipelines.phong, nullptr);
		vkDestroyPipeline(device, pipelines.starsphere, nullptr);

		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

		for (auto& thread : threadData) {
			vkFreeCommandBuffers(device, thread.commandPool, thread.commandBuffer.size(), thread.commandBuffer.data());
			vkDestroyCommandPool(device, thread.commandPool, nullptr);
		}

		vkDestroyFence(device, renderFence, nullptr);
	}

	float rnd(float range)
	{
		std::uniform_real_distribution<float> rndDist(0.0f, range);
		return rndDist(rndEngine);
	}

	// Create all threads and initialize shader push constants
	void prepareMultiThreadedRenderer()
	{
		// Since this demo updates the command buffers on each frame
		// we don't use the per-framebuffer command buffers from the
		// base class, and create a single primary command buffer instead
		VkCommandBufferAllocateInfo cmdBufAllocateInfo =
			vks::initializers::commandBufferAllocateInfo(
				cmdPool,
				VK_COMMAND_BUFFER_LEVEL_PRIMARY,
				1);
		VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &primaryCommandBuffer));

		// Create additional secondary CBs for background and ui
		cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
		VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &secondaryCommandBuffers.background));
		VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &secondaryCommandBuffers.ui));

		threadData.resize(numThreads);

		float maxX = std::floor(std::sqrt(numThreads * numObjectsPerThread));
		uint32_t posX = 0;
		uint32_t posZ = 0;

		for (uint32_t i = 0; i < numThreads; i++) {
			ThreadData *thread = &threadData[i];

			// Create one command pool for each thread
			VkCommandPoolCreateInfo cmdPoolInfo = vks::initializers::commandPoolCreateInfo();
			cmdPoolInfo.queueFamilyIndex = swapChain.queueNodeIndex;
			cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			VK_CHECK_RESULT(vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &thread->commandPool));

			// One secondary command buffer per object that is updated by this thread
			thread->commandBuffer.resize(numObjectsPerThread);
			// Generate secondary command buffers for each thread
			VkCommandBufferAllocateInfo secondaryCmdBufAllocateInfo =
				vks::initializers::commandBufferAllocateInfo(
					thread->commandPool,
					VK_COMMAND_BUFFER_LEVEL_SECONDARY,
					thread->commandBuffer.size());
			VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &secondaryCmdBufAllocateInfo, thread->commandBuffer.data()));

			thread->pushConstBlock.resize(numObjectsPerThread);
			thread->objectData.resize(numObjectsPerThread);

			for (uint32_t j = 0; j < numObjectsPerThread; j++) {
				float theta = 2.0f * float(M_PI) * rnd(1.0f);
				float phi = acos(1.0f - 2.0f * rnd(1.0f));
				thread->objectData[j].pos = glm::vec3(sin(phi) * cos(theta), 0.0f, cos(phi)) * 35.0f;

				thread->objectData[j].rotation = glm::vec3(0.0f, rnd(360.0f), 0.0f);
				thread->objectData[j].deltaT = rnd(1.0f);
				thread->objectData[j].rotationDir = (rnd(100.0f) < 50.0f) ? 1.0f : -1.0f;
				thread->objectData[j].rotationSpeed = (2.0f + rnd(4.0f)) * thread->objectData[j].rotationDir;
				thread->objectData[j].scale = 0.75f + rnd(0.5f);

				thread->pushConstBlock[j].color = glm::vec3(rnd(1.0f), rnd(1.0f), rnd(1.0f));
			}
		}

	}

	// Builds the secondary command buffer for each thread
	void threadRenderCode(uint32_t threadIndex, uint32_t cmdBufferIndex, VkCommandBufferInheritanceInfo inheritanceInfo)
	{
		ThreadData *thread = &threadData[threadIndex];
		ObjectData *objectData = &thread->objectData[cmdBufferIndex];

		// Check visibility against view frustum using a simple sphere check based on the radius of the mesh
		objectData->visible = frustum.checkSphere(objectData->pos, models.ufo.dimensions.radius * 0.5f);

		if (!objectData->visible)
		{
			return;
		}

		VkCommandBufferBeginInfo commandBufferBeginInfo = vks::initializers::commandBufferBeginInfo();
		commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
		commandBufferBeginInfo.pInheritanceInfo = &inheritanceInfo;

		VkCommandBuffer cmdBuffer = thread->commandBuffer[cmdBufferIndex];

		VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &commandBufferBeginInfo));

		VkViewport viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
		vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

		VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);
		vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.phong);

		// Update
		if (!paused) {
			objectData->rotation.y += 2.5f * objectData->rotationSpeed * frameTimer;
			if (objectData->rotation.y > 360.0f) {
				objectData->rotation.y -= 360.0f;
			}
			objectData->deltaT += 0.15f * frameTimer;
			if (objectData->deltaT > 1.0f)
				objectData->deltaT -= 1.0f;
			objectData->pos.y = sin(glm::radians(objectData->deltaT * 360.0f)) * 2.5f;
		}

		objectData->model = glm::translate(glm::mat4(1.0f), objectData->pos);
		objectData->model = glm::rotate(objectData->model, -sinf(glm::radians(objectData->deltaT * 360.0f)) * 0.25f, glm::vec3(objectData->rotationDir, 0.0f, 0.0f));
		objectData->model = glm::rotate(objectData->model, glm::radians(objectData->rotation.y), glm::vec3(0.0f, objectData->rotationDir, 0.0f));
		objectData->model = glm::rotate(objectData->model, glm::radians(objectData->deltaT * 360.0f), glm::vec3(0.0f, objectData->rotationDir, 0.0f));
		objectData->model = glm::scale(objectData->model, glm::vec3(objectData->scale));

		thread->pushConstBlock[cmdBufferIndex].mvp = matrices.projection * matrices.view * objectData->model;

		// Update shader push constant block
		// Contains model view matrix
		vkCmdPushConstants(
			cmdBuffer,
			pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT,
			0,
			sizeof(ThreadPushConstantBlock),
			&thread->pushConstBlock[cmdBufferIndex]);

		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &models.ufo.vertices.buffer, offsets);
		vkCmdBindIndexBuffer(cmdBuffer, models.ufo.indices.buffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(cmdBuffer, models.ufo.indices.count, 1, 0, 0, 0);

		VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffer));
	}

	void updateSecondaryCommandBuffers(VkCommandBufferInheritanceInfo inheritanceInfo)
	{
		// Secondary command buffer for the sky sphere
		VkCommandBufferBeginInfo commandBufferBeginInfo = vks::initializers::commandBufferBeginInfo();
		commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
		commandBufferBeginInfo.pInheritanceInfo = &inheritanceInfo;

		VkViewport viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
		VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);

		/*
			Background
		*/

		VK_CHECK_RESULT(vkBeginCommandBuffer(secondaryCommandBuffers.background, &commandBufferBeginInfo));

		vkCmdSetViewport(secondaryCommandBuffers.background, 0, 1, &viewport);
		vkCmdSetScissor(secondaryCommandBuffers.background, 0, 1, &scissor);

		vkCmdBindPipeline(secondaryCommandBuffers.background, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.starsphere);

		glm::mat4 mvp = matrices.projection * matrices.view;
		mvp[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		mvp = glm::scale(mvp, glm::vec3(2.0f));

		vkCmdPushConstants(
			secondaryCommandBuffers.background,
			pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT,
			0,
			sizeof(mvp),
			&mvp);

		models.starSphere.draw(secondaryCommandBuffers.background);
		
		VK_CHECK_RESULT(vkEndCommandBuffer(secondaryCommandBuffers.background));

		/*
			User interface

			With VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS, the primary command buffer's content has to be defined
			by secondary command buffers, which also applies to the UI overlay command buffer
		*/

		VK_CHECK_RESULT(vkBeginCommandBuffer(secondaryCommandBuffers.ui, &commandBufferBeginInfo));

		vkCmdSetViewport(secondaryCommandBuffers.ui, 0, 1, &viewport);
		vkCmdSetScissor(secondaryCommandBuffers.ui, 0, 1, &scissor);

		vkCmdBindPipeline(secondaryCommandBuffers.ui, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.starsphere);

		if (settings.overlay) {
			drawUI(secondaryCommandBuffers.ui);
		}

		VK_CHECK_RESULT(vkEndCommandBuffer(secondaryCommandBuffers.ui));
	}

	// Updates the secondary command buffers using a thread pool
	// and puts them into the primary command buffer that's
	// lat submitted to the queue for rendering
	void updateCommandBuffers(VkFramebuffer frameBuffer)
	{
		// Contains the list of secondary command buffers to be submitted
		std::vector<VkCommandBuffer> commandBuffers;

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
		renderPassBeginInfo.framebuffer = frameBuffer;

		// Set target frame buffer

		VK_CHECK_RESULT(vkBeginCommandBuffer(primaryCommandBuffer, &cmdBufInfo));

		// The primary command buffer does not contain any rendering commands
		// These are stored (and retrieved) from the secondary command buffers
		vkCmdBeginRenderPass(primaryCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

		// Inheritance info for the secondary command buffers
		VkCommandBufferInheritanceInfo inheritanceInfo = vks::initializers::commandBufferInheritanceInfo();
		inheritanceInfo.renderPass = renderPass;
		// Secondary command buffer also use the currently active framebuffer
		inheritanceInfo.framebuffer = frameBuffer;

		// Update secondary sene command buffers
		updateSecondaryCommandBuffers(inheritanceInfo);

		if (displayStarSphere) {
			commandBuffers.push_back(secondaryCommandBuffers.background);
		}

		// Add a job to the thread's queue for each object to be rendered
		for (uint32_t t = 0; t < numThreads; t++)
		{
			for (uint32_t i = 0; i < numObjectsPerThread; i++)
			{
				threadPool.threads[t]->addJob([=] { threadRenderCode(t, i, inheritanceInfo); });
			}
		}

		threadPool.wait();

		// Only submit if object is within the current view frustum
		for (uint32_t t = 0; t < numThreads; t++)
		{
			for (uint32_t i = 0; i < numObjectsPerThread; i++)
			{
				if (threadData[t].objectData[i].visible)
				{
					commandBuffers.push_back(threadData[t].commandBuffer[i]);
				}
			}
		}

		// Render ui last
		if (UIOverlay.visible) {
			commandBuffers.push_back(secondaryCommandBuffers.ui);
		}

		// Execute render commands from the secondary command buffer
		vkCmdExecuteCommands(primaryCommandBuffer, commandBuffers.size(), commandBuffers.data());

		vkCmdEndRenderPass(primaryCommandBuffer);

		VK_CHECK_RESULT(vkEndCommandBuffer(primaryCommandBuffer));
	}

	void loadAssets()
	{
		const uint32_t glTFLoadingFlags = vkglTF::FileLoadingFlags::PreTransformVertices | vkglTF::FileLoadingFlags::PreMultiplyVertexColors | vkglTF::FileLoadingFlags::FlipY;
		models.ufo.loadFromFile(getAssetPath() + "models/retroufo_red_lowpoly.gltf",vulkanDevice, queue,glTFLoadingFlags);
		models.starSphere.loadFromFile(getAssetPath() + "models/sphere.gltf", vulkanDevice, queue, glTFLoadingFlags);
	}

	void setupPipelineLayout()
	{
		VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo =
			vks::initializers::pipelineLayoutCreateInfo(nullptr, 0);

		// Push constants for model matrices
		VkPushConstantRange pushConstantRange =
			vks::initializers::pushConstantRange(
				VK_SHADER_STAGE_VERTEX_BIT,
				sizeof(ThreadPushConstantBlock),
				0);

		// Push constant ranges are part of the pipeline layout
		pPipelineLayoutCreateInfo.pushConstantRangeCount = 1;
		pPipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &pipelineLayout));
	}

	void preparePipelines()
	{
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
		VkPipelineRasterizationStateCreateInfo rasterizationState = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
		VkPipelineColorBlendAttachmentState blendAttachmentState = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
		VkPipelineColorBlendStateCreateInfo colorBlendState = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
		VkPipelineDepthStencilStateCreateInfo depthStencilState = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
		VkPipelineViewportStateCreateInfo viewportState = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
		VkPipelineMultisampleStateCreateInfo multisampleState = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
		std::vector<VkDynamicState> dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
		VkPipelineDynamicStateCreateInfo dynamicState = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

		VkGraphicsPipelineCreateInfo pipelineCI = vks::initializers::pipelineCreateInfo(pipelineLayout, renderPass, 0);
		pipelineCI.pInputAssemblyState = &inputAssemblyState;
		pipelineCI.pRasterizationState = &rasterizationState;
		pipelineCI.pColorBlendState = &colorBlendState;
		pipelineCI.pMultisampleState = &multisampleState;
		pipelineCI.pViewportState = &viewportState;
		pipelineCI.pDepthStencilState = &depthStencilState;
		pipelineCI.pDynamicState = &dynamicState;
		pipelineCI.stageCount = shaderStages.size();
		pipelineCI.pStages = shaderStages.data();
		pipelineCI.pVertexInputState = vkglTF::Vertex::getPipelineVertexInputState({vkglTF::VertexComponent::Position, vkglTF::VertexComponent::Normal, vkglTF::VertexComponent::Color});

		// Object rendering pipeline
		shaderStages[0] = loadShader(getShadersPath() + "multithreading/phong.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getShadersPath() + "multithreading/phong.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.phong));

		// Star sphere rendering pipeline
		rasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;
		depthStencilState.depthWriteEnable = VK_FALSE;
		shaderStages[0] = loadShader(getShadersPath() + "multithreading/starsphere.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getShadersPath() + "multithreading/starsphere.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.starsphere));
	}

	void updateMatrices()
	{
		matrices.projection = camera.matrices.perspective;
		matrices.view = camera.matrices.view;
		frustum.update(matrices.projection * matrices.view);
	}

	void draw()
	{
		// Wait for fence to signal that all command buffers are ready
		VkResult fenceRes;
		do {
			fenceRes = vkWaitForFences(device, 1, &renderFence, VK_TRUE, 100000000);
		} while (fenceRes == VK_TIMEOUT);
		VK_CHECK_RESULT(fenceRes);
		vkResetFences(device, 1, &renderFence);

		VulkanExampleBase::prepareFrame();

		updateCommandBuffers(frameBuffers[currentBuffer]);

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &primaryCommandBuffer;

		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, renderFence));

		VulkanExampleBase::submitFrame();
	}

	void prepare()
	{
		VulkanExampleBase::prepare();
		// Create a fence for synchronization
		VkFenceCreateInfo fenceCreateInfo = vks::initializers::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
		vkCreateFence(device, &fenceCreateInfo, nullptr, &renderFence);
		loadAssets();
		setupPipelineLayout();
		preparePipelines();
		prepareMultiThreadedRenderer();
		updateMatrices();
		prepared = true;
	}

	virtual void render()
	{
		if (!prepared)
			return;
		draw();
		if (camera.updated)
		{
			updateMatrices();
		}
	}

	virtual void OnUpdateUIOverlay(vks::UIOverlay *overlay)
	{
		if (overlay->header("Statistics")) {
			overlay->text("Active threads: %d", numThreads);
		}
		if (overlay->header("Settings")) {
			overlay->checkBox("Stars", &displayStarSphere);
		}

	}
};

VULKAN_EXAMPLE_MAIN()