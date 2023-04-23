/*
 * Vulkan Example - Using shader objects via VK_EXT_shader_object
 *
 * Copyright (C) 2023 by Sascha Willems - www.saschawillems.de
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */

#include "vulkanexamplebase.h"
#include "VulkanglTFModel.h"

#define ENABLE_VALIDATION false

class VulkanExample: public VulkanExampleBase
{
public:
	vkglTF::Model scene;

	// Same uniform buffer layout as shader
	struct UBOVS {
		glm::mat4 projection;
		glm::mat4 modelView;
		glm::vec4 lightPos = glm::vec4(0.0f, 2.0f, 1.0f, 0.0f);
	} uboVS;
	vks::Buffer uniformBuffer;

	VkPipelineLayout pipelineLayout;
	VkDescriptorSet descriptorSet;
	VkDescriptorSetLayout descriptorSetLayout;

	VkShaderEXT shaders[2];

	VkPhysicalDeviceShaderObjectFeaturesEXT enabledDeviceShaderObjectFeaturesEXT{};

	PFN_vkCreateShadersEXT vkCreateShadersEXT;
	PFN_vkCmdBindShadersEXT vkCmdBindShadersEXT;	

	// With VK_EXT_shader_object pipeline state must be set at command buffer creation using these functions
	// VK_EXT_dynamic_state
	PFN_vkCmdSetViewportWithCountEXT vkCmdSetViewportWithCountEXT;
	PFN_vkCmdSetScissorWithCountEXT vkCmdSetScissorWithCountEXT;
	PFN_vkCmdSetDepthCompareOpEXT vkCmdSetDepthCompareOpEXT;
	PFN_vkCmdSetCullModeEXT vkCmdSetCullModeEXT;
	PFN_vkCmdSetDepthTestEnableEXT vkCmdSetDepthTestEnableEXT;
	PFN_vkCmdSetDepthWriteEnableEXT vkCmdSetDepthWriteEnableEXT;
	PFN_vkCmdSetFrontFaceEXT vkCmdSetFrontFaceEXT;
	PFN_vkCmdSetPrimitiveTopologyEXT vkCmdSetPrimitiveTopologyEXT;
	// VK_EXT_vertex_input_dynamic_state
	PFN_vkCmdSetVertexInputEXT vkCmdSetVertexInputEXT;

	VulkanExample() : VulkanExampleBase(ENABLE_VALIDATION)
	{
		title = "Shader objects (VK_EXT_shader_object)";
		camera.type = Camera::CameraType::lookat;
		camera.setPosition(glm::vec3(0.0f, 0.0f, -10.5f));
		camera.setRotation(glm::vec3(-25.0f, 15.0f, 0.0f));
		camera.setRotationSpeed(0.5f);
		camera.setPerspective(60.0f, (float)(width) / (float)height, 0.1f, 256.0f);

		enabledDeviceExtensions.push_back(VK_EXT_SHADER_OBJECT_EXTENSION_NAME);
		// With VK_EXT_shader_object all baked pipeline state is set dynamically at command buffer creation, so we need to enable additional extensions
		enabledDeviceExtensions.push_back(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME);
		enabledDeviceExtensions.push_back(VK_EXT_VERTEX_INPUT_DYNAMIC_STATE_EXTENSION_NAME);

		enabledDeviceShaderObjectFeaturesEXT.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_OBJECT_FEATURES_EXT;
		enabledDeviceShaderObjectFeaturesEXT.shaderObject = VK_TRUE;

		deviceCreatepNextChain = &enabledDeviceShaderObjectFeaturesEXT;
	}

	~VulkanExample()
	{
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
		// @todo: destroy shaders
		uniformBuffer.destroy();
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
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1)
		};
		VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, 2);
		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));
		// Layout
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
			// Binding 0 : Vertex shader uniform buffer
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0)
		};
		VkDescriptorSetLayoutCreateInfo descriptorLayout = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayout));
		VkPipelineLayoutCreateInfo pipelineLayoutCI = vks::initializers::pipelineLayoutCreateInfo(&descriptorSetLayout, 1);
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &pipelineLayout));
		// Sets
		VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));
		std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
			// Binding 0 : Vertex shader uniform buffer
			vks::initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffer.descriptor) 
		};
		vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
	}

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

		// VS
		_loadShader(getShadersPath() + "pipelines/phong.vert.spv", shaderCodes[0], shaderCodeSizes[0]);
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
		_loadShader(getShadersPath() + "pipelines/phong.frag.spv", shaderCodes[1], shaderCodeSizes[1]);
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

		vkCreateShadersEXT(device, 2, shaderCreateInfos, nullptr, shaders);
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

			VkViewport viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
			VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);

			// No more pipelines required, everything is bound at command buffer level

			vkCmdSetViewportWithCountEXT(drawCmdBuffers[i], 1, &viewport);
			vkCmdSetScissorWithCountEXT(drawCmdBuffers[i], 1, &scissor);
			vkCmdSetCullModeEXT(drawCmdBuffers[i], VK_CULL_MODE_BACK_BIT);
			vkCmdSetFrontFaceEXT(drawCmdBuffers[i], VK_FRONT_FACE_COUNTER_CLOCKWISE);
			vkCmdSetDepthTestEnableEXT(drawCmdBuffers[i], VK_TRUE);
			vkCmdSetDepthWriteEnableEXT(drawCmdBuffers[i], VK_TRUE);
			vkCmdSetDepthCompareOpEXT(drawCmdBuffers[i], VK_COMPARE_OP_LESS_OR_EQUAL);
			vkCmdSetPrimitiveTopologyEXT(drawCmdBuffers[i], VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

			VkVertexInputBindingDescription2EXT vertexInputBinding{};
			vertexInputBinding.sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_BINDING_DESCRIPTION_2_EXT;
			vertexInputBinding.binding = 0;
			vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			vertexInputBinding.stride = sizeof(vkglTF::Vertex);

			std::vector<VkVertexInputAttributeDescription2EXT> vertexAttributes = {
				{ VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT, nullptr, 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vkglTF::Vertex, pos) },
				{ VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT, nullptr, 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vkglTF::Vertex, normal) },
				{ VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT, nullptr, 2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(vkglTF::Vertex, color) }
			};

			vkCmdSetVertexInputEXT(drawCmdBuffers[i], 1, &vertexInputBinding, 3, vertexAttributes.data());

			vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
			scene.bindBuffers(drawCmdBuffers[i]);

			// Binding the shaders
			VkShaderStageFlagBits stages[2] = { VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT };
			vkCmdBindShadersEXT(drawCmdBuffers[i], 2, stages, shaders);
			scene.draw(drawCmdBuffers[i]);

			drawUI(drawCmdBuffers[i]);

			vkCmdEndRenderPass(drawCmdBuffers[i]);
			VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
		}
	}

	// Prepare and initialize uniform buffer containing shader uniforms
	void prepareUniformBuffers()
	{
		// Create the vertex shader uniform buffer block
		VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &uniformBuffer, sizeof(uboVS)));
		VK_CHECK_RESULT(uniformBuffer.map());
		updateUniformBuffers();
	}

	void updateUniformBuffers()
	{
		uboVS.projection = camera.matrices.perspective;
		uboVS.modelView = camera.matrices.view;
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

		vkCreateShadersEXT = reinterpret_cast<PFN_vkCreateShadersEXT>(vkGetDeviceProcAddr(device, "vkCreateShadersEXT"));
		vkCmdBindShadersEXT = reinterpret_cast<PFN_vkCmdBindShadersEXT>(vkGetDeviceProcAddr(device, "vkCmdBindShadersEXT"));

		vkCmdSetViewportWithCountEXT = reinterpret_cast<PFN_vkCmdSetViewportWithCountEXT>(vkGetDeviceProcAddr(device, "vkCmdSetViewportWithCountEXT"));;
		vkCmdSetScissorWithCountEXT = reinterpret_cast<PFN_vkCmdSetScissorWithCountEXT>(vkGetDeviceProcAddr(device, "vkCmdSetScissorWithCountEXT"));
		vkCmdSetDepthCompareOpEXT = reinterpret_cast<PFN_vkCmdSetDepthCompareOpEXT>(vkGetDeviceProcAddr(device, "vkCmdSetDepthCompareOpEXT"));
		vkCmdSetCullModeEXT = reinterpret_cast<PFN_vkCmdSetCullModeEXT>(vkGetDeviceProcAddr(device, "vkCmdSetCullModeEXT"));
		vkCmdSetDepthTestEnableEXT = reinterpret_cast<PFN_vkCmdSetDepthTestEnableEXT>(vkGetDeviceProcAddr(device, "vkCmdSetDepthTestEnableEXT"));
		vkCmdSetDepthWriteEnableEXT = reinterpret_cast<PFN_vkCmdSetDepthWriteEnableEXT>(vkGetDeviceProcAddr(device, "vkCmdSetDepthWriteEnableEXT"));
		vkCmdSetFrontFaceEXT = reinterpret_cast<PFN_vkCmdSetFrontFaceEXT>(vkGetDeviceProcAddr(device, "vkCmdSetFrontFaceEXT"));
		vkCmdSetPrimitiveTopologyEXT = reinterpret_cast<PFN_vkCmdSetPrimitiveTopologyEXT>(vkGetDeviceProcAddr(device, "vkCmdSetPrimitiveTopologyEXT"));
		vkCmdSetVertexInputEXT = reinterpret_cast<PFN_vkCmdSetVertexInputEXT>(vkGetDeviceProcAddr(device, "vkCmdSetVertexInputEXT"));

		loadAssets();
		prepareUniformBuffers();
		setupDescriptors();
		createShaderObjects();
		buildCommandBuffers();
		prepared = true;
	}

	virtual void render()
	{
		if (!prepared)
			return;
		draw();
		updateUniformBuffers();
	}
};

VULKAN_EXAMPLE_MAIN()
