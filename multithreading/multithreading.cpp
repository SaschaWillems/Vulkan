/*
* Vulkan Example - Multi threaded command buffer generation and rendering
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "vulkanexamplebase.h"

#include <thread>
#include "threadpool.hpp"
#include "frustum.hpp"

#define VERTEX_BUFFER_BIND_ID 0

// Vertex layout used in this example
// Vertex layout for this example
std::vector<vkMeshLoader::VertexLayout> vertexLayout =
{
	vkMeshLoader::VERTEX_LAYOUT_POSITION,
	vkMeshLoader::VERTEX_LAYOUT_NORMAL,
	vkMeshLoader::VERTEX_LAYOUT_COLOR,
};

class VulkanExample : public VulkanExampleBase
{
public:
	struct {
		VkPipelineVertexInputStateCreateInfo inputState;
		std::vector<VkVertexInputBindingDescription> bindingDescriptions;
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
	} vertices;

	struct {
		vkMeshLoader::MeshBuffer ufo;
		vkMeshLoader::MeshBuffer skysphere;
	} meshes;

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
	VkCommandBuffer secondaryCommandBuffer;

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
		vkMeshLoader::MeshBuffer mesh;
		VkCommandPool commandPool;
		// One command buffer per render object
		std::vector<VkCommandBuffer> commandBuffer;
		// One push constant block per render object
		std::vector<ThreadPushConstantBlock> pushConstBlock;
		// Per object information (position, rotation, etc.)
		std::vector<ObjectData> objectData;
	};
	std::vector<ThreadData> threadData;

	vkTools::ThreadPool threadPool;

	// Fence to wait for all command buffers to finish before
	// presenting to the swap chain
	VkFence renderFence = {};

	// Max. dimension of the ufo mesh for use as the sphere
	// radius for frustum culling
	float objectSphereDim;

	// View frustum for culling invisible objects
	vkTools::Frustum frustum;

	VulkanExample() : VulkanExampleBase(ENABLE_VALIDATION)
	{
		zoom = -32.5f;
		zoomSpeed = 2.5f;
		rotationSpeed = 0.5f;
		rotation = { 0.0f, 37.5f, 0.0f };
		enableTextOverlay = true;
		title = "Vulkan Example - Multi threaded rendering";
		// Get number of max. concurrrent threads
		numThreads = std::thread::hardware_concurrency();
		assert(numThreads > 0);
#if defined(__ANDROID__)
		LOGD("numThreads = %d", numThreads);
#else
		std::cout << "numThreads = " << numThreads << std::endl;
#endif
		srand(time(NULL));

		threadPool.setThreadCount(numThreads);

		numObjectsPerThread = 256 / numThreads;
	}

	~VulkanExample()
	{
		// Clean up used Vulkan resources 
		// Note : Inherited destructor cleans up resources stored in base class
		vkDestroyPipeline(device, pipelines.phong, nullptr);
		vkDestroyPipeline(device, pipelines.starsphere, nullptr);

		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

		vkFreeCommandBuffers(device, cmdPool, 1, &primaryCommandBuffer);
		vkFreeCommandBuffers(device, cmdPool, 1, &secondaryCommandBuffer);

		vkMeshLoader::freeMeshBufferResources(device, &meshes.ufo);
		vkMeshLoader::freeMeshBufferResources(device, &meshes.skysphere);

		for (auto& thread : threadData)
		{
			vkFreeCommandBuffers(device, thread.commandPool, thread.commandBuffer.size(), thread.commandBuffer.data());
			vkDestroyCommandPool(device, thread.commandPool, nullptr);
			vkMeshLoader::freeMeshBufferResources(device, &thread.mesh);
		}

		vkDestroyFence(device, renderFence, nullptr);
	}

	float rnd(float range)
	{
		return range * (rand() / double(RAND_MAX));
	}

	// Create all threads and initialize shader push constants
	void prepareMultiThreadedRenderer()
	{
		// Since this demo updates the command buffers on each frame
		// we don't use the per-framebuffer command buffers from the
		// base class, and create a single primary command buffer instead
		VkCommandBufferAllocateInfo cmdBufAllocateInfo =
			vkTools::initializers::commandBufferAllocateInfo(
				cmdPool,
				VK_COMMAND_BUFFER_LEVEL_PRIMARY,
				1);
		VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &primaryCommandBuffer));

		// Create a secondary command buffer for rendering the star sphere
		cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
		VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &secondaryCommandBuffer));
		
		threadData.resize(numThreads);

		createSetupCommandBuffer();

		float maxX = std::floor(std::sqrt(numThreads * numObjectsPerThread));
		uint32_t posX = 0;
		uint32_t posZ = 0;

		for (uint32_t i = 0; i < numThreads; i++)
		{
			ThreadData *thread = &threadData[i];
			
			// Create one command pool for each thread
			VkCommandPoolCreateInfo cmdPoolInfo = vkTools::initializers::commandPoolCreateInfo();
			cmdPoolInfo.queueFamilyIndex = swapChain.queueNodeIndex;
			cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			VK_CHECK_RESULT(vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &thread->commandPool));

			// One secondary command buffer per object that is updated by this thread
			thread->commandBuffer.resize(numObjectsPerThread);
			// Generate secondary command buffers for each thread
			VkCommandBufferAllocateInfo secondaryCmdBufAllocateInfo =
				vkTools::initializers::commandBufferAllocateInfo(
					thread->commandPool,
					VK_COMMAND_BUFFER_LEVEL_SECONDARY,
					thread->commandBuffer.size());
			VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &secondaryCmdBufAllocateInfo, thread->commandBuffer.data()));

			// Unique vertex and index buffers per thread
			createBuffer(
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				meshes.ufo.vertices.size,
				nullptr,
				&thread->mesh.vertices.buf,
				&thread->mesh.vertices.mem);
			createBuffer(
				VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				meshes.ufo.indices.size,
				nullptr,
				&thread->mesh.indices.buf,
				&thread->mesh.indices.mem);

			// Copy from mesh buffer
			VkBufferCopy copyRegion = {};

			// Vertex buffer
			copyRegion.size = meshes.ufo.vertices.size;
			vkCmdCopyBuffer(
				setupCmdBuffer,
				meshes.ufo.vertices.buf,
				thread->mesh.vertices.buf,
				1,
				&copyRegion);
			// Index buffer
			copyRegion.size = meshes.ufo.indices.size;
			vkCmdCopyBuffer(
				setupCmdBuffer,
				meshes.ufo.indices.buf,
				thread->mesh.indices.buf,
				1,
				&copyRegion);

			// todo : staging

			thread->mesh.indexCount = meshes.ufo.indexCount;

			thread->pushConstBlock.resize(numObjectsPerThread);
			thread->objectData.resize(numObjectsPerThread);

			float step = 360.0f / (float)(numThreads * numObjectsPerThread);
			for (uint32_t j = 0; j < numObjectsPerThread; j++)
			{
				float radius = 8.0f + rnd(8.0f) - rnd(4.0f);

				thread->objectData[j].pos.x = (posX - maxX / 2.0f) * 3.0f + rnd(1.5f) - rnd(1.5f);
				thread->objectData[j].pos.z = (posZ - maxX / 2.0f) * 3.0f + rnd(1.5f) - rnd(1.5f);

				posX += 1.0f;
				if (posX >= maxX)
				{
					posX = 0.0f;
					posZ += 1.0f;
				}

				thread->objectData[j].rotation = glm::vec3(0.0f, rnd(360.0f), 0.0f);
				thread->objectData[j].deltaT = rnd(1.0f);
				thread->objectData[j].rotationDir = (rnd(100.0f) < 50.0f) ? 1.0f : -1.0f;
				thread->objectData[j].rotationSpeed = (2.0f + rnd(4.0f)) * thread->objectData[j].rotationDir;
				thread->objectData[j].scale = 0.75f + rnd(0.5f);

				thread->pushConstBlock[j].color = glm::vec3(rnd(1.0f), rnd(1.0f), rnd(1.0f));
			}
		}
		
		// Submit buffer copies to the queue
		flushSetupCommandBuffer();
		// todo : fence?
	}

	// Builds the secondary command buffer for each thread
	void threadRenderCode(uint32_t threadIndex, uint32_t cmdBufferIndex, VkCommandBufferInheritanceInfo inheritanceInfo)
	{
		ThreadData *thread = &threadData[threadIndex];
		ObjectData *objectData = &thread->objectData[cmdBufferIndex];

		// Check visibility against view frustum
		objectData->visible = frustum.checkSphere(objectData->pos, objectSphereDim * 0.5f); 

		if (!objectData->visible)
		{
			return;
		}

		VkCommandBufferBeginInfo commandBufferBeginInfo = vkTools::initializers::commandBufferBeginInfo();
		commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
		commandBufferBeginInfo.pInheritanceInfo = &inheritanceInfo;

		VkCommandBuffer cmdBuffer = thread->commandBuffer[cmdBufferIndex];

		VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &commandBufferBeginInfo));

		VkViewport viewport = vkTools::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
		vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

		VkRect2D scissor = vkTools::initializers::rect2D(width, height, 0, 0);
		vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.phong);

		// Update
		objectData->rotation.y += 2.5f * objectData->rotationSpeed * frameTimer;
		if (objectData->rotation.y > 360.0f)
		{
			objectData->rotation.y -= 360.0f;
		}
		objectData->deltaT += 0.15f * frameTimer;
		if (objectData->deltaT > 1.0f)
			objectData->deltaT -= 1.0f;
		objectData->pos.y = sin(glm::radians(objectData->deltaT * 360.0f)) * 2.5f;

		objectData->model = glm::translate(glm::mat4(), objectData->pos);
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
		vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &thread->mesh.vertices.buf, offsets);
		vkCmdBindIndexBuffer(cmdBuffer, thread->mesh.indices.buf, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(cmdBuffer, thread->mesh.indexCount, 1, 0, 0, 0);

		VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffer));
	}

	void updateSecondaryCommandBuffer(VkCommandBufferInheritanceInfo inheritanceInfo)
	{
		// Secondary command buffer for the sky sphere
		VkCommandBufferBeginInfo commandBufferBeginInfo = vkTools::initializers::commandBufferBeginInfo();
		commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
		commandBufferBeginInfo.pInheritanceInfo = &inheritanceInfo;

		VK_CHECK_RESULT(vkBeginCommandBuffer(secondaryCommandBuffer, &commandBufferBeginInfo));

		VkViewport viewport = vkTools::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
		vkCmdSetViewport(secondaryCommandBuffer, 0, 1, &viewport);

		VkRect2D scissor = vkTools::initializers::rect2D(width, height, 0, 0);
		vkCmdSetScissor(secondaryCommandBuffer, 0, 1, &scissor);

		vkCmdBindPipeline(secondaryCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.starsphere);


		glm::mat4 view = glm::mat4();
		view = glm::rotate(view, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		view = glm::rotate(view, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		view = glm::rotate(view, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

		glm::mat4 mvp = matrices.projection * view;

		vkCmdPushConstants(
			secondaryCommandBuffer,
			pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT,
			0,
			sizeof(mvp),
			&mvp);

		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(secondaryCommandBuffer, 0, 1, &meshes.skysphere.vertices.buf, offsets);
		vkCmdBindIndexBuffer(secondaryCommandBuffer, meshes.skysphere.indices.buf, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(secondaryCommandBuffer, meshes.skysphere.indexCount, 1, 0, 0, 0);

		VK_CHECK_RESULT(vkEndCommandBuffer(secondaryCommandBuffer));
	}

	// Updates the secondary command buffers using a thread pool 
	// and puts them into the primary command buffer that's 
	// lat submitted to the queue for rendering
	void updateCommandBuffers(VkFramebuffer frameBuffer)
	{
		VkCommandBufferBeginInfo cmdBufInfo = vkTools::initializers::commandBufferBeginInfo();

		VkClearValue clearValues[2];
		clearValues[0].color = defaultClearColor;
		clearValues[0].color = { {0.0f, 0.0f, 0.2f, 0.0f} };
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderPassBeginInfo = vkTools::initializers::renderPassBeginInfo();
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
		VkCommandBufferInheritanceInfo inheritanceInfo = vkTools::initializers::commandBufferInheritanceInfo();
		inheritanceInfo.renderPass = renderPass;
		// Secondary command buffer also use the currently active framebuffer
		inheritanceInfo.framebuffer = frameBuffer;

		// Contains the list of secondary command buffers to be executed
		std::vector<VkCommandBuffer> commandBuffers;

		// Secondary command buffer with star background sphere
		updateSecondaryCommandBuffer(inheritanceInfo);
		commandBuffers.push_back(secondaryCommandBuffer);

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

		// Execute render commands from the secondary command buffer
		vkCmdExecuteCommands(primaryCommandBuffer, commandBuffers.size(), commandBuffers.data());

		vkCmdEndRenderPass(primaryCommandBuffer);

		VK_CHECK_RESULT(vkEndCommandBuffer(primaryCommandBuffer));
	}

	void loadMeshes()
	{
		loadMesh(getAssetPath() + "models/retroufo_red_lowpoly.dae", &meshes.ufo, vertexLayout, 0.12f);
		loadMesh(getAssetPath() + "models/sphere.obj", &meshes.skysphere, vertexLayout, 1.0f);
		objectSphereDim = std::max(std::max(meshes.ufo.dim.x, meshes.ufo.dim.y), meshes.ufo.dim.z);
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
		// Location 3 : Color
		vertices.attributeDescriptions[2] =
			vkTools::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				2,
				VK_FORMAT_R32G32B32_SFLOAT,
				sizeof(float) * 6);

		vertices.inputState = vkTools::initializers::pipelineVertexInputStateCreateInfo();
		vertices.inputState.vertexBindingDescriptionCount = vertices.bindingDescriptions.size();
		vertices.inputState.pVertexBindingDescriptions = vertices.bindingDescriptions.data();
		vertices.inputState.vertexAttributeDescriptionCount = vertices.attributeDescriptions.size();
		vertices.inputState.pVertexAttributeDescriptions = vertices.attributeDescriptions.data();
	}

	void setupPipelineLayout()
	{
		VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo =
			vkTools::initializers::pipelineLayoutCreateInfo(nullptr, 0);

		// Push constants for model matrices
		VkPushConstantRange pushConstantRange =
			vkTools::initializers::pushConstantRange(
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
				dynamicStateEnables.size(),
				0);

		// Solid rendering pipeline
		// Load shaders
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

		shaderStages[0] = loadShader(getAssetPath() + "shaders/multithreading/phong.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getAssetPath() + "shaders/multithreading/phong.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

		VkGraphicsPipelineCreateInfo pipelineCreateInfo =
			vkTools::initializers::pipelineCreateInfo(
				pipelineLayout,
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
		pipelineCreateInfo.stageCount = shaderStages.size();
		pipelineCreateInfo.pStages = shaderStages.data();

		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.phong));

		// Star sphere rendering pipeline
		rasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;
		depthStencilState.depthWriteEnable = VK_FALSE;
		shaderStages[0] = loadShader(getAssetPath() + "shaders/multithreading/starsphere.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getAssetPath() + "shaders/multithreading/starsphere.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.starsphere));
	}

	void updateMatrices()
	{
		matrices.projection = glm::perspective(glm::radians(60.0f), (float)width / (float)height, 0.1f, 256.0f);
		matrices.view = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, zoom));
		matrices.view = glm::rotate(matrices.view, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		matrices.view = glm::rotate(matrices.view, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		matrices.view = glm::rotate(matrices.view, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

		frustum.update(matrices.projection * matrices.view);
	}

	void draw()
	{
		VulkanExampleBase::prepareFrame();

		updateCommandBuffers(frameBuffers[currentBuffer]);

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &primaryCommandBuffer;

		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, renderFence));

		// Wait for fence to signal that all command buffers are ready
		VkResult fenceRes;
		do
		{
			fenceRes = vkWaitForFences(device, 1, &renderFence, VK_TRUE, 100000000);
		} while (fenceRes == VK_TIMEOUT);
		VK_CHECK_RESULT(fenceRes);
		vkResetFences(device, 1, &renderFence);

		VulkanExampleBase::submitFrame();
	}

	void prepare()
	{
		VulkanExampleBase::prepare();
		// Create a fence for synchronization
		VkFenceCreateInfo fenceCreateInfo = vkTools::initializers::fenceCreateInfo(VK_FLAGS_NONE);
		vkCreateFence(device, &fenceCreateInfo, NULL, &renderFence);
		loadMeshes();
		setupVertexDescriptions();
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
	}

	virtual void viewChanged()
	{
		updateMatrices();
	}

	virtual void getOverlayText(VulkanTextOverlay *textOverlay)
	{
		textOverlay->addText("Using " + std::to_string(numThreads) + " threads", 5.0f, 85.0f, VulkanTextOverlay::alignLeft);
	}
};

RUN_EXAMPLE(VulkanExample)
