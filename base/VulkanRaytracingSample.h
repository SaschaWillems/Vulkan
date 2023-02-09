/*
* Extended sample base class for ray tracing based samples
*
* Copyright (C) 2020 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#include "vulkan/vulkan.h"
#include "vulkanexamplebase.h"
#include "VulkanTools.h"
#include "VulkanDevice.h"

class VulkanRaytracingSample : public VulkanExampleBase
{
protected:
	// Update the default render pass with different color attachment load ops
	virtual void updateRenderPass();
public:
	// Function pointers for ray tracing related stuff
	PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR;
	PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
	PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;
	PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
	PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;
	PFN_vkBuildAccelerationStructuresKHR vkBuildAccelerationStructuresKHR;
	PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;
	PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;
	PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR;
	PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;

	// Available features and properties
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR  rayTracingPipelineProperties{};
	VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{};

	// Enabled features and properties
	VkPhysicalDeviceBufferDeviceAddressFeatures enabledBufferDeviceAddresFeatures{};
	VkPhysicalDeviceRayTracingPipelineFeaturesKHR enabledRayTracingPipelineFeatures{};
	VkPhysicalDeviceAccelerationStructureFeaturesKHR enabledAccelerationStructureFeatures{};

	// Holds information for a ray tracing scratch buffer that is used as a temporary storage
	struct ScratchBuffer
	{
		uint64_t deviceAddress = 0;
		VkBuffer handle = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;
	};

	// Holds information for a ray tracing acceleration structure
	struct AccelerationStructure {
		VkAccelerationStructureKHR handle;
		uint64_t deviceAddress = 0;
		VkDeviceMemory memory;
		VkBuffer buffer;
	};

	// Holds information for a storage image that the ray tracing shaders output to
	struct StorageImage {
		VkDeviceMemory memory = VK_NULL_HANDLE;
		VkImage image = VK_NULL_HANDLE;
		VkImageView view = VK_NULL_HANDLE;
		VkFormat format;
	} storageImage;

	// Extends the buffer class and holds information for a shader binding table
	class ShaderBindingTable : public vks::Buffer {
	public:
		VkStridedDeviceAddressRegionKHR stridedDeviceAddressRegion{};
	};

	// Set to true, to denote that the sample only uses ray queries (changes extension and render pass handling)
	bool rayQueryOnly = false;

	void enableExtensions();
	ScratchBuffer createScratchBuffer(VkDeviceSize size);
	void deleteScratchBuffer(ScratchBuffer& scratchBuffer);
	void createAccelerationStructure(AccelerationStructure& accelerationStructure, VkAccelerationStructureTypeKHR type, VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo);
	void deleteAccelerationStructure(AccelerationStructure& accelerationStructure);
	uint64_t getBufferDeviceAddress(VkBuffer buffer);
	void createStorageImage(VkFormat format, VkExtent3D extent);
	void deleteStorageImage();
	VkStridedDeviceAddressRegionKHR getSbtEntryStridedDeviceAddressRegion(VkBuffer buffer, uint32_t handleCount);
	void createShaderBindingTable(ShaderBindingTable& shaderBindingTable, uint32_t handleCount);
	// Draw the ImGUI UI overlay using a render pass
	void drawUI(VkCommandBuffer commandBuffer, VkFramebuffer framebuffer);

	virtual void prepare();
};
