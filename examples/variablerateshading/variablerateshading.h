/*
* Vulkan Example - Variable rate shading
*
* Copyright (C) 2020 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "vulkanexamplebase.h"
#include "VulkanglTFModel.h"

#define ENABLE_VALIDATION false

class VulkanExample : public VulkanExampleBase
{
public:
	vkglTF::Model scene;

	struct ShadingRateImage {
		VkImage image;
		VkDeviceMemory memory;
		VkImageView view;
	} shadingRateImage;

	bool enableShadingRate = true;
	bool colorShadingRate = false;

	struct ShaderData {
		vks::Buffer buffer;
		struct Values {
			glm::mat4 projection;
			glm::mat4 view;
			glm::mat4 model = glm::mat4(1.0f);
			glm::vec4 lightPos = glm::vec4(0.0f, 2.5f, 0.0f, 1.0f);
			glm::vec4 viewPos;
			int32_t colorShadingRate;
		} values;
	} shaderData;

	struct Pipelines {
		VkPipeline opaque;
		VkPipeline masked;
	};

	Pipelines basePipelines;
	Pipelines shadingRatePipelines;

	VkPipelineLayout pipelineLayout;
	VkDescriptorSet descriptorSet;
	VkDescriptorSetLayout descriptorSetLayout;

	VkPhysicalDeviceShadingRateImagePropertiesNV physicalDeviceShadingRateImagePropertiesNV{};
	VkPhysicalDeviceShadingRateImageFeaturesNV enabledPhysicalDeviceShadingRateImageFeaturesNV{};
	PFN_vkCmdBindShadingRateImageNV vkCmdBindShadingRateImageNV;

	VulkanExample();
	~VulkanExample();
	virtual void getEnabledFeatures();
	void handleResize();
	void buildCommandBuffers();
	void loadglTFFile(std::string filename);
	void loadAssets();
	void prepareShadingRateImage();
	void setupDescriptors();
	void preparePipelines();
	void prepareUniformBuffers();
	void updateUniformBuffers();
	void prepare();
	virtual void render();
	virtual void OnUpdateUIOverlay(vks::UIOverlay* overlay);
};