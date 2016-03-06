/*
* Vulkan Example - Multi threaded command buffer generation and update (using push constants)
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
#include <iomanip> // todo : only for debug

#define GLM_FORCE_RADIANS
#define GLM_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vulkan/vulkan.h>
#include "vulkanexamplebase.h"

#define VERTEX_BUFFER_BIND_ID 0
//#define USE_GLSL
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

	struct {
		vkTools::UniformData vsScene;
	} uniformData;

	struct {
		glm::mat4 projection;
		glm::mat4 view;
		glm::vec4 lightPos = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	} uboVS;

	struct {
		VkPipeline phong;
	} pipelines;

	VkPipelineLayout pipelineLayout;
	VkDescriptorSet descriptorSet;
	VkDescriptorSetLayout descriptorSetLayout;

	// Multi threaded stuff
	// Max. number of concurrent threads
	uint32_t numThreads;

	// Use push constants to update shader
	// parameters on a per-thread base
	struct ThreadPushConstantBlock {
		glm::mat4 model;
		glm::vec3 color;
	};

	struct RenderThread {
		std::thread thread;
		VkCommandPool cmdPool;
		std::vector<VkCommandBuffer> cmdBuffers;
		ThreadPushConstantBlock pushConstantBlock;
	};
	std::vector<RenderThread> renderThreads;

	VulkanExample() : VulkanExampleBase(ENABLE_VALIDATION)
	{
		width = 1280;
		height = 720;
		zoom = -20.0f;
		zoomSpeed = 2.5f;
		rotationSpeed = 0.5f;
		rotation = { 0.0f, 0.0f, 0.0f };
		title = "Vulkan Example - Multi threaded rendering";
		// Get number of max. concurrrent threads
		// todo : May not work on all compilers (e.g. old GCC versions?)
		numThreads = std::thread::hardware_concurrency();
		assert(numThreads > 0);
		// todo : test, remove
		std::cout << "numThreads = " << numThreads << std::endl;
		srand(time(NULL));
	}

	~VulkanExample()
	{
		// Clean up used Vulkan resources 
		// Note : Inherited destructor cleans up resources stored in base class
		vkDestroyPipeline(device, pipelines.phong, nullptr);

		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

		vkTools::destroyUniformData(device, &uniformData.vsScene);

		vkMeshLoader::freeMeshBufferResources(device, &meshes.ufo);

		for (auto& thread : renderThreads)
		{
			vkFreeCommandBuffers(device, thread.cmdPool, thread.cmdBuffers.size(), thread.cmdBuffers.data());
			vkDestroyCommandPool(device, thread.cmdPool, nullptr);
		}
	}

	// Update command buffer and push constants
	void threadUpdate(uint32_t index)
	{
		// todo : Update secondary command buffer
	}

	// Create all threads and initialize shader push constants
	void prepareMultiThreadedRenderer()
	{
		VkResult err;

		renderThreads.resize(numThreads);
		uint32_t index = 0;
		for (auto& thread : renderThreads)
		{
			// Command pool
			VkCommandPoolCreateInfo cmdPoolInfo = vkTools::initializers::commandPoolCreateInfo();
			cmdPoolInfo.queueFamilyIndex = swapChain.queueNodeIndex;
			cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			err = vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &thread.cmdPool);
			assert(!err);

			// Command buffer 
			// Use secondary level command buffers
			thread.cmdBuffers.resize(swapChain.imageCount);
			VkCommandBufferAllocateInfo cmdBufAllocateInfo =
				vkTools::initializers::commandBufferAllocateInfo(
					thread.cmdPool,
					VK_COMMAND_BUFFER_LEVEL_SECONDARY,
					(uint32_t)thread.cmdBuffers.size());

			err = vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, thread.cmdBuffers.data());
			assert(!err);

			// Push constant block

			// Color
			// todo : randomize
			thread.pushConstantBlock.color = glm::vec3(1.0f, 1.0f, 1.0f);
			
			// Model matrix
			float rot = (float)(rand() % 360);
			float deltaT = (float)(rand() % 255) / 255.0f;

			glm::mat4 modelMat = glm::translate(glm::mat4(), glm::vec3((float)index * 4.0f - (float)(numThreads-1) * 2.0f, 0.0f, 0.0f));
			modelMat = glm::rotate(modelMat, -sinf(glm::radians(deltaT * 360.0f)) * 0.25f, glm::vec3(1.0f, 0.0f, 0.0f));
			modelMat = glm::rotate(modelMat, glm::radians(rot), glm::vec3(0.0f, 1.0f, 0.0f));
			modelMat = glm::rotate(modelMat, glm::radians(deltaT * 360.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			thread.pushConstantBlock.model = modelMat;

			thread.thread = std::thread([=] { threadUpdate(index); });
			index++;

			// Viewport and scissor rect are shared
			VkViewport viewport = vkTools::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
			VkRect2D scissor = vkTools::initializers::rect2D(width, height, 0, 0);

			// Fill command buffers
			for (uint32_t i = 0; i < thread.cmdBuffers.size(); ++i)
			{
				// Inheritance infor for secondary command buffers
				VkCommandBufferInheritanceInfo inheritanceInfo = vkTools::initializers::commandBufferInheritanceInfo();
				inheritanceInfo.renderPass = renderPass;
				inheritanceInfo.framebuffer = frameBuffers[i];

				VkCommandBufferBeginInfo beginInfo = vkTools::initializers::commandBufferBeginInfo();
				beginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
				beginInfo.pInheritanceInfo = &inheritanceInfo;

				vkBeginCommandBuffer(thread.cmdBuffers[i], &beginInfo);

				vkCmdSetViewport(thread.cmdBuffers[i], 0, 1, &viewport);
				vkCmdSetScissor(thread.cmdBuffers[i], 0, 1, &scissor);

				vkCmdBindPipeline(thread.cmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.phong);

				// Update shader push constant block
				// Contains model view matrix
				vkCmdPushConstants(
					thread.cmdBuffers[i],
					pipelineLayout,
					VK_SHADER_STAGE_VERTEX_BIT,
					0,
					sizeof(ThreadPushConstantBlock),
					&thread.pushConstantBlock);

				// Render mesh
				VkDeviceSize offsets[1] = { 0 };
				vkCmdBindDescriptorSets(thread.cmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);
				vkCmdBindVertexBuffers(thread.cmdBuffers[i], VERTEX_BUFFER_BIND_ID, 1, &meshes.ufo.vertices.buf, offsets);
				vkCmdBindIndexBuffer(thread.cmdBuffers[i], meshes.ufo.indices.buf, 0, VK_INDEX_TYPE_UINT32);
				vkCmdDrawIndexed(thread.cmdBuffers[i], meshes.ufo.indexCount, 1, 0, 0, 0);

				vkEndCommandBuffer(thread.cmdBuffers[i]);
			}
		}

		for (auto& thread : renderThreads)
		{
			thread.thread.join();
		}

	}

	void buildCommandBuffers()
	{
		VkCommandBufferBeginInfo cmdBufInfo = vkTools::initializers::commandBufferBeginInfo();

		VkClearValue clearValues[2];
		clearValues[0].color = defaultClearColor;
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderPassBeginInfo = vkTools::initializers::renderPassBeginInfo();
		renderPassBeginInfo.renderPass = renderPass;
		renderPassBeginInfo.renderArea.offset.x = 0;
		renderPassBeginInfo.renderArea.offset.y = 0;
		renderPassBeginInfo.renderArea.extent.width = width;
		renderPassBeginInfo.renderArea.extent.height = height;
		renderPassBeginInfo.clearValueCount = 2;
		renderPassBeginInfo.pClearValues = clearValues;

		VkResult err;

		for (int32_t i = 0; i < drawCmdBuffers.size(); ++i)
		{
			// Set target frame buffer
			renderPassBeginInfo.framebuffer = frameBuffers[i];

			err = vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo);
			assert(!err);

			// The primary command buffer does not contain any rendering commands
			// These are stored (and retrieved) from the secondary command buffers

			vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

			// Execute secondary command buffers
			for (auto& renderThread : renderThreads)
			{
				// todo : Make sure threads are finished before accessing their command buffers
				vkCmdExecuteCommands(drawCmdBuffers[i], 1, &renderThread.cmdBuffers[i]);
			}

			vkCmdEndRenderPass(drawCmdBuffers[i]);

			VkImageMemoryBarrier prePresentBarrier = vkTools::prePresentBarrier(swapChain.buffers[i].image);
			vkCmdPipelineBarrier(
				drawCmdBuffers[i],
				VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_FLAGS_NONE,
				0, nullptr,
				0, nullptr,
				1, &prePresentBarrier);

			err = vkEndCommandBuffer(drawCmdBuffers[i]);
			assert(!err);
		}
	}

	void draw()
	{
		VkResult err;
		VkSemaphore presentCompleteSemaphore;
		VkSemaphoreCreateInfo presentCompleteSemaphoreCreateInfo = vkTools::initializers::semaphoreCreateInfo();

		err = vkCreateSemaphore(device, &presentCompleteSemaphoreCreateInfo, nullptr, &presentCompleteSemaphore);
		assert(!err);

		// Get next image in the swap chain (back/front buffer)
		err = swapChain.acquireNextImage(presentCompleteSemaphore, &currentBuffer);
		assert(!err);

		VkSubmitInfo submitInfo = vkTools::initializers::submitInfo();
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &presentCompleteSemaphore;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];

		// Submit draw command buffer
		err = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
		assert(!err);

		err = swapChain.queuePresent(queue, currentBuffer);
		assert(!err);

		vkDestroySemaphore(device, presentCompleteSemaphore, nullptr);

		submitPostPresentBarrier(swapChain.buffers[currentBuffer].image);

		err = vkQueueWaitIdle(queue);
		assert(!err);
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
				sizeof(glm::mat4),
				0);

		// Push constant ranges are part of the pipeline layout
		pPipelineLayoutCreateInfo.pushConstantRangeCount = 1;
		pPipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

		err = vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &pipelineLayout);
		assert(!err);
	}

	void setupDescriptorSets()
	{
		VkDescriptorSetAllocateInfo allocInfo =
			vkTools::initializers::descriptorSetAllocateInfo(
				descriptorPool,
				&descriptorSetLayout,
				1);

		VkResult vkRes = vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet);
		assert(!vkRes);

		std::vector<VkWriteDescriptorSet> writeDescriptorSets =
		{
			// Binding 0 : Vertex shader uniform buffer
			vkTools::initializers::writeDescriptorSet(
				descriptorSet,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				0,
				&uniformData.vsScene.descriptor)
		};

		vkUpdateDescriptorSets(device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);
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

#ifdef USE_GLSL
		shaderStages[0] = loadShaderGLSL("./../data/shaders/multithreading/phong.vert", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShaderGLSL("./../data/shaders/multithreading/phong.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
#else
		shaderStages[0] = loadShader("./../data/shaders/multithreading/phong.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader("./../data/shaders/multithreading/phong.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
#endif

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

	// Prepare and initialize uniform buffer containing shader uniforms
	void prepareUniformBuffers()
	{
		// Vertex shader uniform buffer block
		createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			sizeof(uboVS),
			&uboVS,
			&uniformData.vsScene.buffer,
			&uniformData.vsScene.memory,
			&uniformData.vsScene.descriptor);

		updateUniformBuffers();
	}

	void updateUniformBuffers()
	{
		uboVS.projection = glm::perspective(glm::radians(60.0f), (float)width / (float)height, 0.1f, 256.0f);

		uboVS.view = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, zoom));
		uboVS.view = glm::rotate(uboVS.view, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		uboVS.view = glm::rotate(uboVS.view, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		uboVS.view = glm::rotate(uboVS.view, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

		uint8_t *pData;
		VkResult err = vkMapMemory(device, uniformData.vsScene.memory, 0, sizeof(uboVS), 0, (void **)&pData);
		assert(!err);
		memcpy(pData, &uboVS, sizeof(uboVS));
		vkUnmapMemory(device, uniformData.vsScene.memory);
	}

	void prepare()
	{
		VulkanExampleBase::prepare();
		loadMeshes();
		setupVertexDescriptions();
		prepareUniformBuffers();
		setupDescriptorSetLayout();
		preparePipelines();
		setupDescriptorPool();
		setupDescriptorSets();
		prepareMultiThreadedRenderer();
		buildCommandBuffers();
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
		updateUniformBuffers();
	}
};

VulkanExample *vulkanExample;

#ifdef _WIN32

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (vulkanExample != NULL)
	{
		vulkanExample->handleMessages(hWnd, uMsg, wParam, lParam);
	}
	return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}

#else 

static void handleEvent(const xcb_generic_event_t *event)
{
	if (vulkanExample != NULL)
	{
		vulkanExample->handleEvent(event);
	}
}
#endif

#ifdef _WIN32
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
#else
int main(const int argc, const char *argv[])
#endif
{
	vulkanExample = new VulkanExample();
#ifdef _WIN32
	vulkanExample->setupWindow(hInstance, WndProc);
#else
	vulkanExample->setupWindow();
#endif
	vulkanExample->initSwapchain();
	vulkanExample->prepare();
	vulkanExample->renderLoop();
	delete(vulkanExample);
	return 0;
}
