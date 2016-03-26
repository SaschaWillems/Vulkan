/*
* Vulkan Demo Scene 
*
* Don't take this a an example, it's more of a personal playground
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* Note : Different license than the other examples!
*
* This code is licensed under the Mozilla Public License Version 2.0 (http://opensource.org/licenses/MPL-2.0)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <vector>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include <vulkan/vulkan.h>
#include "vulkanexamplebase.h"

#define VERTEX_BUFFER_BIND_ID 0
#define ENABLE_VALIDATION false

class VulkanExample : public VulkanExampleBase
{
public:

	struct DemoMeshes
	{
		std::vector<std::string> names{ "logos", "background", "models", "skybox" };
		VkPipelineVertexInputStateCreateInfo inputState;
		std::vector<VkVertexInputBindingDescription> bindingDescriptions;
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
		VkPipeline pipeline;
		VulkanMeshLoader* logos;
		VulkanMeshLoader* background;
		VulkanMeshLoader* models;
		VulkanMeshLoader* skybox;
	} demoMeshes;
	std::vector<VulkanMeshLoader*> meshes;

	struct {
		vkTools::UniformData meshVS;
	} uniformData;

	struct {
		glm::mat4 projection;
		glm::mat4 model;
		glm::mat4 normal;
		glm::mat4 view;
		glm::vec4 lightPos;
	} uboVS;

	struct
	{
		vkTools::VulkanTexture skybox;
	} textures;

	struct {
		VkPipeline logos;
		VkPipeline models;
		VkPipeline skybox;
	} pipelines;

	VkPipelineLayout pipelineLayout;
	VkDescriptorSet descriptorSet;
	VkDescriptorSetLayout descriptorSetLayout;

	glm::vec4 lightPos = glm::vec4(1.0f, 2.0f, 0.0f, 0.0f);

	VulkanExample() : VulkanExampleBase(ENABLE_VALIDATION)
	{
		width = 1280;
		height = 720;
		zoom = -3.75f;
		rotationSpeed = 0.5f;
		rotation = glm::vec3(15.0f, 0.f, 0.0f);
		title = "Vulkan Demo Scene - © 2016 by Sascha Willems";
	}

	~VulkanExample()
	{
		// Clean up used Vulkan resources 
		// Note : Inherited destructor cleans up resources stored in base class
		vkDestroyPipeline(device, pipelines.logos, nullptr);
		vkDestroyPipeline(device, pipelines.models, nullptr);
		vkDestroyPipeline(device, pipelines.skybox, nullptr);

		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

		vkTools::destroyUniformData(device, &uniformData.meshVS);

		for (auto& mesh : meshes)
		{
			vkDestroyBuffer(device, mesh->vertexBuffer.buf, nullptr);
			vkFreeMemory(device, mesh->vertexBuffer.mem, nullptr);

			vkDestroyBuffer(device, mesh->indexBuffer.buf, nullptr);
			vkFreeMemory(device, mesh->indexBuffer.mem, nullptr);
		}

		textureLoader->destroyTexture(textures.skybox);
	}

	void loadTextures()
	{
		textureLoader->loadCubemap(
			getAssetPath() + "textures/cubemap_vulkan.ktx", 
			VK_FORMAT_R8G8B8A8_UNORM, 
			&textures.skybox);
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
			renderPassBeginInfo.framebuffer = frameBuffers[i];

			err = vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo);
			assert(!err);

			vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport viewport = vkTools::initializers::viewport(
				(float)width,
				(float)height,
				0.0f,
				1.0f);
			vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

			VkRect2D scissor = vkTools::initializers::rect2D(
				width,
				height,
				0,
				0);
			vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

			vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);

			VkDeviceSize offsets[1] = { 0 };
			for (auto& mesh : meshes)
			{
				vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, mesh->pipeline);
				vkCmdBindVertexBuffers(drawCmdBuffers[i], VERTEX_BUFFER_BIND_ID, 1, &mesh->vertexBuffer.buf, offsets);
				vkCmdBindIndexBuffer(drawCmdBuffers[i], mesh->indexBuffer.buf, 0, VK_INDEX_TYPE_UINT32);
				vkCmdDrawIndexed(drawCmdBuffers[i], mesh->indexBuffer.count, 1, 0, 0, 0);
			}

			vkCmdEndRenderPass(drawCmdBuffers[i]);

			err = vkEndCommandBuffer(drawCmdBuffers[i]);
			assert(!err);
		}
	}

	void draw()
	{
		VkResult err;

		// Get next image in the swap chain (back/front buffer)
		err = swapChain.acquireNextImage(semaphores.presentComplete, &currentBuffer);
		assert(!err);

		submitPostPresentBarrier(swapChain.buffers[currentBuffer].image);

		// Command buffer to be sumitted to the queue
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];

		// Submit to queue
		err = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
		assert(!err);

		submitPrePresentBarrier(swapChain.buffers[currentBuffer].image);

		err = swapChain.queuePresent(queue, currentBuffer, semaphores.renderComplete);
		assert(!err);

		err = vkQueueWaitIdle(queue);
		assert(!err);
	}

	void prepareVertices()
	{
		struct Vertex {
			float pos[3];
			float normal[3];
			float uv[2];
			float color[3];
		};

		// Load meshes for demos scene
		demoMeshes.logos = new VulkanMeshLoader();
		demoMeshes.background = new VulkanMeshLoader();
		demoMeshes.models = new VulkanMeshLoader();
		demoMeshes.skybox = new VulkanMeshLoader();

#if defined(__ANDROID__)
		demoMeshes.logos->assetManager = androidApp->activity->assetManager;
		demoMeshes.background->assetManager = androidApp->activity->assetManager;
		demoMeshes.models->assetManager = androidApp->activity->assetManager;
		demoMeshes.skybox->assetManager = androidApp->activity->assetManager;
#endif

		demoMeshes.logos->LoadMesh(getAssetPath() + "models/vulkanscenelogos.dae");
		demoMeshes.background->LoadMesh(getAssetPath() + "models/vulkanscenebackground.dae");
		demoMeshes.models->LoadMesh(getAssetPath() + "models/vulkanscenemodels.dae");
		demoMeshes.skybox->LoadMesh(getAssetPath() + "models/cube.obj");

		std::vector<VulkanMeshLoader*> meshList;
		meshList.push_back(demoMeshes.skybox); // skybox first because of depth writes
		meshList.push_back(demoMeshes.logos);
		meshList.push_back(demoMeshes.background);
		meshList.push_back(demoMeshes.models);

		VkMemoryAllocateInfo memAlloc = vkTools::initializers::memoryAllocateInfo();
		VkMemoryRequirements memReqs;

		// todo : Use mesh function for loading
		float scale = 1.0f;
		for (auto& mesh : meshList)
		{
			// Generate vertex buffer (pos, normal, uv, color)
			std::vector<Vertex> vertexBuffer;
			for (int m = 0; m < mesh->m_Entries.size(); m++)
			{
				for (int i = 0; i < mesh->m_Entries[m].Vertices.size(); i++) {
					glm::vec3 pos = mesh->m_Entries[m].Vertices[i].m_pos * scale;
					glm::vec3 normal = mesh->m_Entries[m].Vertices[i].m_normal;
					glm::vec2 uv = mesh->m_Entries[m].Vertices[i].m_tex;
					glm::vec3 col = mesh->m_Entries[m].Vertices[i].m_color;
					Vertex vert = {
						{ pos.x, pos.y, pos.z },
						{ normal.x, -normal.y, normal.z },
						{ uv.s, uv.t },
						{ col.r, col.g, col.b }
					};

					// Offset Vulkan meshes
					// todo : center before export
					if (mesh != demoMeshes.skybox)
					{
						vert.pos[1] += 1.15f;
					}

					vertexBuffer.push_back(vert);
				}
			}
			createBuffer(
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				vertexBuffer.size() * sizeof(Vertex),
				vertexBuffer.data(),
				&mesh->vertexBuffer.buf,
				&mesh->vertexBuffer.mem);

			uint32_t vertexBufferSize = vertexBuffer.size() * sizeof(Vertex);

			std::vector<uint32_t> indexBuffer;
			for (int m = 0; m < mesh->m_Entries.size(); m++)
			{
				int indexBase = indexBuffer.size();
				for (int i = 0; i < mesh->m_Entries[m].Indices.size(); i++) {
					indexBuffer.push_back(mesh->m_Entries[m].Indices[i] + indexBase);
				}
			}
			createBuffer(
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				indexBuffer.size() * sizeof(uint32_t),
				indexBuffer.data(),
				&mesh->indexBuffer.buf,
				&mesh->indexBuffer.mem);
			mesh->indexBuffer.count = indexBuffer.size();

			meshes.push_back(mesh);
		}

		// Binding description
		demoMeshes.bindingDescriptions.resize(1);
		demoMeshes.bindingDescriptions[0] =
			vkTools::initializers::vertexInputBindingDescription(
				VERTEX_BUFFER_BIND_ID,
				sizeof(Vertex),
				VK_VERTEX_INPUT_RATE_VERTEX);

		// Attribute descriptions
		// Location 0 : Position
		demoMeshes.attributeDescriptions.resize(4);
		demoMeshes.attributeDescriptions[0] =
			vkTools::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				0,
				VK_FORMAT_R32G32B32_SFLOAT,
				0);
		// Location 1 : Normal
		demoMeshes.attributeDescriptions[1] =
			vkTools::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				1,
				VK_FORMAT_R32G32B32_SFLOAT,
				sizeof(float) * 3);
		// Location 2 : Texture coordinates
		demoMeshes.attributeDescriptions[2] =
			vkTools::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				2,
				VK_FORMAT_R32G32_SFLOAT,
				sizeof(float) * 6);
		// Location 3 : Color
		demoMeshes.attributeDescriptions[3] =
			vkTools::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				3,
				VK_FORMAT_R32G32B32_SFLOAT,
				sizeof(float) * 8);

		demoMeshes.inputState = vkTools::initializers::pipelineVertexInputStateCreateInfo();
		demoMeshes.inputState.vertexBindingDescriptionCount = demoMeshes.bindingDescriptions.size();
		demoMeshes.inputState.pVertexBindingDescriptions = demoMeshes.bindingDescriptions.data();
		demoMeshes.inputState.vertexAttributeDescriptionCount = demoMeshes.attributeDescriptions.size();
		demoMeshes.inputState.pVertexAttributeDescriptions = demoMeshes.attributeDescriptions.data();
	}

	void setupDescriptorPool()
	{
		// Example uses one ubo and one image sampler
		std::vector<VkDescriptorPoolSize> poolSizes =
		{
			vkTools::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2),
			vkTools::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
		};

		VkDescriptorPoolCreateInfo descriptorPoolInfo =
			vkTools::initializers::descriptorPoolCreateInfo(
				poolSizes.size(),
				poolSizes.data(),
				2);

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
				0),
			// Binding 1 : Fragment shader color map image sampler
			vkTools::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_SHADER_STAGE_FRAGMENT_BIT,
				1)
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

		err = vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &pipelineLayout);
		assert(!err);
	}

	void setupDescriptorSet()
	{
		VkDescriptorSetAllocateInfo allocInfo =
			vkTools::initializers::descriptorSetAllocateInfo(
				descriptorPool,
				&descriptorSetLayout,
				1);

		VkResult vkRes = vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet);
		assert(!vkRes);

		// Cube map image descriptor
		VkDescriptorImageInfo texDescriptorCubeMap =
			vkTools::initializers::descriptorImageInfo(
				textures.skybox.sampler,
				textures.skybox.view,
				VK_IMAGE_LAYOUT_GENERAL);

		std::vector<VkWriteDescriptorSet> writeDescriptorSets =
		{
			// Binding 0 : Vertex shader uniform buffer
			vkTools::initializers::writeDescriptorSet(
				descriptorSet,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				0,
				&uniformData.meshVS.descriptor),
			// Binding 1 : Fragment shader image sampler
			vkTools::initializers::writeDescriptorSet(
				descriptorSet,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				1,
				&texDescriptorCubeMap)
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

		// Pipeline for the meshes (armadillo, bunny, etc.)
		// Load shaders
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;
		shaderStages[0] = loadShader(getAssetPath() + "shaders/vulkanscene/mesh.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getAssetPath() + "shaders/vulkanscene/mesh.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

		VkGraphicsPipelineCreateInfo pipelineCreateInfo =
			vkTools::initializers::pipelineCreateInfo(
				pipelineLayout,
				renderPass,
				0);

		pipelineCreateInfo.pVertexInputState = &demoMeshes.inputState;
		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
		pipelineCreateInfo.pRasterizationState = &rasterizationState;
		pipelineCreateInfo.pColorBlendState = &colorBlendState;
		pipelineCreateInfo.pMultisampleState = &multisampleState;
		pipelineCreateInfo.pViewportState = &viewportState;
		pipelineCreateInfo.pDepthStencilState = &depthStencilState;
		pipelineCreateInfo.pDynamicState = &dynamicState;
		pipelineCreateInfo.stageCount = shaderStages.size();
		pipelineCreateInfo.pStages = shaderStages.data();

		VkResult err = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.models);
		assert(!err);

		// Pipeline for the logos
		shaderStages[0] = loadShader(getAssetPath() + "shaders/vulkanscene/logo.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getAssetPath() + "shaders/vulkanscene/logo.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		err = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.logos);
		assert(!err);

		// Pipeline for the sky sphere (todo)
		rasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT; // Inverted culling
		depthStencilState.depthWriteEnable = VK_FALSE; // No depth writes
		shaderStages[0] = loadShader(getAssetPath() + "shaders/vulkanscene/skybox.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getAssetPath() + "shaders/vulkanscene/skybox.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		err = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.skybox);
		assert(!err);

		// Assign pipelines
		demoMeshes.logos->pipeline = pipelines.logos;
		demoMeshes.models->pipeline = pipelines.models;
		demoMeshes.background->pipeline = pipelines.models;
		demoMeshes.skybox->pipeline = pipelines.skybox;
	}

	// Prepare and initialize uniform buffer containing shader uniforms
	void prepareUniformBuffers()
	{
		// Vertex shader uniform buffer block
		createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			sizeof(uboVS),
			&uboVS,
			&uniformData.meshVS.buffer,
			&uniformData.meshVS.memory,
			&uniformData.meshVS.descriptor);

		updateUniformBuffers();
	}

	void updateUniformBuffers()
	{
		uboVS.projection = glm::perspective(glm::radians(60.0f), (float)width / (float)height, 0.1f, 256.0f);

		uboVS.view = glm::lookAt(
			glm::vec3(0, 0, -zoom),
			glm::vec3(0, 0, 0),
			glm::vec3(0, 1, 0)
			);

		uboVS.model = glm::mat4();
		uboVS.model = glm::rotate(uboVS.model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		uboVS.model = glm::rotate(uboVS.model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		uboVS.model = glm::rotate(uboVS.model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

		uboVS.normal = glm::inverseTranspose(uboVS.view * uboVS.model);

		uboVS.lightPos = lightPos;

		uint8_t *pData;
		VkResult err = vkMapMemory(device, uniformData.meshVS.memory, 0, sizeof(uboVS), 0, (void **)&pData);
		assert(!err);
		memcpy(pData, &uboVS, sizeof(uboVS));
		vkUnmapMemory(device, uniformData.meshVS.memory);
	}

	void prepare()
	{
		VulkanExampleBase::prepare();
		loadTextures();
		prepareVertices();
		prepareUniformBuffers();
		setupDescriptorSetLayout();
		preparePipelines();
		setupDescriptorPool();
		setupDescriptorSet();
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