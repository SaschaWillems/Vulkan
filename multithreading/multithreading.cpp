/*
* Vulkan Example - Multi threaded command buffer generation and rendering
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
#include <thread>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vulkan/vulkan.h>
#include "vulkanexamplebase.h"

#include "threadpool.hpp"

#define VERTEX_BUFFER_BIND_ID 0
#define ENABLE_VALIDATION false

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
	} meshes;

	// Shared matrices used for thread push constant blocks
	struct {
		glm::mat4 projection;
		glm::mat4 view;
	} matrices;

	struct {
		VkPipeline phong;
	} pipelines;

	VkPipelineLayout pipelineLayout;
	VkDescriptorSet descriptorSet;
	VkDescriptorSetLayout descriptorSetLayout;

	VkCommandBuffer primaryCommandBuffer;

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

	struct MeshData {
		glm::vec3 pos;
		glm::vec3 rotation;
		float deltaT;
		vkMeshLoader::MeshBufferInfo vertices;
		vkMeshLoader::MeshBufferInfo indices;
		uint32_t indexCount;
	};

	struct ThreadData {
		MeshData meshData;
		VkCommandPool commandPool;
		std::vector<VkCommandBuffer> commandBuffer;
		ThreadPushConstantBlock pushConstBlock;
	};
	std::vector<ThreadData> threadData;

	vkTools::ThreadPool threadPool;

	VulkanExample() : VulkanExampleBase(ENABLE_VALIDATION)
	{
		width = 1280;
		height = 720;
		zoom = -35.0f;
		zoomSpeed = 2.5f;
		rotationSpeed = 0.5f;
		rotation = { -16.0f, -32.0f, 0.0f };
		title = "Vulkan Example - Multi threaded rendering";
		// Get number of max. concurrrent threads
		// todo : May not work on all compilers (e.g. old GCC versions?)
		numThreads = std::thread::hardware_concurrency();
		assert(numThreads > 0);
		// todo : test, remove
		std::cout << "numThreads = " << numThreads << std::endl;
		srand(time(NULL));
		//numThreads *= 4; // todo : test

		threadPool.setThreadCount(numThreads);

		// Render 32 animated objects
		numObjectsPerThread = 32 / numThreads;
	}

	~VulkanExample()
	{
		// Clean up used Vulkan resources 
		// Note : Inherited destructor cleans up resources stored in base class
		vkDestroyPipeline(device, pipelines.phong, nullptr);

		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

		vkFreeCommandBuffers(device, cmdPool, 1, &primaryCommandBuffer);

		vkMeshLoader::freeMeshBufferResources(device, &meshes.ufo);

		for (auto& thread : threadData)
		{
			vkFreeCommandBuffers(device, thread.commandPool, thread.commandBuffer.size(), thread.commandBuffer.data());
			vkDestroyCommandPool(device, thread.commandPool, nullptr);
		}
	}

	// Create all threads and initialize shader push constants
	void prepareMultiThreadedRenderer()
	{
		// Since this demo updates the command buffers on each frame
		// we don't use the per-framebuffer command buffers from the
		// base class, and create a single primary command buffer instead
		VkCommandBufferAllocateInfo primaryAllocateInfo =
			vkTools::initializers::commandBufferAllocateInfo(
				cmdPool,
				VK_COMMAND_BUFFER_LEVEL_PRIMARY,
				1);
		vkTools::checkResult(vkAllocateCommandBuffers(device, &primaryAllocateInfo, &primaryCommandBuffer));

		threadData.resize(numThreads);

		createSetupCommandBuffer();

		for (uint32_t i = 0; i < numThreads; i++)
		{
			ThreadData *thread = &threadData[i];
			

			// Create one command pool for each thread
			VkCommandPoolCreateInfo cmdPoolInfo = vkTools::initializers::commandPoolCreateInfo();
			cmdPoolInfo.queueFamilyIndex = swapChain.queueNodeIndex;
			cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			vkTools::checkResult(vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &thread->commandPool));

			// One secondary command buffer per object that is updated by this thread
			thread->commandBuffer.resize(numObjectsPerThread);
			// Generate secondary command buffers for each thread
			VkCommandBufferAllocateInfo cmdBufAllocateInfo =
				vkTools::initializers::commandBufferAllocateInfo(
					thread->commandPool,
					VK_COMMAND_BUFFER_LEVEL_SECONDARY,
					thread->commandBuffer.size());
			vkTools::checkResult(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, thread->commandBuffer.data()));

			// Unique vertex and index buffers per thread

			createBuffer(
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				meshes.ufo.vertices.size,
				nullptr,
				&thread->meshData.vertices.buf,
				&thread->meshData.vertices.mem);
			createBuffer(
				VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				meshes.ufo.indices.size,
				nullptr,
				&thread->meshData.indices.buf,
				&thread->meshData.indices.mem);

			// Copy from mesh buffer
			VkBufferCopy copyRegion = {};

			// Vertex buffer
			copyRegion.size = meshes.ufo.vertices.size;
			vkCmdCopyBuffer(
				setupCmdBuffer,
				meshes.ufo.vertices.buf,
				thread->meshData.vertices.buf,
				1,
				&copyRegion);
			// Index buffer
			copyRegion.size = meshes.ufo.indices.size;
			vkCmdCopyBuffer(
				setupCmdBuffer,
				meshes.ufo.indices.buf,
				thread->meshData.indices.buf,
				1,
				&copyRegion);

			thread->meshData.indexCount = meshes.ufo.indexCount;

			float step = 360.0f / (float)numThreads;
			float radius = 20.0f;
			thread->meshData.pos.x = sin(glm::radians(step * i)) * radius;
			thread->meshData.pos.z = cos(glm::radians(step * i)) * radius;
			thread->meshData.rotation = glm::vec3(0.0f, (float)(rand() % 360), 0.0f);
			thread->meshData.deltaT = (float)(rand() % 255) / 255.0f;
		}
		
		// Submit buffer copies to the queue
		flushSetupCommandBuffer();
		// todo : fence?
	}

	// Builds the secondary command buffer for each thread
	void threadRenderCode(uint32_t threadIndex, uint32_t cmdBufferIndex, VkCommandBufferInheritanceInfo inheritanceInfo)
	{
		VkCommandBufferBeginInfo commandBufferBeginInfo = vkTools::initializers::commandBufferBeginInfo();
		commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
		commandBufferBeginInfo.pInheritanceInfo = &inheritanceInfo;

		ThreadData *thread = &threadData[threadIndex];
		VkCommandBuffer cmdBuffer = thread->commandBuffer[cmdBufferIndex];

		vkTools::checkResult(vkBeginCommandBuffer(cmdBuffer, &commandBufferBeginInfo));

		VkViewport viewport = vkTools::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
		vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

		VkRect2D scissor = vkTools::initializers::rect2D(width, height, 0, 0);
		vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.phong);

		// Update
		// todo : timebased
		thread->meshData.rotation.y += 0.15f;
		if (thread->meshData.rotation.y > 360.0f)
			thread->meshData.rotation.y -= 360.0f;
		thread->meshData.deltaT += 0.0005f;
		if (thread->meshData.deltaT > 1.0f)
			thread->meshData.deltaT -= 1.0f;
		thread->meshData.pos.y = sin(glm::radians(thread->meshData.deltaT * 360.0f)) * 1.5f;

		glm::mat4 model = glm::translate(glm::mat4(), thread->meshData.pos);
		model = glm::rotate(model, -sinf(glm::radians(thread->meshData.deltaT * 360.0f)) * 0.25f, glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(thread->meshData.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, glm::radians(thread->meshData.deltaT * 360.0f), glm::vec3(0.0f, 1.0f, 0.0f));

		thread->pushConstBlock.mvp = matrices.projection * matrices.view * model;

		// Update shader push constant block
		// Contains model view matrix
		vkCmdPushConstants(
			cmdBuffer,
			pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT,
			0,
			sizeof(ThreadPushConstantBlock),
			&thread->pushConstBlock);

		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &thread->meshData.vertices.buf, offsets);
		vkCmdBindIndexBuffer(cmdBuffer, thread->meshData.indices.buf, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(cmdBuffer, thread->meshData.indexCount, 1, 0, 0, 0);

		vkTools::checkResult(vkEndCommandBuffer(cmdBuffer));
	}

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

		vkTools::checkResult(vkBeginCommandBuffer(primaryCommandBuffer, &cmdBufInfo));

		// The primary command buffer does not contain any rendering commands
		// These are stored (and retrieved) from the secondary command buffers
		vkCmdBeginRenderPass(primaryCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

		std::vector<VkCommandBuffer> commandBuffers;

		// Inheritance info for the secondary command buffers
		VkCommandBufferInheritanceInfo inheritanceInfo = vkTools::initializers::commandBufferInheritanceInfo();
		inheritanceInfo.renderPass = renderPass;
		// Secondary command buffer also use the currently active framebuffer
		inheritanceInfo.framebuffer = frameBuffer;

		for (uint32_t t = 0; t < numThreads; t++)
		{
			threadPool.threads[t]->addJob([=] { threadRenderCode(t, 0, inheritanceInfo); });
			commandBuffers.push_back(threadData[t].commandBuffer[0]);
		}
			
		threadPool.wait();

		// Execute render commands from the secondary command buffer
		vkCmdExecuteCommands(primaryCommandBuffer, commandBuffers.size(), commandBuffers.data());

		vkCmdEndRenderPass(primaryCommandBuffer);

		vkTools::checkResult(vkEndCommandBuffer(primaryCommandBuffer));
	}

	void draw()
	{
		// Get next image in the swap chain (back/front buffer)
		vkTools::checkResult(swapChain.acquireNextImage(semaphores.presentComplete, &currentBuffer));

		submitPostPresentBarrier(swapChain.buffers[currentBuffer].image);

		updateCommandBuffers(frameBuffers[currentBuffer]);

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &primaryCommandBuffer;

		// Setup a wait fence
		// todo : reuse
		VkFence renderFence = {};
		VkFenceCreateInfo fenceCreateInfo = vkTools::initializers::fenceCreateInfo(VK_FLAGS_NONE);
		vkCreateFence(device, &fenceCreateInfo, NULL, &renderFence);

		vkTools::checkResult(vkQueueSubmit(queue, 1, &submitInfo, renderFence));

		// Wait for fence to signal that all command buffers are ready
		VkResult fenceRes;
		do 
		{
			// todo : timeout as define
			fenceRes = vkWaitForFences(device, 1, &renderFence, VK_TRUE, 100000000);
		} while (fenceRes == VK_TIMEOUT);
		vkTools::checkResult(fenceRes);

		submitPrePresentBarrier(swapChain.buffers[currentBuffer].image);

		vkTools::checkResult(swapChain.queuePresent(queue, currentBuffer, semaphores.renderComplete));

		vkDestroyFence(device, renderFence, nullptr);

		vkTools::checkResult(vkQueueWaitIdle(queue));
	}

	void loadMeshes()
	{
		loadMesh("./../data/models/retroufo_red.X", &meshes.ufo, vertexLayout, 0.25f);
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

	void setupDescriptorPool()
	{
		std::vector<VkDescriptorPoolSize> poolSizes =
		{
			vkTools::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3)
		};

		VkDescriptorPoolCreateInfo descriptorPoolInfo =
			vkTools::initializers::descriptorPoolCreateInfo(
				poolSizes.size(),
				poolSizes.data(),
				3);

		VkResult vkRes = vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool);
		assert(!vkRes);
	}

	void setupDescriptorSet()
	{
		// todo :
		VkDescriptorSetAllocateInfo allocInfo =
			vkTools::initializers::descriptorSetAllocateInfo(
				descriptorPool,
				&descriptorSetLayout,
				1);

		vkTools::checkResult(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));

		vkUpdateDescriptorSets(device, 0, nullptr, 0, nullptr);
	}

	void setupDescriptorSetLayout()
	{
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings =
		{
			// Binding 0 : Vertex shader uniform buffer
			vkTools::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				VK_SHADER_STAGE_VERTEX_BIT,
				0)
		};

		VkDescriptorSetLayoutCreateInfo descriptorLayout =
			vkTools::initializers::descriptorSetLayoutCreateInfo(
				setLayoutBindings.data(),
				setLayoutBindings.size());

		VkResult err = vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayout);
		assert(!err);

		VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo =
			vkTools::initializers::pipelineLayoutCreateInfo(
				&descriptorSetLayout,
				1);

		// Push constants for model matrices
		VkPushConstantRange pushConstantRange =
			vkTools::initializers::pushConstantRange(
				VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
				sizeof(ThreadPushConstantBlock),
				0);

		// Push constant ranges are part of the pipeline layout
		pPipelineLayoutCreateInfo.pushConstantRangeCount = 1;
		pPipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

		err = vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &pipelineLayout);
		assert(!err);
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

		shaderStages[0] = loadShader("./../data/shaders/multithreading/phong.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader("./../data/shaders/multithreading/phong.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

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

		VkResult err = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.phong);
		assert(!err);
	}

	void updateMatrices()
	{
		matrices.projection = glm::perspective(glm::radians(60.0f), (float)width / (float)height, 0.1f, 256.0f);
		matrices.view = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, zoom));
		matrices.view = glm::rotate(matrices.view, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		matrices.view = glm::rotate(matrices.view, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		matrices.view = glm::rotate(matrices.view, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
	}

	void prepare()
	{
		VulkanExampleBase::prepare();
		loadMeshes();
		setupVertexDescriptions();
		setupDescriptorSetLayout();
		preparePipelines();
		setupDescriptorPool();
		setupDescriptorSet();
		prepareMultiThreadedRenderer();
		updateMatrices();
		prepared = true;
	}

	virtual void render()
	{
		if (!prepared)
			return;
		vkDeviceWaitIdle(device);
		draw();
		vkDeviceWaitIdle(device);
	}

	virtual void viewChanged()
	{
		updateMatrices();
	}
};

VulkanExample *vulkanExample;

#if defined(_WIN32)
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (vulkanExample != NULL)
	{
		vulkanExample->handleMessages(hWnd, uMsg, wParam, lParam);
	}
	return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}
#elif defined(__linux__) && !defined(__ANDROID__)
static void handleEvent(const xcb_generic_event_t *event)
{
	if (vulkanExample != NULL)
	{
		vulkanExample->handleEvent(event);
	}
}
#endif

// Main entry point
#if defined(_WIN32)
// Windows entry point
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
#elif defined(__ANDROID__)
// Android entry point
void android_main(android_app* state)
#elif defined(__linux__)
// Linux entry point
int main(const int argc, const char *argv[])
#endif
{
#if defined(__ANDROID__)
	// Removing this may cause the compiler to omit the main entry point 
	// which would make the application crash at start
	app_dummy();
#endif
	vulkanExample = new VulkanExample();
#if defined(_WIN32)
	vulkanExample->setupWindow(hInstance, WndProc);
#elif defined(__ANDROID__)
	// Attach vulkan example to global android application state
	state->userData = vulkanExample;
	state->onAppCmd = VulkanExample::handleAppCommand;
	state->onInputEvent = VulkanExample::handleAppInput;
	vulkanExample->androidApp = state;
#elif defined(__linux__)
	vulkanExample->setupWindow();
#endif
#if !defined(__ANDROID__)
	vulkanExample->initSwapchain();
	vulkanExample->prepare();
#endif
	vulkanExample->renderLoop();
	delete(vulkanExample);
#if !defined(__ANDROID__)
	return 0;
#endif
}