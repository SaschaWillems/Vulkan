/*
* Vulkan Example - Variable rate shading
*
* Copyright (C) 2020-2023 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "vulkanexamplebase.h"
#include "VulkanglTFModel.h"

class VulkanExample : public VulkanExampleBase
{
public:
	vkglTF::Model scene;

	struct ShadingRateImage {
		VkImage image{ VK_NULL_HANDLE };
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
	} pipelines;

	VkPipelineLayout pipelineLayout;
	VkDescriptorSet descriptorSet;
	VkDescriptorSetLayout descriptorSetLayout;

	VkPhysicalDeviceFragmentShadingRatePropertiesKHR physicalDeviceShadingRateImageProperties{};
	VkPhysicalDeviceFragmentShadingRateFeaturesKHR enabledPhysicalDeviceShadingRateImageFeaturesKHR{};

	PFN_vkGetPhysicalDeviceFragmentShadingRatesKHR vkGetPhysicalDeviceFragmentShadingRatesKHR{ nullptr };
	PFN_vkCmdSetFragmentShadingRateKHR vkCmdSetFragmentShadingRateKHR{ nullptr };
	PFN_vkCreateRenderPass2KHR vkCreateRenderPass2KHR{ nullptr };

	VulkanExample();
	~VulkanExample();
	virtual void getEnabledFeatures() override;
	void handleResize();
	void buildCommandBuffers() override;
	void loadAssets();
	void prepareShadingRateImage();
	void setupDescriptors();
	void preparePipelines();
	void prepareUniformBuffers();
	void updateUniformBuffers();
	void prepare() override;
	void setupFrameBuffer() override;
	void setupRenderPass() override;
	virtual void render() override;
	virtual void OnUpdateUIOverlay(vks::UIOverlay* overlay) override;
};
