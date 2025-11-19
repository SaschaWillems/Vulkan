/*
* Extended sample base class for ray tracing based samples
*
* Copyright (C) 2020-2025 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "VulkanRaytracingSample.h"

void VulkanRaytracingSample::setupRenderPass()
{
	// Update the default render pass with different color attachment load ops to keep attachment contents
	// With this change, we can e.g. draw an UI on top of the ray traced scene

	vkDestroyRenderPass(device, renderPass, nullptr);

	VkAttachmentLoadOp colorLoadOp{ VK_ATTACHMENT_LOAD_OP_LOAD };
	VkImageLayout colorInitialLayout{ VK_IMAGE_LAYOUT_PRESENT_SRC_KHR };
	
	if (rayQueryOnly) {
		colorLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorInitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	}

	std::array<VkAttachmentDescription, 2> attachments{
		VkAttachmentDescription{
			.format = swapChain.colorFormat,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = colorLoadOp,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = colorInitialLayout,
			.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		},
		VkAttachmentDescription{
			.format = depthFormat,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		}
	};

	VkAttachmentReference colorReference{ .attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	VkAttachmentReference depthReference{ .attachment = 1,.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

	VkSubpassDescription subpassDescription{
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorReference,
		.pDepthStencilAttachment = &depthReference,
	};

	std::array<VkSubpassDependency, 2> dependencies{
		VkSubpassDependency{
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			.srcAccessMask = VK_ACCESS_NONE_KHR,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
		},
		VkSubpassDependency{
			.srcSubpass = 0,
			.dstSubpass = VK_SUBPASS_EXTERNAL,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
		}
	};

	VkRenderPassCreateInfo renderPassInfo{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = static_cast<uint32_t>(attachments.size()),
		.pAttachments = attachments.data(),
		.subpassCount = 1,
		.pSubpasses = &subpassDescription,
		.dependencyCount = static_cast<uint32_t>(dependencies.size()),
		.pDependencies = dependencies.data()
	};
	VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));
}

void VulkanRaytracingSample::setupFrameBuffer()
{

	frameBuffers.resize(swapChain.images.size());
	for (uint32_t i = 0; i < frameBuffers.size(); i++) {
		VkImageView attachments[2] { swapChain.imageViews[i], depthStencil.view };
		VkFramebufferCreateInfo frameBufferCreateInfo{
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = renderPass,
			.attachmentCount = 2,
			.pAttachments = attachments,
			.width = width,
			.height = height,
			.layers = 1
		};
		VK_CHECK_RESULT(vkCreateFramebuffer(device, &frameBufferCreateInfo, nullptr, &frameBuffers[i]));
	}
}

void VulkanRaytracingSample::enableExtensions()
{
	// Require Vulkan 1.1
	apiVersion = VK_API_VERSION_1_1;

	// Ray tracing related extensions required by this sample
	enabledDeviceExtensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
	if (!rayQueryOnly) {
		enabledDeviceExtensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
	}

	// Required by VK_KHR_acceleration_structure
	enabledDeviceExtensions.push_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
	enabledDeviceExtensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
	enabledDeviceExtensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);

	// Required for VK_KHR_ray_tracing_pipeline
	enabledDeviceExtensions.push_back(VK_KHR_SPIRV_1_4_EXTENSION_NAME);

	// Required by VK_KHR_spirv_1_4
	enabledDeviceExtensions.push_back(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);

	// Format for the storage image is decided at runtime, so we can't explicilily state it in the shader
	enabledFeatures.shaderStorageImageWriteWithoutFormat = VK_TRUE;
	enabledFeatures.shaderStorageImageReadWithoutFormat = VK_TRUE;
}

VulkanRaytracingSample::ScratchBuffer VulkanRaytracingSample::createScratchBuffer(VkDeviceSize size)
{
	ScratchBuffer scratchBuffer{};
	VkBufferCreateInfo bufferCreateInfo{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.flags = 0,
		.size = size,
		.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE
	};
	VK_CHECK_RESULT(vkCreateBuffer(vulkanDevice->logicalDevice, &bufferCreateInfo, nullptr, &scratchBuffer.handle));
	VkMemoryRequirements memoryRequirements{};
	vkGetBufferMemoryRequirements(vulkanDevice->logicalDevice, scratchBuffer.handle, &memoryRequirements);
	VkMemoryAllocateFlagsInfo memoryAllocateFlagsInfo{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
		.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR
	};
	VkMemoryAllocateInfo memoryAllocateInfo{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = &memoryAllocateFlagsInfo,
		.allocationSize = memoryRequirements.size,
		.memoryTypeIndex = vulkanDevice->getMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
	};
	VK_CHECK_RESULT(vkAllocateMemory(vulkanDevice->logicalDevice, &memoryAllocateInfo, nullptr, &scratchBuffer.memory));
	VK_CHECK_RESULT(vkBindBufferMemory(vulkanDevice->logicalDevice, scratchBuffer.handle, scratchBuffer.memory, 0));
	VkBufferDeviceAddressInfoKHR bufferDeviceAddresInfo{
		.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
		.buffer = scratchBuffer.handle
	};
	scratchBuffer.deviceAddress = vkGetBufferDeviceAddressKHR(vulkanDevice->logicalDevice, &bufferDeviceAddresInfo);
	return scratchBuffer;
}

void VulkanRaytracingSample::deleteScratchBuffer(ScratchBuffer& scratchBuffer)
{
	if (scratchBuffer.memory != VK_NULL_HANDLE) {
		vkFreeMemory(vulkanDevice->logicalDevice, scratchBuffer.memory, nullptr);
	}
	if (scratchBuffer.handle != VK_NULL_HANDLE) {
		vkDestroyBuffer(vulkanDevice->logicalDevice, scratchBuffer.handle, nullptr);
	}
}

void VulkanRaytracingSample::createAccelerationStructure(AccelerationStructure& accelerationStructure, VkAccelerationStructureTypeKHR type, VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo)
{
	VkBufferCreateInfo bufferCreateInfo{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = buildSizeInfo.accelerationStructureSize,
		.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE
	};
	VK_CHECK_RESULT(vkCreateBuffer(vulkanDevice->logicalDevice, &bufferCreateInfo, nullptr, &accelerationStructure.buffer));
	VkMemoryRequirements memoryRequirements{};
	vkGetBufferMemoryRequirements(vulkanDevice->logicalDevice, accelerationStructure.buffer, &memoryRequirements);
	VkMemoryAllocateFlagsInfo memoryAllocateFlagsInfo{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
		.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR
	};
	VkMemoryAllocateInfo memoryAllocateInfo{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = &memoryAllocateFlagsInfo,
		.allocationSize = memoryRequirements.size,
		.memoryTypeIndex = vulkanDevice->getMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
	};
	VK_CHECK_RESULT(vkAllocateMemory(vulkanDevice->logicalDevice, &memoryAllocateInfo, nullptr, &accelerationStructure.memory));
	VK_CHECK_RESULT(vkBindBufferMemory(vulkanDevice->logicalDevice, accelerationStructure.buffer, accelerationStructure.memory, 0));
	VkAccelerationStructureCreateInfoKHR accelerationStructureCreate_info{
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
		.buffer = accelerationStructure.buffer,
		.size = buildSizeInfo.accelerationStructureSize,
		.type = type
	};
	vkCreateAccelerationStructureKHR(vulkanDevice->logicalDevice, &accelerationStructureCreate_info, nullptr, &accelerationStructure.handle);
	VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
		.accelerationStructure = accelerationStructure.handle
	};
	accelerationStructure.deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(vulkanDevice->logicalDevice, &accelerationDeviceAddressInfo);
}

void VulkanRaytracingSample::deleteAccelerationStructure(AccelerationStructure& accelerationStructure)
{
	vkFreeMemory(device, accelerationStructure.memory, nullptr);
	vkDestroyBuffer(device, accelerationStructure.buffer, nullptr);
	vkDestroyAccelerationStructureKHR(device, accelerationStructure.handle, nullptr);
}

uint64_t VulkanRaytracingSample::getBufferDeviceAddress(VkBuffer buffer)
{
	VkBufferDeviceAddressInfoKHR bufferDeviceAI{
		.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
		.buffer = buffer
	};
	return vkGetBufferDeviceAddressKHR(vulkanDevice->logicalDevice, &bufferDeviceAI);
}

void VulkanRaytracingSample::createStorageImage(VkFormat format, VkExtent3D extent)
{
	// Release ressources if image is to be recreated
	if (storageImage.image != VK_NULL_HANDLE) {
		vkDestroyImageView(device, storageImage.view, nullptr);
		vkDestroyImage(device, storageImage.image, nullptr);
		vkFreeMemory(device, storageImage.memory, nullptr);
		storageImage = {};
	}

	VkImageCreateInfo image{
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = format,
		.extent = extent,
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
	};
	VK_CHECK_RESULT(vkCreateImage(vulkanDevice->logicalDevice, &image, nullptr, &storageImage.image));

	VkMemoryRequirements memReqs;
	vkGetImageMemoryRequirements(vulkanDevice->logicalDevice, storageImage.image, &memReqs);
	VkMemoryAllocateInfo memoryAllocateInfo = vks::initializers::memoryAllocateInfo();
	memoryAllocateInfo.allocationSize = memReqs.size;
	memoryAllocateInfo.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK_RESULT(vkAllocateMemory(vulkanDevice->logicalDevice, &memoryAllocateInfo, nullptr, &storageImage.memory));
	VK_CHECK_RESULT(vkBindImageMemory(vulkanDevice->logicalDevice, storageImage.image, storageImage.memory, 0));

	VkImageViewCreateInfo colorImageView{
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = storageImage.image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = format,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		},
	};
	VK_CHECK_RESULT(vkCreateImageView(vulkanDevice->logicalDevice, &colorImageView, nullptr, &storageImage.view));

	VkCommandBuffer cmdBuffer = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
	vks::tools::setImageLayout(cmdBuffer, storageImage.image,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_GENERAL,
		{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
	vulkanDevice->flushCommandBuffer(cmdBuffer, queue);
}

void VulkanRaytracingSample::deleteStorageImage()
{
	vkDestroyImageView(vulkanDevice->logicalDevice, storageImage.view, nullptr);
	vkDestroyImage(vulkanDevice->logicalDevice, storageImage.image, nullptr);
	vkFreeMemory(vulkanDevice->logicalDevice, storageImage.memory, nullptr);
}

void VulkanRaytracingSample::prepare()
{
	VulkanExampleBase::prepare();
	// Get properties and features
	rayTracingPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
	VkPhysicalDeviceProperties2 deviceProperties2{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
		.pNext = &rayTracingPipelineProperties
	};
	vkGetPhysicalDeviceProperties2(physicalDevice, &deviceProperties2);
	accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
	VkPhysicalDeviceFeatures2 deviceFeatures2{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
		.pNext = &accelerationStructureFeatures
	};
	vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures2);
	// Get the function pointers required for ray tracing
	vkGetBufferDeviceAddressKHR = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(vkGetDeviceProcAddr(device, "vkGetBufferDeviceAddressKHR"));
	vkCmdBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(device, "vkCmdBuildAccelerationStructuresKHR"));
	vkBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(device, "vkBuildAccelerationStructuresKHR"));
	vkCreateAccelerationStructureKHR = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureKHR"));
	vkDestroyAccelerationStructureKHR = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(vkGetDeviceProcAddr(device, "vkDestroyAccelerationStructureKHR"));
	vkGetAccelerationStructureBuildSizesKHR = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(device, "vkGetAccelerationStructureBuildSizesKHR"));
	vkGetAccelerationStructureDeviceAddressKHR = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(device, "vkGetAccelerationStructureDeviceAddressKHR"));
	vkCmdTraceRaysKHR = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(device, "vkCmdTraceRaysKHR"));
	vkGetRayTracingShaderGroupHandlesKHR = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(vkGetDeviceProcAddr(device, "vkGetRayTracingShaderGroupHandlesKHR"));
	vkCreateRayTracingPipelinesKHR = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesKHR"));
}

VkStridedDeviceAddressRegionKHR VulkanRaytracingSample::getSbtEntryStridedDeviceAddressRegion(VkBuffer buffer, uint32_t handleCount)
{
	const uint32_t handleSizeAligned = vks::tools::alignedSize(rayTracingPipelineProperties.shaderGroupHandleSize, rayTracingPipelineProperties.shaderGroupHandleAlignment);
	VkStridedDeviceAddressRegionKHR stridedDeviceAddressRegionKHR{
		.deviceAddress = getBufferDeviceAddress(buffer),
		.stride = handleSizeAligned
	};
	stridedDeviceAddressRegionKHR.size = handleCount * handleSizeAligned;
	return stridedDeviceAddressRegionKHR;
}

void VulkanRaytracingSample::createShaderBindingTable(ShaderBindingTable& shaderBindingTable, uint32_t handleCount)
{
	// Create buffer to hold all shader handles for the SBT
	VK_CHECK_RESULT(vulkanDevice->createBuffer(
		VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
		&shaderBindingTable, 
		rayTracingPipelineProperties.shaderGroupHandleSize * handleCount));
	// Get the strided address to be used when dispatching the rays
	shaderBindingTable.stridedDeviceAddressRegion = getSbtEntryStridedDeviceAddressRegion(shaderBindingTable.buffer, handleCount);
	// Map persistent 
	shaderBindingTable.map();
}

void VulkanRaytracingSample::drawUI(VkCommandBuffer commandBuffer, VkFramebuffer framebuffer)
{
	VkClearValue clearValues[2]{};
	clearValues[0].color = defaultClearColor;
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassBeginInfo{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = renderPass,
		.framebuffer = framebuffer,
		.renderArea = {.offset = {.x = 0, .y = 0, }, .extent = {.width = width, .height = height } },
		.clearValueCount = 2,
		.pClearValues = clearValues
	};
	vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	VulkanExampleBase::drawUI(commandBuffer);
	vkCmdEndRenderPass(commandBuffer);
}
