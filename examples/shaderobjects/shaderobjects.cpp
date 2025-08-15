/*
 * Vulkan Example - Using shader objects via VK_EXT_shader_object
 *
 * Copyright (C) 2023 by Sascha Willems - www.saschawillems.de
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */

#include "vulkanexamplebase.h"
#include "VulkanglTFModel.h"

class VulkanExample: public VulkanExampleBase
{
public:
	vkglTF::Model scene;

	// Same uniform buffer layout as shader
	struct UniformData {
		glm::mat4 projection;
		glm::mat4 modelView;
		glm::vec4 lightPos = glm::vec4(0.0f, 2.0f, 1.0f, 0.0f);
	} uniformData;
	std::array<vks::Buffer, maxConcurrentFrames> uniformBuffers;

	VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };
	VkDescriptorSetLayout descriptorSetLayout{ VK_NULL_HANDLE };
	std::array<VkDescriptorSet, maxConcurrentFrames> descriptorSets{};

	VkShaderEXT shaders[2]{};

	VkPhysicalDeviceShaderObjectFeaturesEXT enabledShaderObjectFeaturesEXT{};
	VkPhysicalDeviceDynamicRenderingFeaturesKHR enabledDynamicRenderingFeaturesKHR{};

	PFN_vkCreateShadersEXT vkCreateShadersEXT{ VK_NULL_HANDLE };
	PFN_vkDestroyShaderEXT vkDestroyShaderEXT{ VK_NULL_HANDLE };
	PFN_vkCmdBindShadersEXT vkCmdBindShadersEXT{ VK_NULL_HANDLE };
	PFN_vkGetShaderBinaryDataEXT vkGetShaderBinaryDataEXT{ VK_NULL_HANDLE };

	// VK_EXT_shader_objects requires render passes to be dynamic
	PFN_vkCmdBeginRenderingKHR vkCmdBeginRenderingKHR{ VK_NULL_HANDLE };
	PFN_vkCmdEndRenderingKHR vkCmdEndRenderingKHR{ VK_NULL_HANDLE };

	// With VK_EXT_shader_object pipeline state must be set at command buffer creation using these functions
	PFN_vkCmdSetAlphaToCoverageEnableEXT vkCmdSetAlphaToCoverageEnableEXT{ VK_NULL_HANDLE };
	PFN_vkCmdSetColorBlendEnableEXT vkCmdSetColorBlendEnableEXT{ VK_NULL_HANDLE };
	PFN_vkCmdSetColorWriteMaskEXT vkCmdSetColorWriteMaskEXT{ VK_NULL_HANDLE };
	PFN_vkCmdSetCullModeEXT vkCmdSetCullModeEXT{ VK_NULL_HANDLE };
	PFN_vkCmdSetDepthBiasEnableEXT vkCmdSetDepthBiasEnableEXT{ VK_NULL_HANDLE };
	PFN_vkCmdSetDepthCompareOpEXT vkCmdSetDepthCompareOpEXT{ VK_NULL_HANDLE };
	PFN_vkCmdSetDepthTestEnableEXT vkCmdSetDepthTestEnableEXT{ VK_NULL_HANDLE };
	PFN_vkCmdSetDepthWriteEnableEXT vkCmdSetDepthWriteEnableEXT{ VK_NULL_HANDLE };
	PFN_vkCmdSetFrontFaceEXT vkCmdSetFrontFaceEXT{ VK_NULL_HANDLE };
	PFN_vkCmdSetPolygonModeEXT vkCmdSetPolygonModeEXT{ VK_NULL_HANDLE };
	PFN_vkCmdSetPrimitiveRestartEnableEXT vkCmdSetPrimitiveRestartEnableEXT{ VK_NULL_HANDLE };
	PFN_vkCmdSetPrimitiveTopologyEXT vkCmdSetPrimitiveTopologyEXT{ VK_NULL_HANDLE };
	PFN_vkCmdSetRasterizationSamplesEXT vkCmdSetRasterizationSamplesEXT{ VK_NULL_HANDLE };
	PFN_vkCmdSetRasterizerDiscardEnableEXT vkCmdSetRasterizerDiscardEnableEXT{ VK_NULL_HANDLE };
	PFN_vkCmdSetSampleMaskEXT vkCmdSetSampleMaskEXT{ VK_NULL_HANDLE };
	PFN_vkCmdSetScissorWithCountEXT vkCmdSetScissorWithCountEXT{ VK_NULL_HANDLE };
	PFN_vkCmdSetStencilTestEnableEXT vkCmdSetStencilTestEnableEXT{ VK_NULL_HANDLE };
	PFN_vkCmdSetViewportWithCountEXT vkCmdSetViewportWithCountEXT{ VK_NULL_HANDLE };

	// VK_EXT_vertex_input_dynamic_state
	PFN_vkCmdSetVertexInputEXT vkCmdSetVertexInputEXT{ VK_NULL_HANDLE };

	VulkanExample() : VulkanExampleBase()
	{
		title = "Shader objects (VK_EXT_shader_object)";
		camera.type = Camera::CameraType::lookat;
		camera.setPosition(glm::vec3(0.0f, 0.0f, -10.5f));
		camera.setRotation(glm::vec3(-25.0f, 15.0f, 0.0f));
		camera.setRotationSpeed(0.5f);
		camera.setPerspective(60.0f, (float)(width) / (float)height, 0.1f, 256.0f);

		enabledDeviceExtensions.push_back(VK_EXT_SHADER_OBJECT_EXTENSION_NAME);

		enabledDeviceExtensions.push_back(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);

		// With VK_EXT_shader_object all baked pipeline state is set dynamically at command buffer creation, so we need to enable additional extensions
		enabledDeviceExtensions.push_back(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME);
		enabledDeviceExtensions.push_back(VK_EXT_VERTEX_INPUT_DYNAMIC_STATE_EXTENSION_NAME);

		// Since we are not requiring Vulkan 1.2, we need to enable some additional extensios for dynamic rendering
		enabledDeviceExtensions.push_back(VK_KHR_MAINTENANCE2_EXTENSION_NAME);
		enabledDeviceExtensions.push_back(VK_KHR_MULTIVIEW_EXTENSION_NAME);
		enabledDeviceExtensions.push_back(VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME);
		enabledDeviceExtensions.push_back(VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME);

		enabledInstanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

		enabledShaderObjectFeaturesEXT.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_OBJECT_FEATURES_EXT;
		enabledShaderObjectFeaturesEXT.shaderObject = VK_TRUE;

		enabledDynamicRenderingFeaturesKHR.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
		enabledDynamicRenderingFeaturesKHR.dynamicRendering = VK_TRUE;
		enabledDynamicRenderingFeaturesKHR.pNext = &enabledShaderObjectFeaturesEXT;

		deviceCreatepNextChain = &enabledDynamicRenderingFeaturesKHR;
	}

	~VulkanExample()
	{
		if (device) {
			vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
			vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
			for (auto& buffer : uniformBuffers) {
				buffer.destroy();
			}
			vkDestroyShaderEXT(device, shaders[0], nullptr);
			vkDestroyShaderEXT(device, shaders[1], nullptr);
		}
	}

	void loadAssets()
	{
		const uint32_t glTFLoadingFlags = vkglTF::FileLoadingFlags::PreTransformVertices | vkglTF::FileLoadingFlags::PreMultiplyVertexColors | vkglTF::FileLoadingFlags::FlipY;
		scene.loadFromFile(getAssetPath() + "models/treasure_smooth.gltf", vulkanDevice, queue, glTFLoadingFlags);
	}

	void setupDescriptors()
	{
		// Pool
		std::vector<VkDescriptorPoolSize> poolSizes = {
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, maxConcurrentFrames)
		};
		VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, maxConcurrentFrames);
		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));
		// Layout
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0)
		};
		VkDescriptorSetLayoutCreateInfo descriptorLayout = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayout));
		VkPipelineLayoutCreateInfo pipelineLayoutCI = vks::initializers::pipelineLayoutCreateInfo(&descriptorSetLayout, 1);
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &pipelineLayout));
		// Sets per frame, just like the buffers themselves
		VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);
		for (auto i = 0; i < uniformBuffers.size(); i++) {
			VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets[i]));
			std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
				vks::initializers::writeDescriptorSet(descriptorSets[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffers[i].descriptor),
			};
			vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
		}
	}

	// Loads a binary shader file
	void _loadShader(std::string filename, char* &code, size_t &size) {
		// @todo: Android
		std::ifstream is(filename, std::ios::binary | std::ios::in | std::ios::ate);
		if (is.is_open())
		{
			size = is.tellg();
			is.seekg(0, std::ios::beg);
			code = new char[size];
			is.read(code, size);
			is.close();
			assert(size > 0);
		}
		else
		{
			vks::tools::exitFatal("Error: Could not open shader " + filename, VK_ERROR_UNKNOWN);
		}
	}

	void createShaderObjects()
	{
		size_t shaderCodeSizes[2]{};
		char* shaderCodes[2]{};

		VkShaderCreateInfoEXT shaderCreateInfos[2]{};

		// With VK_EXT_shader_object we can generate an implementation dependent binary file that's faster to load
		// So we check if the binray files exist and if we can load it instead of the SPIR-V
		bool binaryShadersLoaded = false;

		if (vks::tools::fileExists(getShadersPath() + "shaderobjects/phong.vert.bin") && vks::tools::fileExists(getShadersPath() + "shaderobjects/phong.frag.bin")) {
			// VS
			_loadShader(getShadersPath() + "shaderobjects/phong.vert.bin", shaderCodes[0], shaderCodeSizes[0]);
			shaderCreateInfos[0].sType = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO_EXT;
			shaderCreateInfos[0].flags = VK_SHADER_CREATE_LINK_STAGE_BIT_EXT;
			shaderCreateInfos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
			shaderCreateInfos[0].nextStage = VK_SHADER_STAGE_FRAGMENT_BIT;
			shaderCreateInfos[0].codeType = VK_SHADER_CODE_TYPE_BINARY_EXT;
			shaderCreateInfos[0].pCode = shaderCodes[0];
			shaderCreateInfos[0].codeSize = shaderCodeSizes[0];
			shaderCreateInfos[0].pName = "main";
			shaderCreateInfos[0].setLayoutCount = 1;
			shaderCreateInfos[0].pSetLayouts = &descriptorSetLayout;

			// FS
			_loadShader(getShadersPath() + "shaderobjects/phong.frag.bin", shaderCodes[1], shaderCodeSizes[1]);
			shaderCreateInfos[1].sType = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO_EXT;
			shaderCreateInfos[1].flags = VK_SHADER_CREATE_LINK_STAGE_BIT_EXT;
			shaderCreateInfos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			shaderCreateInfos[1].nextStage = 0;
			shaderCreateInfos[1].codeType = VK_SHADER_CODE_TYPE_BINARY_EXT;
			shaderCreateInfos[1].pCode = shaderCodes[1];
			shaderCreateInfos[1].codeSize = shaderCodeSizes[1];
			shaderCreateInfos[1].pName = "main";
			shaderCreateInfos[1].setLayoutCount = 1;
			shaderCreateInfos[1].pSetLayouts = &descriptorSetLayout;

			VkResult result = vkCreateShadersEXT(device, 2, shaderCreateInfos, nullptr, shaders);
			// If the function returns e.g. VK_ERROR_INCOMPATIBLE_SHADER_BINARY_EXT, the binary file is no longer (or not at all) compatible with the current implementation
			if (result == VK_SUCCESS) {
				binaryShadersLoaded = true;
			} else {
				std::cout << "Could not load binary shader files (" << vks::tools::errorString(result) << ", loading SPIR - V instead\n";
			}
		}

		// If the binary files weren't present, or we could not load them, we load from SPIR-V
		if (!binaryShadersLoaded) {
			// VS
			_loadShader(getShadersPath() + "shaderobjects/phong.vert.spv", shaderCodes[0], shaderCodeSizes[0]);
			shaderCreateInfos[0].sType = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO_EXT;
			shaderCreateInfos[0].flags = VK_SHADER_CREATE_LINK_STAGE_BIT_EXT;
			shaderCreateInfos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
			shaderCreateInfos[0].nextStage = VK_SHADER_STAGE_FRAGMENT_BIT;
			shaderCreateInfos[0].codeType = VK_SHADER_CODE_TYPE_SPIRV_EXT;
			shaderCreateInfos[0].pCode = shaderCodes[0];
			shaderCreateInfos[0].codeSize = shaderCodeSizes[0];
			shaderCreateInfos[0].pName = "main";
			shaderCreateInfos[0].setLayoutCount = 1;
			shaderCreateInfos[0].pSetLayouts = &descriptorSetLayout;

			// FS
			_loadShader(getShadersPath() + "shaderobjects/phong.frag.spv", shaderCodes[1], shaderCodeSizes[1]);
			shaderCreateInfos[1].sType = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO_EXT;
			shaderCreateInfos[1].flags = VK_SHADER_CREATE_LINK_STAGE_BIT_EXT;
			shaderCreateInfos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			shaderCreateInfos[1].nextStage = 0;
			shaderCreateInfos[1].codeType = VK_SHADER_CODE_TYPE_SPIRV_EXT;
			shaderCreateInfos[1].pCode = shaderCodes[1];
			shaderCreateInfos[1].codeSize = shaderCodeSizes[1];
			shaderCreateInfos[1].pName = "main";
			shaderCreateInfos[1].setLayoutCount = 1;
			shaderCreateInfos[1].pSetLayouts = &descriptorSetLayout;

			VK_CHECK_RESULT(vkCreateShadersEXT(device, 2, shaderCreateInfos, nullptr, shaders));

			// Store the binary shader files so we can try to load them at the next start
			size_t dataSize{ 0 };
			char* data{ nullptr };
			std::fstream is;

			vkGetShaderBinaryDataEXT(device, shaders[0], &dataSize, nullptr);
			data = new char[dataSize];
			vkGetShaderBinaryDataEXT(device, shaders[0], &dataSize, data);
			is.open(getShadersPath() + "shaderobjects/phong.vert.bin", std::ios::binary | std::ios::out);
			is.write(data, dataSize);
			is.close();
			delete[] data;

			vkGetShaderBinaryDataEXT(device, shaders[1], &dataSize, nullptr);
			data = new char[dataSize];
			vkGetShaderBinaryDataEXT(device, shaders[1], &dataSize, data);
			is.open(getShadersPath() + "shaderobjects/phong.frag.bin", std::ios::binary | std::ios::out);
			is.write(data, dataSize);
			is.close();
			delete[] data;
		}
	}

	// Prepare and initialize uniform buffer containing shader uniforms
	void prepareUniformBuffers()
	{
		for (auto& buffer : uniformBuffers) {
			VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &buffer, sizeof(UniformData), &uniformData));
			VK_CHECK_RESULT(buffer.map());
		}
	}

	void updateUniformBuffers()
	{
		uniformData.projection = camera.matrices.perspective;
		uniformData.modelView = camera.matrices.view;
		memcpy(uniformBuffers[currentBuffer].mapped, &uniformData, sizeof(UniformData));
	}

	void prepare()
	{
		VulkanExampleBase::prepare();

		// As this is an extension, we need to explicitly load the function pointers for the shader object commands used in this sample

		vkCreateShadersEXT = reinterpret_cast<PFN_vkCreateShadersEXT>(vkGetDeviceProcAddr(device, "vkCreateShadersEXT"));
		vkDestroyShaderEXT = reinterpret_cast<PFN_vkDestroyShaderEXT>(vkGetDeviceProcAddr(device, "vkDestroyShaderEXT"));
		vkCmdBindShadersEXT = reinterpret_cast<PFN_vkCmdBindShadersEXT>(vkGetDeviceProcAddr(device, "vkCmdBindShadersEXT"));
		vkGetShaderBinaryDataEXT = reinterpret_cast<PFN_vkGetShaderBinaryDataEXT>(vkGetDeviceProcAddr(device, "vkGetShaderBinaryDataEXT"));

		vkCmdBeginRenderingKHR = reinterpret_cast<PFN_vkCmdBeginRenderingKHR>(vkGetDeviceProcAddr(device, "vkCmdBeginRenderingKHR"));
		vkCmdEndRenderingKHR = reinterpret_cast<PFN_vkCmdEndRenderingKHR>(vkGetDeviceProcAddr(device, "vkCmdEndRenderingKHR"));

		vkCmdSetAlphaToCoverageEnableEXT = reinterpret_cast<PFN_vkCmdSetAlphaToCoverageEnableEXT>(vkGetDeviceProcAddr(device, "vkCmdSetAlphaToCoverageEnableEXT"));
		vkCmdSetColorBlendEnableEXT = reinterpret_cast<PFN_vkCmdSetColorBlendEnableEXT>(vkGetDeviceProcAddr(device, "vkCmdSetColorBlendEnableEXT"));
		vkCmdSetColorWriteMaskEXT = reinterpret_cast<PFN_vkCmdSetColorWriteMaskEXT>(vkGetDeviceProcAddr(device, "vkCmdSetColorWriteMaskEXT"));
		vkCmdSetCullModeEXT = reinterpret_cast<PFN_vkCmdSetCullModeEXT>(vkGetDeviceProcAddr(device, "vkCmdSetCullModeEXT"));
		vkCmdSetDepthBiasEnableEXT = reinterpret_cast<PFN_vkCmdSetDepthBiasEnableEXT>(vkGetDeviceProcAddr(device, "vkCmdSetDepthBiasEnableEXT"));
		vkCmdSetDepthCompareOpEXT = reinterpret_cast<PFN_vkCmdSetDepthCompareOpEXT>(vkGetDeviceProcAddr(device, "vkCmdSetDepthCompareOpEXT"));
		vkCmdSetDepthTestEnableEXT = reinterpret_cast<PFN_vkCmdSetDepthTestEnableEXT>(vkGetDeviceProcAddr(device, "vkCmdSetDepthTestEnableEXT"));
		vkCmdSetDepthWriteEnableEXT = reinterpret_cast<PFN_vkCmdSetDepthWriteEnableEXT>(vkGetDeviceProcAddr(device, "vkCmdSetDepthWriteEnableEXT"));
		vkCmdSetFrontFaceEXT = reinterpret_cast<PFN_vkCmdSetFrontFaceEXT>(vkGetDeviceProcAddr(device, "vkCmdSetFrontFaceEXT"));
		vkCmdSetPolygonModeEXT = reinterpret_cast<PFN_vkCmdSetPolygonModeEXT>(vkGetDeviceProcAddr(device, "vkCmdSetPolygonModeEXT"));
		vkCmdSetPrimitiveRestartEnableEXT = reinterpret_cast<PFN_vkCmdSetPrimitiveRestartEnableEXT>(vkGetDeviceProcAddr(device, "vkCmdSetPrimitiveRestartEnableEXT"));
		vkCmdSetPrimitiveTopologyEXT = reinterpret_cast<PFN_vkCmdSetPrimitiveTopologyEXT>(vkGetDeviceProcAddr(device, "vkCmdSetPrimitiveTopologyEXT"));
		vkCmdSetRasterizationSamplesEXT = reinterpret_cast<PFN_vkCmdSetRasterizationSamplesEXT>(vkGetDeviceProcAddr(device, "vkCmdSetRasterizationSamplesEXT"));
		vkCmdSetRasterizerDiscardEnableEXT = reinterpret_cast<PFN_vkCmdSetRasterizerDiscardEnableEXT>(vkGetDeviceProcAddr(device, "vkCmdSetRasterizerDiscardEnableEXT"));
		vkCmdSetSampleMaskEXT = reinterpret_cast<PFN_vkCmdSetSampleMaskEXT>(vkGetDeviceProcAddr(device, "vkCmdSetSampleMaskEXT"));
		vkCmdSetScissorWithCountEXT = reinterpret_cast<PFN_vkCmdSetScissorWithCountEXT>(vkGetDeviceProcAddr(device, "vkCmdSetScissorWithCountEXT"));
		vkCmdSetStencilTestEnableEXT = reinterpret_cast<PFN_vkCmdSetStencilTestEnableEXT>(vkGetDeviceProcAddr(device, "vkCmdSetStencilTestEnableEXT"));
		vkCmdSetVertexInputEXT = reinterpret_cast<PFN_vkCmdSetVertexInputEXT>(vkGetDeviceProcAddr(device, "vkCmdSetVertexInputEXT"));
		vkCmdSetViewportWithCountEXT = reinterpret_cast<PFN_vkCmdSetViewportWithCountEXT>(vkGetDeviceProcAddr(device, "vkCmdSetViewportWithCountEXT"));;

		loadAssets();
		prepareUniformBuffers();
		setupDescriptors();
		createShaderObjects();
		prepared = true;
	}

	void buildCommandBuffer()
	{
		VkCommandBuffer cmdBuffer = drawCmdBuffers[currentBuffer];
		
		VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

		VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));

		// Transition color and depth images for drawing
		vks::tools::insertImageMemoryBarrier(
			cmdBuffer,
			swapChain.images[currentImageIndex],
			0,
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
		vks::tools::insertImageMemoryBarrier(
			cmdBuffer,
			depthStencil.image,
			0,
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			VkImageSubresourceRange{ VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1 });

		// New structures are used to define the attachments used in dynamic rendering
		VkRenderingAttachmentInfoKHR colorAttachment{};
		colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
		colorAttachment.imageView = swapChain.imageViews[currentImageIndex];
		colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.clearValue.color = { 0.0f,0.0f,0.0f,0.0f };

		// A single depth stencil attachment info can be used, but they can also be specified separately.
		// When both are specified separately, the only requirement is that the image view is identical.			
		VkRenderingAttachmentInfoKHR depthStencilAttachment{};
		depthStencilAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
		depthStencilAttachment.imageView = depthStencil.view;
		depthStencilAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		depthStencilAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthStencilAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		depthStencilAttachment.clearValue.depthStencil = { 1.0f,  0 };

		VkRenderingInfoKHR renderingInfo{};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
		renderingInfo.renderArea = { 0, 0, width, height };
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachments = &colorAttachment;
		renderingInfo.pDepthAttachment = &depthStencilAttachment;
		renderingInfo.pStencilAttachment = &depthStencilAttachment;

		// Begin dynamic rendering
		vkCmdBeginRenderingKHR(cmdBuffer, &renderingInfo);

		VkViewport viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
		VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);

		// No more pipelines required, everything is bound at command buffer level
		// This also means that we need to explicitly set a lot of the state to be spec compliant

		vkCmdSetViewportWithCountEXT(cmdBuffer, 1, &viewport);
		vkCmdSetScissorWithCountEXT(cmdBuffer, 1, &scissor);
		vkCmdSetCullModeEXT(cmdBuffer, VK_CULL_MODE_BACK_BIT);
		vkCmdSetFrontFaceEXT(cmdBuffer, VK_FRONT_FACE_COUNTER_CLOCKWISE);
		vkCmdSetDepthTestEnableEXT(cmdBuffer, VK_TRUE);
		vkCmdSetDepthWriteEnableEXT(cmdBuffer, VK_TRUE);
		vkCmdSetDepthCompareOpEXT(cmdBuffer, VK_COMPARE_OP_LESS_OR_EQUAL);
		vkCmdSetPrimitiveTopologyEXT(cmdBuffer, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
		vkCmdSetRasterizerDiscardEnableEXT(cmdBuffer, VK_FALSE);
		vkCmdSetPolygonModeEXT(cmdBuffer, VK_POLYGON_MODE_FILL);
		vkCmdSetRasterizationSamplesEXT(cmdBuffer, VK_SAMPLE_COUNT_1_BIT);
		vkCmdSetAlphaToCoverageEnableEXT(cmdBuffer, VK_FALSE);
		vkCmdSetDepthBiasEnableEXT(cmdBuffer, VK_FALSE);
		vkCmdSetStencilTestEnableEXT(cmdBuffer, VK_FALSE);
		vkCmdSetPrimitiveRestartEnableEXT(cmdBuffer, VK_FALSE);

		const uint32_t sampleMask = 0xFF;
		vkCmdSetSampleMaskEXT(cmdBuffer, VK_SAMPLE_COUNT_1_BIT, &sampleMask);

		const VkBool32 colorBlendEnables = false;
		const VkColorComponentFlags colorBlendComponentFlags = 0xf;
		const VkColorBlendEquationEXT colorBlendEquation{};
		vkCmdSetColorBlendEnableEXT(cmdBuffer, 0, 1, &colorBlendEnables);
		vkCmdSetColorWriteMaskEXT(cmdBuffer, 0, 1, &colorBlendComponentFlags);

		VkVertexInputBindingDescription2EXT vertexInputBinding{};
		vertexInputBinding.sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_BINDING_DESCRIPTION_2_EXT;
		vertexInputBinding.binding = 0;
		vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		vertexInputBinding.stride = sizeof(vkglTF::Vertex);
		vertexInputBinding.divisor = 1;

		std::vector<VkVertexInputAttributeDescription2EXT> vertexAttributes = {
			{ VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT, nullptr, 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vkglTF::Vertex, pos) },
			{ VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT, nullptr, 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vkglTF::Vertex, normal) },
			{ VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT, nullptr, 2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(vkglTF::Vertex, color) }
		};

		vkCmdSetVertexInputEXT(cmdBuffer, 1, &vertexInputBinding, 3, vertexAttributes.data());

		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentBuffer], 0, nullptr);
		scene.bindBuffers(cmdBuffer);

		// Binding the shaders
		VkShaderStageFlagBits stages[2] = { VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT };
		vkCmdBindShadersEXT(cmdBuffer, 2, stages, shaders);
		scene.draw(cmdBuffer);

		// @todo: Currently disabled, the UI needs to be adopted to work with shader objects
		// drawUI(cmdBuffer);

		// End dynamic rendering
		vkCmdEndRenderingKHR(cmdBuffer);

		// Transition color image for presentation
		vks::tools::insertImageMemoryBarrier(
			cmdBuffer,
			swapChain.images[currentImageIndex],
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			0,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

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
	}
};

VULKAN_EXAMPLE_MAIN()
