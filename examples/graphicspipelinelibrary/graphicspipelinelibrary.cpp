/*
* Vulkan Example - Using VK_EXT_graphics_pipeline_library
* 
* Copyright (C) 2022 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "vulkanexamplebase.h"
#include "VulkanglTFModel.h"
#include <thread>
#include <mutex>

#define ENABLE_VALIDATION false

class VulkanExample: public VulkanExampleBase
{
public:
	vkglTF::Model scene;

	struct UBOVS {
		glm::mat4 projection;
		glm::mat4 modelView;
		glm::vec4 lightPos = glm::vec4(0.0f, 2.0f, 1.0f, 0.0f);
	} uboVS;
	vks::Buffer uniformBuffer;

	VkPipelineLayout pipelineLayout;
	VkDescriptorSet descriptorSet;
	VkDescriptorSetLayout descriptorSetLayout;

	VkPhysicalDeviceGraphicsPipelineLibraryFeaturesEXT graphicsPipelineLibraryFeatures{};

	struct PipelineLibrary {
		VkPipeline vertexInputInterface;
		VkPipeline preRasterizationShaders;
		VkPipeline fragmentOutputInterface;
	} pipelineLibrary;

	std::vector<VkPipeline> pipelines{};

	struct ShaderInfo {
		uint32_t* code;
		size_t size;
	};

	std::mutex mutex;
	VkPipelineCache threadPipelineCache{ VK_NULL_HANDLE };

	bool  newPipelineCreated = false;

	uint32_t splitX{ 2 };
	uint32_t splitY{ 2 };

	std::vector<glm::vec3> colors{};
	float rotation{ 0.0f };

	VulkanExample() : VulkanExampleBase(ENABLE_VALIDATION)
	{
		title = "Graphics pipeline library";
		camera.type = Camera::CameraType::lookat;
		camera.setPosition(glm::vec3(0.0f, 0.0f, -2.0f));
		camera.setRotation(glm::vec3(-25.0f, 15.0f, 0.0f));
		camera.setRotationSpeed(0.5f);

		// Enable required extensions
		enabledInstanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
		enabledDeviceExtensions.push_back(VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME);
		enabledDeviceExtensions.push_back(VK_EXT_GRAPHICS_PIPELINE_LIBRARY_EXTENSION_NAME);

		// Enable required extension features
		graphicsPipelineLibraryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GRAPHICS_PIPELINE_LIBRARY_FEATURES_EXT;
		graphicsPipelineLibraryFeatures.graphicsPipelineLibrary = VK_TRUE;
		deviceCreatepNextChain = &graphicsPipelineLibraryFeatures;
	}

	~VulkanExample()
	{
		if (device) {
			for (auto pipeline : pipelines) {
				vkDestroyPipeline(device, pipeline, nullptr);
			}
			vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
			vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
			uniformBuffer.destroy();
		}
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

		for (int32_t i = 0; i < drawCmdBuffers.size(); ++i)
		{
			renderPassBeginInfo.framebuffer = frameBuffers[i];

			VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo));

			vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);
			scene.bindBuffers(drawCmdBuffers[i]);

			// Render a viewport for each pipeline
			float w = (float)width / (float)splitX;
			float h = (float)height / (float)splitY;
			uint32_t idx = 0;
			for (uint32_t y = 0; y < splitX; y++) {
				for (uint32_t x = 0; x < splitY; x++) {
					VkViewport viewport{};
					viewport.x = w * (float)x;
					viewport.y = h * (float)y;
					viewport.width = w;
					viewport.height = h;
					viewport.minDepth = 0.0f;
					viewport.maxDepth = 1.0f;
					vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

					VkRect2D scissor{};
					scissor.extent.width = (uint32_t)w;
					scissor.extent.height = (uint32_t)h;
					scissor.offset.x = (uint32_t)w * x;
					scissor.offset.y = (uint32_t)h * y;
					vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

					if (pipelines.size() > idx) {
						vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[idx]);
						scene.draw(drawCmdBuffers[i]);
					}

					idx++;
				}
			}

			drawUI(drawCmdBuffers[i]);

			vkCmdEndRenderPass(drawCmdBuffers[i]);

			VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
		}
	}

	void loadAssets()
	{
		const uint32_t glTFLoadingFlags = vkglTF::FileLoadingFlags::PreTransformVertices | vkglTF::FileLoadingFlags::PreMultiplyVertexColors | vkglTF::FileLoadingFlags::FlipY;
		scene.loadFromFile(getAssetPath() + "models/color_teapot_spheres.gltf", vulkanDevice, queue, glTFLoadingFlags);
	}

	void setupDescriptorPool()
	{
		std::vector<VkDescriptorPoolSize> poolSizes = {
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1)
		};
		VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, 2);
		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));
	}

	void setupDescriptorSetLayout()
	{
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0)
		};
		VkDescriptorSetLayoutCreateInfo descriptorLayout = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayout));

		VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(&descriptorSetLayout, 1);
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &pipelineLayout));
	}

	void setupDescriptorSet()
	{
		VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));

		std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
			vks::initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffer.descriptor)
		};
		vkUpdateDescriptorSets(device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
	}

	// With VK_EXT_graphics_pipeline_library we don't need to create the shader module when loading it, but instead have the driver create it at linking time
	// So we use a custom function that only loads the required shader information without actually creating the shader module
	bool loadShaderFile(std::string fileName, ShaderInfo &shaderInfo)
	{
#if defined(__ANDROID__)
		// Load shader from compressed asset
		// @todo
		AAsset* asset = AAssetManager_open(androidApp->activity->assetManager, fileName, AASSET_MODE_STREAMING);
		assert(asset);
		size_t size = AAsset_getLength(asset);
		assert(size > 0);

		shaderInfo.size = size;
		shaderInfo.code = new uint32_t[size / 4];
		AAsset_read(asset, shaderCode, size);
		AAsset_close(asset);
#else
		std::ifstream is(fileName, std::ios::binary | std::ios::in | std::ios::ate);

		if (is.is_open())
		{
			shaderInfo.size = is.tellg();
			is.seekg(0, std::ios::beg);
			shaderInfo.code = new uint32_t[shaderInfo.size];
			is.read(reinterpret_cast<char*>(shaderInfo.code), shaderInfo.size);
			is.close();
			return true;
		} else {
			std::cerr << "Error: Could not open shader file \"" << fileName << "\"" << "\n";
			throw std::runtime_error("Could open shader file");
			return false;
		}
#endif
	}

	// Create the shared pipeline parts up-front
	void preparePipelineLibrary()
	{
		// Create a pipeline library for the vertex input interface
		{
			VkGraphicsPipelineLibraryCreateInfoEXT libraryInfo{};
			libraryInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_LIBRARY_CREATE_INFO_EXT;
			libraryInfo.flags = VK_GRAPHICS_PIPELINE_LIBRARY_VERTEX_INPUT_INTERFACE_BIT_EXT;

			VkPipelineVertexInputStateCreateInfo vertexInputState = *vkglTF::Vertex::getPipelineVertexInputState({ vkglTF::VertexComponent::Position, vkglTF::VertexComponent::Normal, vkglTF::VertexComponent::Color });
			VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

			VkGraphicsPipelineCreateInfo pipelineCI{};
			pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipelineCI.flags = VK_PIPELINE_CREATE_LIBRARY_BIT_KHR | VK_PIPELINE_CREATE_RETAIN_LINK_TIME_OPTIMIZATION_INFO_BIT_EXT;
			pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipelineCI.pNext = &libraryInfo;
			pipelineCI.pInputAssemblyState = &inputAssemblyState;
			pipelineCI.pVertexInputState = &vertexInputState;
			VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelineLibrary.vertexInputInterface));
		}

		// Creata a pipeline library for the vertex shader stage
		{
			VkGraphicsPipelineLibraryCreateInfoEXT libraryInfo{};
			libraryInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_LIBRARY_CREATE_INFO_EXT;
			libraryInfo.flags = VK_GRAPHICS_PIPELINE_LIBRARY_PRE_RASTERIZATION_SHADERS_BIT_EXT;

			VkDynamicState vertexDynamicStates[2] = {
				VK_DYNAMIC_STATE_VIEWPORT,
				VK_DYNAMIC_STATE_SCISSOR };

			VkPipelineDynamicStateCreateInfo dynamicInfo{};
			dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamicInfo.dynamicStateCount = 2;
			dynamicInfo.pDynamicStates = vertexDynamicStates;

			VkPipelineViewportStateCreateInfo viewportState = {};
			viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportState.viewportCount = 1;
			viewportState.scissorCount = 1;

			VkPipelineRasterizationStateCreateInfo rasterizationState = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);

			// @todo: we can skip the pipeline shader module info and directly consume the shader module
			ShaderInfo shaderInfo{};
			loadShaderFile(getShadersPath() + "graphicspipelinelibrary/shared.vert.spv", shaderInfo);

			VkShaderModuleCreateInfo shaderModuleCI{};
			shaderModuleCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			shaderModuleCI.codeSize = shaderInfo.size;
			shaderModuleCI.pCode = shaderInfo.code;

			VkPipelineShaderStageCreateInfo shaderStageCI{};
			shaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStageCI.pNext = &shaderModuleCI;
			shaderStageCI.stage = VK_SHADER_STAGE_VERTEX_BIT;
			shaderStageCI.pName = "main";

			VkGraphicsPipelineCreateInfo pipelineCI{};
			pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipelineCI.pNext = &libraryInfo;
			pipelineCI.renderPass = renderPass;
			pipelineCI.flags = VK_PIPELINE_CREATE_LIBRARY_BIT_KHR | VK_PIPELINE_CREATE_RETAIN_LINK_TIME_OPTIMIZATION_INFO_BIT_EXT;
			pipelineCI.stageCount = 1;
			pipelineCI.pStages = &shaderStageCI;
			pipelineCI.layout = pipelineLayout;
			pipelineCI.pDynamicState = &dynamicInfo;
			pipelineCI.pViewportState = &viewportState;
			pipelineCI.pRasterizationState = &rasterizationState;
			VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelineLibrary.preRasterizationShaders));
		}

		// Create a pipeline library for the fragment output interface
		{
			VkGraphicsPipelineLibraryCreateInfoEXT libraryInfo{};
			libraryInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_LIBRARY_CREATE_INFO_EXT;
			libraryInfo.flags = VK_GRAPHICS_PIPELINE_LIBRARY_FRAGMENT_OUTPUT_INTERFACE_BIT_EXT;

			VkPipelineColorBlendAttachmentState  blendAttachmentSstate = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
			VkPipelineColorBlendStateCreateInfo  colorBlendState = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentSstate);
			VkPipelineMultisampleStateCreateInfo multisampleState = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);

			VkGraphicsPipelineCreateInfo pipelineLibraryCI{};
			pipelineLibraryCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipelineLibraryCI.pNext = &libraryInfo;
			pipelineLibraryCI.layout = pipelineLayout;
			pipelineLibraryCI.renderPass = renderPass;
			pipelineLibraryCI.pColorBlendState = &colorBlendState;
			pipelineLibraryCI.pMultisampleState = &multisampleState;
			VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineLibraryCI, nullptr, &pipelineLibrary.fragmentOutputInterface));
		}
	}

	void threadFn()
	{
		const std::lock_guard<std::mutex> lock(mutex);

		auto start = std::chrono::steady_clock::now();

		prepareNewPipeline();
		newPipelineCreated = true;

		// Change viewport/draw count
		if (pipelines.size() > splitX * splitY) {
			splitX++;
			splitY++;
		}

		auto delta = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start);
		std::cout << "Pipeline created in " << delta.count() << " microseconds\n";
	}

	// Create a new pipeline using the pipeline library and a customized fragment shader
	// Used from a thread
	void prepareNewPipeline()
	{
		// Create the fragment shader part of the pipeline library with some random options
		VkGraphicsPipelineLibraryCreateInfoEXT libraryInfo{};
		libraryInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_LIBRARY_CREATE_INFO_EXT;
		libraryInfo.flags = VK_GRAPHICS_PIPELINE_LIBRARY_FRAGMENT_SHADER_BIT_EXT;

		VkPipelineDepthStencilStateCreateInfo depthStencilState = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
		VkPipelineMultisampleStateCreateInfo  multisampleState = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);

		// Using the pipeline library extension, we can skip the pipeline shader module creation and directly pass the shader code to the pipeline
		ShaderInfo shaderInfo{};
		loadShaderFile(getShadersPath() + "graphicspipelinelibrary/uber.frag.spv", shaderInfo);

		VkShaderModuleCreateInfo shaderModuleCI{};
		shaderModuleCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shaderModuleCI.codeSize = shaderInfo.size;
		shaderModuleCI.pCode = shaderInfo.code;

		VkPipelineShaderStageCreateInfo shaderStageCI{};
		shaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageCI.pNext = &shaderModuleCI;
		shaderStageCI.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		shaderStageCI.pName = "main";

		// Select lighting model using a specialization constant
		srand((unsigned int)time(NULL));
		uint32_t lighting_model = (int)(rand() % 4);

		// Each shader constant of a shader stage corresponds to one map entry
		VkSpecializationMapEntry specializationMapEntry{};
		specializationMapEntry.constantID = 0;
		specializationMapEntry.size = sizeof(uint32_t);

		VkSpecializationInfo specializationInfo{};
		specializationInfo.mapEntryCount = 1;
		specializationInfo.pMapEntries = &specializationMapEntry;
		specializationInfo.dataSize = sizeof(uint32_t);
		specializationInfo.pData = &lighting_model;

		shaderStageCI.pSpecializationInfo = &specializationInfo;

		VkGraphicsPipelineCreateInfo pipelineCI{};
		pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineCI.pNext = &libraryInfo;
		pipelineCI.flags = VK_PIPELINE_CREATE_LIBRARY_BIT_KHR | VK_PIPELINE_CREATE_RETAIN_LINK_TIME_OPTIMIZATION_INFO_BIT_EXT;
		pipelineCI.stageCount = 1;
		pipelineCI.pStages = &shaderStageCI;
		pipelineCI.layout = pipelineLayout;
		pipelineCI.renderPass = renderPass;
		pipelineCI.pDepthStencilState = &depthStencilState;
		pipelineCI.pMultisampleState = &multisampleState;
		VkPipeline fragment_shader = VK_NULL_HANDLE;
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, threadPipelineCache, 1, &pipelineCI, nullptr, &fragment_shader));

		// Create the pipeline using the pre-built pipeline library parts
		// Except for above fragment shader part all parts have been pre-built and will be re-used
		std::vector<VkPipeline> libraries = {
			pipelineLibrary.vertexInputInterface,
			pipelineLibrary.preRasterizationShaders,
			fragment_shader,
			pipelineLibrary.fragmentOutputInterface };

		// Link the library parts into a graphics pipeline
		VkPipelineLibraryCreateInfoKHR pipelineLibraryCI{};
		pipelineLibraryCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LIBRARY_CREATE_INFO_KHR;
		pipelineLibraryCI.libraryCount = static_cast<uint32_t>(libraries.size());
		pipelineLibraryCI.pLibraries = libraries.data();

		// If set to true, we pass VK_PIPELINE_CREATE_LINK_TIME_OPTIMIZATION_BIT_EXT which will let the implementation do additional optimizations at link time
		// This trades in pipeline creation time for run-time performance
		bool optimized = true;

		VkGraphicsPipelineCreateInfo executablePipelineCI{};
		executablePipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		executablePipelineCI.pNext = &pipelineLibraryCI;
		executablePipelineCI.flags |= optimized ? VK_PIPELINE_CREATE_LINK_TIME_OPTIMIZATION_BIT_EXT : 0;

		VkPipeline executable = VK_NULL_HANDLE;
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, threadPipelineCache, 1, &executablePipelineCI, nullptr, &executable));

		pipelines.push_back(executable);
	}

	// Prepare and initialize uniform buffer containing shader uniforms
	void prepareUniformBuffers()
	{
		// Create the vertex shader uniform buffer block
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&uniformBuffer,
			sizeof(uboVS)));

		// Map persistent
		VK_CHECK_RESULT(uniformBuffer.map());

		updateUniformBuffers();
	}

	void updateUniformBuffers()
	{
		if (!paused) {
			rotation += frameTimer * 0.1f;
		}
		camera.setPerspective(45.0f, ((float)width / (float)splitX) / ((float)height / (float)splitY), 0.1f, 256.0f);
		uboVS.projection = camera.matrices.perspective;
		uboVS.modelView = camera.matrices.view * glm::rotate(glm::mat4(1.0f), glm::radians(rotation * 360.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		memcpy(uniformBuffer.mapped, &uboVS, sizeof(uboVS));
	}

	void draw()
	{
		VulkanExampleBase::prepareFrame();

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

		VulkanExampleBase::submitFrame();
	}

	void prepare()
	{
		VulkanExampleBase::prepare();
		loadAssets();
		prepareUniformBuffers();
		setupDescriptorSetLayout();
		preparePipelineLibrary();
		setupDescriptorPool();
		setupDescriptorSet();
		buildCommandBuffers();

		// Create a separate pipeline cache for the pipeline creation thread
		VkPipelineCacheCreateInfo pipelineCachCI = {};
		pipelineCachCI.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		vkCreatePipelineCache(device, &pipelineCachCI, nullptr, &threadPipelineCache);

		// Create first pipeline using a background thread
		std::thread pipelineGenerationThread(&VulkanExample::threadFn, this);
		pipelineGenerationThread.detach();

		prepared = true;
	}

	virtual void render()
	{
		if (!prepared)
			return;
		if (newPipelineCreated)
		{
			newPipelineCreated = false;
			vkQueueWaitIdle(queue);
			buildCommandBuffers();
		}
		draw();
		updateUniformBuffers();
	}

	virtual void OnUpdateUIOverlay(vks::UIOverlay *overlay)
	{
		if (overlay->button("New pipeline")) {
			// Spwan a thread to create a new pipeline in the background
			std::thread pipelineGenerationThread(&VulkanExample::threadFn, this);
			pipelineGenerationThread.detach();
		}
	}
};

VULKAN_EXAMPLE_MAIN()