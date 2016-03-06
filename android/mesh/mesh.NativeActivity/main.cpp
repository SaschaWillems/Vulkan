/*
* Vulkan Example - Mesh rendering
*
* Uses tiny obj loader (https://github.com/syoyo/tinyobjloader) by syoyo
*
* Note :
*	This is a basic android example. It may be integrated into the other examples at some point in the future.
*	Until then this serves as a starting point for using Vulkan on Android, with some of the functionality required
*	already moved to the example base classes (e.g. swap chain)
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include <assert.h>
#include "vulkanandroid.h"
#include "vulkanswapchain.hpp" 
#include <android/asset_manager.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#define GLM_FORCE_RADIANS
#define GLM_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "AndroidProject1.NativeActivity", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "AndroidProject1.NativeActivity", __VA_ARGS__))

#define VERTEX_BUFFER_BIND_ID 0

struct saved_state {
	glm::vec3 rotation;
	float zoom;
};

struct VulkanExample
{
	struct android_app* app;

	int animating;
	uint32_t width;
	uint32_t height;
	struct saved_state state;

	// Vulkan
	struct Vertex {
		glm::vec3 pos;
		glm::vec3 normal;
		glm::vec3 color;
	};

	VkInstance instance;
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	VulkanSwapChain swapChain;
	VkQueue queue;
	VkCommandPool cmdPool;
	VkRenderPass renderPass;
	VkPipelineCache pipelineCache;
	VkDescriptorPool descriptorPool;
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSet descriptorSet;
	VkPipelineLayout pipelineLayout;
	std::vector<VkCommandBuffer> drawCmdBuffers;
	VkCommandBuffer postPresentCmdBuffer = VK_NULL_HANDLE;
	VkCommandBuffer setupCmdBuffer = VK_NULL_HANDLE;
	VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
	std::vector<VkShaderModule> shaderModules;

	struct {
		VkBuffer buf;
		VkDeviceMemory mem;
		VkPipelineVertexInputStateCreateInfo inputState;
		std::vector<VkVertexInputBindingDescription> bindingDescriptions;
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
	} vertices;

	struct {
		int count;
		VkBuffer buf;
		VkDeviceMemory mem;
	} indices;

	struct {
		VkBuffer buffer;
		VkDeviceMemory memory;
		VkDescriptorBufferInfo descriptor;
	}  uniformDataVS;

	struct {
		glm::mat4 projection;
		glm::mat4 model;
		glm::vec4 lightPos = glm::vec4(0.0f, 0.0f, 10.0f, 1.0f);
	} uboVS;

	struct {
		VkPipeline solid;
	} pipelines;

	uint32_t currentBuffer = 0;

	struct
	{
		VkImage image;
		VkDeviceMemory mem;
		VkImageView view;
	} depthStencil;

	std::vector<VkFramebuffer>frameBuffers;

	bool prepared = false;

	VkBool32 getMemoryType(uint32_t typeBits, VkFlags properties, uint32_t * typeIndex)
	{
		for (uint32_t i = 0; i < 32; i++)
		{
			if ((typeBits & 1) == 1)
			{
				if ((deviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
				{
					*typeIndex = i;
					return true;
				}
			}
			typeBits >>= 1;
		}
		return false;
	}

	VkShaderModule loadShaderModule(const char *fileName, VkShaderStageFlagBits stage)
	{
		// Load shader from compressed asset
		AAsset* asset = AAssetManager_open(app->activity->assetManager, fileName, AASSET_MODE_STREAMING);
		assert(asset);
		size_t size = AAsset_getLength(asset);
		assert(size > 0);

		char *shaderCode = new char[size];
		AAsset_read(asset, shaderCode, size);
		AAsset_close(asset);

		VkShaderModule shaderModule;
		VkShaderModuleCreateInfo moduleCreateInfo;
		VkResult err;

		moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		moduleCreateInfo.pNext = NULL;

		moduleCreateInfo.codeSize = size;
		moduleCreateInfo.pCode = (uint32_t*)shaderCode;
		moduleCreateInfo.flags = 0;
		err = vkCreateShaderModule(device, &moduleCreateInfo, NULL, &shaderModule);
		assert(!err);

		return shaderModule;
	}

	VkPipelineShaderStageCreateInfo loadShader(const char * fileName, VkShaderStageFlagBits stage)
	{
		VkPipelineShaderStageCreateInfo shaderStage = {};
		shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStage.stage = stage;
		shaderStage.module = loadShaderModule(fileName, stage);
		shaderStage.pName = "main";
		assert(shaderStage.module != NULL);
		shaderModules.push_back(shaderStage.module);
		return shaderStage;
	}

	void initVulkan()
	{
		prepared = false;

		bool libLoaded = loadVulkanLibrary();
		assert(libLoaded);

		VkResult vkRes;

		// Instance
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Vulkan Android Example";
		appInfo.applicationVersion = 1;
		appInfo.pEngineName = "VulkanAndroidExample";
		appInfo.engineVersion = 1;
		// todo : Workaround to support implementations that are not using the latest SDK
		appInfo.apiVersion = VK_MAKE_VERSION(1, 0, 1);

		VkInstanceCreateInfo instanceCreateInfo = {};
		instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceCreateInfo.pApplicationInfo = &appInfo;

		vkRes = vkCreateInstance(&instanceCreateInfo, NULL, &instance);
		assert(vkRes == VK_SUCCESS);

		loadVulkanFunctions(instance);

		// Device
		// Always use first physical device
		uint32_t gpuCount;
		vkRes = vkEnumeratePhysicalDevices(instance, &gpuCount, &physicalDevice);
		assert(vkRes == VK_SUCCESS);

		// Find a queue that supports graphics operations
		uint32_t graphicsQueueIndex = 0;
		uint32_t queueCount;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, NULL);
		assert(queueCount >= 1);

		std::vector<VkQueueFamilyProperties> queueProps;
		queueProps.resize(queueCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, queueProps.data());

		for (graphicsQueueIndex = 0; graphicsQueueIndex < queueCount; graphicsQueueIndex++)
		{
			if (queueProps[graphicsQueueIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT)
				break;
		}
		assert(graphicsQueueIndex < queueCount);

		// Request the queue
		float queuePriorities = 0.0f;
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = graphicsQueueIndex;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriorities;

		// Create device
		VkDeviceCreateInfo deviceCreateInfo = {};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.queueCreateInfoCount = 1;
		deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;

		vkRes = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device);
		assert(vkRes == VK_SUCCESS);

		// Get graphics queue
		vkGetDeviceQueue(device, graphicsQueueIndex, 0, &queue);

		// Device memory properties (for finding appropriate memory types)
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &deviceMemoryProperties);

		// Swap chain
		swapChain.connect(instance, physicalDevice, device);
		swapChain.initSurface(app->window);

		// Command buffer pool
		VkCommandPoolCreateInfo cmdPoolInfo = {};
		cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolInfo.queueFamilyIndex = swapChain.queueNodeIndex;
		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		vkRes = vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &cmdPool);
		assert(!vkRes);

		// Pipeline cache
		VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
		pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		VkResult err = vkCreatePipelineCache(device, &pipelineCacheCreateInfo, nullptr, &pipelineCache);
		assert(!err);

		createSetupCommandBuffer();
		startSetupCommandBuffer();

		swapChain.create(setupCmdBuffer, &width, &height);

		setupDepthStencil();
		setupRenderPass();
		setupFrameBuffer();

		flushSetupCommandBuffer();

		createCommandBuffers();

		prepareVertices();
		prepareUniformBuffers();
		setupDescriptorSetLayout();
		preparePipelines();
		setupDescriptorPool();
		setupDescriptorSet();

		buildCommandBuffers();

		state.zoom = -5.0f;
		state.rotation = glm::vec3();

		prepared = true;
	}

	void cleanupVulkan()
	{
		prepared = false;
		vkDestroyPipeline(device, pipelines.solid, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
		vkDestroyBuffer(device, vertices.buf, nullptr);
		vkFreeMemory(device, vertices.mem, nullptr);
		vkDestroyBuffer(device, indices.buf, nullptr);
		vkFreeMemory(device, indices.mem, nullptr);
		vkDestroyBuffer(device, uniformDataVS.buffer, nullptr);
		vkFreeMemory(device, uniformDataVS.memory, nullptr);

		swapChain.cleanup();

		vkDestroyDescriptorPool(device, descriptorPool, nullptr);
		if (setupCmdBuffer != VK_NULL_HANDLE)
		{
			vkFreeCommandBuffers(device, cmdPool, 1, &setupCmdBuffer);

		}
		vkFreeCommandBuffers(device, cmdPool, drawCmdBuffers.size(), drawCmdBuffers.data());
		vkFreeCommandBuffers(device, cmdPool, 1, &postPresentCmdBuffer);

		vkDestroyRenderPass(device, renderPass, nullptr);
		for (uint32_t i = 0; i < frameBuffers.size(); i++)
		{
			vkDestroyFramebuffer(device, frameBuffers[i], nullptr);
		}

		for (auto& shaderModule : shaderModules)
		{
			vkDestroyShaderModule(device, shaderModule, nullptr);
		}

		vkDestroyImageView(device, depthStencil.view, nullptr);
		vkDestroyImage(device, depthStencil.image, nullptr);
		vkFreeMemory(device, depthStencil.mem, nullptr);

		vkDestroyPipelineCache(device, pipelineCache, nullptr);
		vkDestroyDevice(device, nullptr);
		vkDestroyInstance(instance, nullptr);

		freeVulkanLibrary();
	}

	void createSetupCommandBuffer()
	{
		VkCommandBufferAllocateInfo cmdBufAllocateInfo =
			vkTools::initializers::commandBufferAllocateInfo(
				cmdPool,
				VK_COMMAND_BUFFER_LEVEL_PRIMARY,
				1);

		VkResult vkRes = vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &setupCmdBuffer);
		assert(!vkRes);
	}

	void startSetupCommandBuffer()
	{
		VkCommandBufferBeginInfo cmdBufInfo = vkTools::initializers::commandBufferBeginInfo();
		vkBeginCommandBuffer(setupCmdBuffer, &cmdBufInfo);
	}

	void flushSetupCommandBuffer()
	{
		VkResult err;

		if (setupCmdBuffer == VK_NULL_HANDLE)
			return;

		err = vkEndCommandBuffer(setupCmdBuffer);
		assert(!err);

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &setupCmdBuffer;

		err = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
		assert(!err);

		err = vkQueueWaitIdle(queue);
		assert(!err);
	}

	void createCommandBuffers()
	{
		drawCmdBuffers.resize(swapChain.imageCount);

		VkCommandBufferAllocateInfo cmdBufAllocateInfo =
			vkTools::initializers::commandBufferAllocateInfo(
				cmdPool,
				VK_COMMAND_BUFFER_LEVEL_PRIMARY,
				drawCmdBuffers.size());

		VkResult vkRes = vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, drawCmdBuffers.data());
		assert(!vkRes);

		cmdBufAllocateInfo.commandBufferCount = 1;

		vkRes = vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &postPresentCmdBuffer);
		assert(!vkRes);
	}

	void prepareVertices()
	{
		// Load mesh from compressed asset
		AAsset* asset = AAssetManager_open(app->activity->assetManager, "models/vulkanlogo.obj", AASSET_MODE_STREAMING);
		assert(asset);
		size_t size = AAsset_getLength(asset);
		assert(size > 0);

		char *assetData = new char[size];
		AAsset_read(asset, assetData, size);
		AAsset_close(asset);

		std::stringstream assetStream(assetData);

		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string objerr;
		tinyobj::MaterialFileReader matFileReader("");
		bool ret = tinyobj::LoadObj(shapes, materials, objerr, assetStream, matFileReader, true);

		LOGW("shapes %d", shapes.size());

		// Setup vertices
		float scale = 0.025f;
		std::vector<Vertex> vertexBuffer;
		std::vector<uint32_t> indexBuffer;
		for (auto& shape : shapes)
		{
			// Vertices
			for (size_t i = 0; i < shape.mesh.positions.size() / 3; i++)
			{
				Vertex v;
				v.pos[0] = shape.mesh.positions[3 * i + 0] * scale;
				v.pos[1] = -shape.mesh.positions[3 * i + 1] * scale;
				v.pos[2] = shape.mesh.positions[3 * i + 2] * scale;
				v.normal[0] = shape.mesh.normals[3 * i + 0];
				v.normal[1] = shape.mesh.normals[3 * i + 1];
				v.normal[2] = shape.mesh.normals[3 * i + 2];
				v.color = glm::vec3(1.0f, 0.0f, 0.0f);
				vertexBuffer.push_back(v);
			}
			// Indices
			for (size_t i = 0; i < shape.mesh.indices.size() / 3; i++)
			{
				indexBuffer.push_back(shape.mesh.indices[3 * i + 0]);
				indexBuffer.push_back(shape.mesh.indices[3 * i + 1]);
				indexBuffer.push_back(shape.mesh.indices[3 * i + 2]);
			}

		}

		uint32_t vertexBufferSize = vertexBuffer.size() * sizeof(Vertex);
		uint32_t indexBufferSize = indexBuffer.size() * sizeof(uint32_t);

		VkMemoryAllocateInfo memAlloc = {};
		memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memAlloc.pNext = NULL;
		memAlloc.allocationSize = 0;
		memAlloc.memoryTypeIndex = 0;
		VkMemoryRequirements memReqs;

		VkResult err;
		void *data;

		// Generate vertex buffer
		//	Setup
		VkBufferCreateInfo bufInfo = {};
		bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufInfo.pNext = NULL;
		bufInfo.size = vertexBufferSize;
		bufInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		bufInfo.flags = 0;
		//	Copy vertex data to VRAM
		memset(&vertices, 0, sizeof(vertices));
		err = vkCreateBuffer(device, &bufInfo, nullptr, &vertices.buf);
		assert(!err);
		vkGetBufferMemoryRequirements(device, vertices.buf, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memAlloc.memoryTypeIndex);
		vkAllocateMemory(device, &memAlloc, nullptr, &vertices.mem);
		assert(!err);
		err = vkMapMemory(device, vertices.mem, 0, memAlloc.allocationSize, 0, &data);
		assert(!err);
		memcpy(data, vertexBuffer.data(), vertexBufferSize);
		vkUnmapMemory(device, vertices.mem);
		assert(!err);
		err = vkBindBufferMemory(device, vertices.buf, vertices.mem, 0);
		assert(!err);

		// Generate index buffer
		//	Setup
		VkBufferCreateInfo indexbufferInfo = {};
		indexbufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		indexbufferInfo.pNext = NULL;
		indexbufferInfo.size = indexBufferSize;
		indexbufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		indexbufferInfo.flags = 0;
		// Copy index data to VRAM
		memset(&indices, 0, sizeof(indices));
		err = vkCreateBuffer(device, &bufInfo, nullptr, &indices.buf);
		assert(!err);
		vkGetBufferMemoryRequirements(device, indices.buf, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memAlloc.memoryTypeIndex);
		err = vkAllocateMemory(device, &memAlloc, nullptr, &indices.mem);
		assert(!err);
		err = vkMapMemory(device, indices.mem, 0, indexBufferSize, 0, &data);
		assert(!err);
		memcpy(data, indexBuffer.data(), indexBufferSize);
		vkUnmapMemory(device, indices.mem);
		err = vkBindBufferMemory(device, indices.buf, indices.mem, 0);
		assert(!err);
		indices.count = indexBuffer.size();

		// Binding description
		vertices.bindingDescriptions.resize(1);
		vertices.bindingDescriptions[0] =
			vkTools::initializers::vertexInputBindingDescription(
				VERTEX_BUFFER_BIND_ID,
				sizeof(Vertex),
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
		// Location 2 : Color
		vertices.attributeDescriptions[2] =
			vkTools::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				2,
				VK_FORMAT_R32G32B32_SFLOAT,
				sizeof(float) * 6);

		// Assign to vertex buffer
		vertices.inputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertices.inputState.pNext = NULL;
		vertices.inputState.vertexBindingDescriptionCount = vertices.bindingDescriptions.size();
		vertices.inputState.pVertexBindingDescriptions = vertices.bindingDescriptions.data();
		vertices.inputState.vertexAttributeDescriptionCount = vertices.attributeDescriptions.size();
		vertices.inputState.pVertexAttributeDescriptions = vertices.attributeDescriptions.data();
	}

	void updateUniformBuffers()
	{
		// Update matrices
		uboVS.projection = glm::perspective(glm::radians(60.0f), (float)width / (float)height, 0.1f, 256.0f);

		glm::mat4 viewMatrix = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, state.zoom));

		uboVS.model = viewMatrix;
		uboVS.model = glm::rotate(uboVS.model, glm::radians(state.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		uboVS.model = glm::rotate(uboVS.model, glm::radians(state.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		uboVS.model = glm::rotate(uboVS.model, glm::radians(state.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));


		// Map uniform buffer and update it
		uint8_t *pData;
		VkResult err = vkMapMemory(device, uniformDataVS.memory, 0, sizeof(uboVS), 0, (void **)&pData);
		assert(!err);
		memcpy(pData, &uboVS, sizeof(uboVS));
		vkUnmapMemory(device, uniformDataVS.memory);
		assert(!err);
	}

	void prepareUniformBuffers()
	{
		// Prepare and initialize uniform buffer containing shader uniforms
		VkMemoryRequirements memReqs;

		// Vertex shader uniform buffer block
		VkBufferCreateInfo bufferInfo = {};
		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.pNext = NULL;
		allocInfo.allocationSize = 0;
		allocInfo.memoryTypeIndex = 0;
		VkResult err;

		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = sizeof(uboVS);
		bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

		// Create a new buffer
		err = vkCreateBuffer(device, &bufferInfo, nullptr, &uniformDataVS.buffer);
		assert(!err);
		// Get memory requirements including size, alignment and memory type 
		vkGetBufferMemoryRequirements(device, uniformDataVS.buffer, &memReqs);
		allocInfo.allocationSize = memReqs.size;
		// Gets the appropriate memory type for this type of buffer allocation
		// Only memory types that are visible to the host
		getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &allocInfo.memoryTypeIndex);
		// Allocate memory for the uniform buffer
		err = vkAllocateMemory(device, &allocInfo, nullptr, &(uniformDataVS.memory));
		assert(!err);
		// Bind memory to buffer
		err = vkBindBufferMemory(device, uniformDataVS.buffer, uniformDataVS.memory, 0);
		assert(!err);

		// Store information in the uniform's descriptor
		uniformDataVS.descriptor.buffer = uniformDataVS.buffer;
		uniformDataVS.descriptor.offset = 0;
		uniformDataVS.descriptor.range = sizeof(uboVS);

		updateUniformBuffers();
	}

	void preparePipelines()
	{
		VkResult err;

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
			vkTools::initializers::pipelineInputAssemblyStateCreateInfo(
				VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
				0,
				VK_FALSE);

		VkPipelineRasterizationStateCreateInfo rasterizationState =
			vkTools::initializers::pipelineRasterizationStateCreateInfo(
				VK_POLYGON_MODE_FILL,
				VK_CULL_MODE_NONE,
				VK_FRONT_FACE_COUNTER_CLOCKWISE,
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

		std::vector<VkDynamicState> dynamicStateEnables;
		dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);
		dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);

		VkPipelineDynamicStateCreateInfo dynamicState =
			vkTools::initializers::pipelineDynamicStateCreateInfo(
				dynamicStateEnables.data(),
				dynamicStateEnables.size(),
				0);

		// Rendering pipeline
		// Load shaders
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

		shaderStages[0] = loadShader("shaders/mesh.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader("shaders/mesh.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

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
		pipelineCreateInfo.renderPass = renderPass;

		err = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.solid);
		assert(!err);
	}

	void setupDescriptorPool()
	{
		std::vector<VkDescriptorPoolSize> poolSizes;
		poolSizes.push_back(vkTools::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1));

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
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings;
		setLayoutBindings.push_back(
			// Binding 0 : Vertex shader uniform buffer
			vkTools::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				VK_SHADER_STAGE_VERTEX_BIT,
				0));

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

		std::vector<VkWriteDescriptorSet> writeDescriptorSets;
		writeDescriptorSets.push_back(
			// Binding 0 : Vertex shader uniform buffer
			vkTools::initializers::writeDescriptorSet(
				descriptorSet,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				0,
				&uniformDataVS.descriptor));

		vkUpdateDescriptorSets(device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);
	}

	void setupDepthStencil()
	{
		VkImageCreateInfo image = {};
		image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		image.pNext = NULL;
		image.imageType = VK_IMAGE_TYPE_2D;
		image.format = VK_FORMAT_D24_UNORM_S8_UINT;
		image.extent = { width, height, 1 };
		image.mipLevels = 1;
		image.arrayLayers = 1;
		image.samples = VK_SAMPLE_COUNT_1_BIT;
		image.tiling = VK_IMAGE_TILING_OPTIMAL;
		image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		image.flags = 0;

		VkMemoryAllocateInfo mem_alloc = {};
		mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		mem_alloc.pNext = NULL;
		mem_alloc.allocationSize = 0;
		mem_alloc.memoryTypeIndex = 0;

		VkImageViewCreateInfo depthStencilView = {};
		depthStencilView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		depthStencilView.pNext = NULL;
		depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
		depthStencilView.format = VK_FORMAT_D24_UNORM_S8_UINT;
		depthStencilView.flags = 0;
		depthStencilView.subresourceRange = {};
		depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		depthStencilView.subresourceRange.baseMipLevel = 0;
		depthStencilView.subresourceRange.levelCount = 1;
		depthStencilView.subresourceRange.baseArrayLayer = 0;
		depthStencilView.subresourceRange.layerCount = 1;

		VkMemoryRequirements memReqs;
		VkResult err;

		err = vkCreateImage(device, &image, nullptr, &depthStencil.image);
		assert(!err);
		vkGetImageMemoryRequirements(device, depthStencil.image, &memReqs);
		mem_alloc.allocationSize = memReqs.size;
		getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &mem_alloc.memoryTypeIndex);
		err = vkAllocateMemory(device, &mem_alloc, nullptr, &depthStencil.mem);
		assert(!err);

		err = vkBindImageMemory(device, depthStencil.image, depthStencil.mem, 0);
		assert(!err);
		vkTools::setImageLayout(setupCmdBuffer, depthStencil.image, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

		depthStencilView.image = depthStencil.image;
		err = vkCreateImageView(device, &depthStencilView, nullptr, &depthStencil.view);
		assert(!err);
	}

	void setupFrameBuffer()
	{
		VkImageView attachments[2];

		// Depth/Stencil attachment is the same for all frame buffers
		attachments[1] = depthStencil.view;

		VkFramebufferCreateInfo frameBufferCreateInfo = {};
		frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frameBufferCreateInfo.pNext = NULL;
		frameBufferCreateInfo.renderPass = renderPass;
		frameBufferCreateInfo.attachmentCount = 2;
		frameBufferCreateInfo.pAttachments = attachments;
		frameBufferCreateInfo.width = width;
		frameBufferCreateInfo.height = height;
		frameBufferCreateInfo.layers = 1;

		// Create frame buffers for every swap chain image
		frameBuffers.resize(swapChain.imageCount);
		for (uint32_t i = 0; i < frameBuffers.size(); i++)
		{
			attachments[0] = swapChain.buffers[i].view;
			VkResult err = vkCreateFramebuffer(device, &frameBufferCreateInfo, nullptr, &frameBuffers[i]);
			assert(!err);
		}
	}

	void setupRenderPass()
	{
		VkAttachmentDescription attachments[2];
		attachments[0].format = VK_FORMAT_R8G8B8A8_UNORM;
		attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		attachments[1].format = VK_FORMAT_D24_UNORM_S8_UINT;
		attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorReference = {};
		colorReference.attachment = 0;
		colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthReference = {};
		depthReference.attachment = 1;
		depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.flags = 0;
		subpass.inputAttachmentCount = 0;
		subpass.pInputAttachments = NULL;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorReference;
		subpass.pResolveAttachments = NULL;
		subpass.pDepthStencilAttachment = &depthReference;
		subpass.preserveAttachmentCount = 0;
		subpass.pPreserveAttachments = NULL;

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.pNext = NULL;
		renderPassInfo.attachmentCount = 2;
		renderPassInfo.pAttachments = attachments;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 0;
		renderPassInfo.pDependencies = NULL;

		VkResult err;

		err = vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass);
		assert(!err);
	}

	void buildCommandBuffers()
	{
		VkCommandBufferBeginInfo cmdBufInfo = {};
		cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBufInfo.pNext = NULL;

		VkClearValue clearValues[2];
		clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderPassBeginInfo = {};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.pNext = NULL;
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

			vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			// Update dynamic viewport state
			VkViewport viewport = {};
			viewport.height = (float)height;
			viewport.width = (float)width;
			viewport.minDepth = (float) 0.0f;
			viewport.maxDepth = (float) 1.0f;
			vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

			// Update dynamic scissor state
			VkRect2D scissor = {};
			scissor.extent.width = width;
			scissor.extent.height = height;
			scissor.offset.x = 0;
			scissor.offset.y = 0;
			vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

			// Bind descriptor sets describing shader binding points
			vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);

			// Bind the rendering pipeline (including the shaders)
			vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.solid);

			// Bind triangle vertices
			VkDeviceSize offsets[1] = { 0 };
			vkCmdBindVertexBuffers(drawCmdBuffers[i], VERTEX_BUFFER_BIND_ID, 1, &vertices.buf, offsets);

			// Bind triangle indices
			vkCmdBindIndexBuffer(drawCmdBuffers[i], indices.buf, 0, VK_INDEX_TYPE_UINT32);

			// Draw indexed triangle
			vkCmdDrawIndexed(drawCmdBuffers[i], indices.count, 1, 0, 0, 1);

			vkCmdEndRenderPass(drawCmdBuffers[i]);

			// Add a present memory barrier to the end of the command buffer
			// This will transform the frame buffer color attachment to a
			// new layout for presenting it to the windowing system integration 
			VkImageMemoryBarrier prePresentBarrier = {};
			prePresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			prePresentBarrier.pNext = NULL;
			prePresentBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			prePresentBarrier.dstAccessMask = 0;
			prePresentBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			prePresentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			prePresentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			prePresentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			prePresentBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
			prePresentBarrier.image = swapChain.buffers[i].image;

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
		VkSemaphoreCreateInfo presentCompleteSemaphoreCreateInfo = {};
		presentCompleteSemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		presentCompleteSemaphoreCreateInfo.pNext = NULL;

		err = vkCreateSemaphore(device, &presentCompleteSemaphoreCreateInfo, nullptr, &presentCompleteSemaphore);
		assert(!err);

		// Get next image in the swap chain (back/front buffer)
		err = swapChain.acquireNextImage(presentCompleteSemaphore, &currentBuffer);
		assert(!err);

		// The submit infor strcuture contains a list of
		// command buffers and semaphores to be submitted to a queue
		// If you want to submit multiple command buffers, pass an array
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &presentCompleteSemaphore;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];

		// Submit to the graphics queue
		err = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
		assert(!err);

		// Present the current buffer to the swap chain
		// This will display the image
		err = swapChain.queuePresent(queue, currentBuffer);
		assert(!err);

		vkDestroySemaphore(device, presentCompleteSemaphore, nullptr);

		// Add a post present image memory barrier
		// This will transform the frame buffer color attachment back
		// to it's initial layout after it has been presented to the
		// windowing system
		// See buildCommandBuffers for the pre present barrier that 
		// does the opposite transformation 
		VkImageMemoryBarrier postPresentBarrier = {};
		postPresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		postPresentBarrier.pNext = NULL;
		postPresentBarrier.srcAccessMask = 0;
		postPresentBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		postPresentBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		postPresentBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		postPresentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		postPresentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		postPresentBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		postPresentBarrier.image = swapChain.buffers[currentBuffer].image;

		// Use dedicated command buffer from example base class for submitting the post present barrier
		VkCommandBufferBeginInfo cmdBufInfo = {};
		cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		err = vkBeginCommandBuffer(postPresentCmdBuffer, &cmdBufInfo);
		assert(!err);

		// Put post present barrier into command buffer
		vkCmdPipelineBarrier(
			postPresentCmdBuffer,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_FLAGS_NONE,
			0, nullptr,
			0, nullptr,
			1, &postPresentBarrier);

		err = vkEndCommandBuffer(postPresentCmdBuffer);
		assert(!err);

		// Submit to the queue
		submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &postPresentCmdBuffer;

		err = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
		assert(!err);

		err = vkQueueWaitIdle(queue);
		assert(!err);
	}


};

static int32_t handleInput(struct android_app* app, AInputEvent* event)
{
	struct VulkanExample* vulkanExample = (struct VulkanExample*)app->userData;
	if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION)
	{
		// todo
		return 1;
	}
	return 0;
}

static void handleCommand(struct android_app* app, int32_t cmd)
{
	VulkanExample* vulkanExample = (VulkanExample*)app->userData;
	switch (cmd)
	{
	case APP_CMD_SAVE_STATE:
		vulkanExample->app->savedState = malloc(sizeof(struct saved_state));
		*((struct saved_state*)vulkanExample->app->savedState) = vulkanExample->state;
		vulkanExample->app->savedStateSize = sizeof(struct saved_state);
		break;
	case APP_CMD_INIT_WINDOW:
		if (vulkanExample->app->window != NULL)
		{
			vulkanExample->initVulkan();
			assert(vulkanExample->prepared);
		}
		break;
	case APP_CMD_LOST_FOCUS:
		vulkanExample->animating = 0;
		break;
	}
}

/**
* This is the main entry point of a native application that is using
* android_native_app_glue.  It runs in its own thread, with its own
* event loop for receiving input events and doing other things.
*/
void android_main(struct android_app* state)
{
	VulkanExample *engine = new VulkanExample();

	//memset(&engine, 0, sizeof(engine));
	state->userData = engine;
	state->onAppCmd = handleCommand;
	state->onInputEvent = handleInput;
	engine->app = state;

	engine->animating = 1;

	// loop waiting for stuff to do.

	while (1)
	{
		// Read all pending events.
		int ident;
		int events;
		struct android_poll_source* source;

		while ((ident = ALooper_pollAll(engine->animating ? 0 : -1, NULL, &events, (void**)&source)) >= 0)
		{
			if (source != NULL)
			{
				source->process(state, source);
			}

			if (state->destroyRequested != 0)
			{
				engine->cleanupVulkan();
				return;
			}
		}

		// Render frame
		if (engine->prepared)
		{
			if (engine->animating)
			{ 
				// Update rotation
				engine->state.rotation.y += 0.25f;
				if (engine->state.rotation.y > 360.0f)
				{
					engine->state.rotation.y -= 360.0f;

				}
				engine->updateUniformBuffers();
			}
			engine->draw();
		}
	}
}
