/*
* Vulkan Example - Basic hardware accelerated ray tracing example using VK_KHR_ray_traying
*
* Copyright (C) 2019-2020 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#define VK_ENABLE_BETA_EXTENSIONS

#include "vulkanexamplebase.h"

// Holds data for a ray tracing scratch buffer that is used as a temporary storage
struct RayTracingScratchBuffer
{
	uint64_t deviceAddress = 0;
	VkBuffer buffer = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
};

// Holds data for a memory object bound to an acceleration structure
struct RayTracingObjectMemory
{
	uint64_t deviceAddress = 0;
	VkDeviceMemory memory = VK_NULL_HANDLE;
};

// Ray tracing acceleration structure
struct AccelerationStructure {
	VkAccelerationStructureKHR accelerationStructure;
	uint64_t handle;
	RayTracingObjectMemory objectMemory;
};

// Indices for the different ray tracing shader types used in this example
#define INDEX_RAYGEN_GROUP 0
#define INDEX_MISS_GROUP 1
#define INDEX_CLOSEST_HIT_GROUP 2

class VulkanExample : public VulkanExampleBase
{
public:
	PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR;
	PFN_vkBindAccelerationStructureMemoryKHR vkBindAccelerationStructureMemoryKHR;
	PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
	PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;
	PFN_vkGetAccelerationStructureMemoryRequirementsKHR vkGetAccelerationStructureMemoryRequirementsKHR;
	PFN_vkCmdBuildAccelerationStructureKHR vkCmdBuildAccelerationStructureKHR;
	PFN_vkBuildAccelerationStructureKHR vkBuildAccelerationStructureKHR;
	PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;
	PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;
	PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR;
	PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;

	VkPhysicalDeviceRayTracingPropertiesKHR rayTracingProperties{};
	VkPhysicalDeviceRayTracingFeaturesKHR rayTracingFeatures{};

	VkPhysicalDeviceBufferDeviceAddressFeatures enabledBufferDeviceAddresFeatures{};
	VkPhysicalDeviceRayTracingFeaturesKHR enabledRayTracingFeatures{};

	AccelerationStructure bottomLevelAS;
	AccelerationStructure topLevelAS;

	vks::Buffer vertexBuffer;
	vks::Buffer indexBuffer;
	uint32_t indexCount;
	std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups{};
	vks::Buffer shaderBindingTable;

	struct StorageImage {
		VkDeviceMemory memory;
		VkImage image;
		VkImageView view;
		VkFormat format;
	} storageImage;

	struct UniformData {
		glm::mat4 viewInverse;
		glm::mat4 projInverse;
	} uniformData;
	vks::Buffer ubo;

	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;
	VkDescriptorSet descriptorSet;
	VkDescriptorSetLayout descriptorSetLayout;

	VulkanExample() : VulkanExampleBase()
	{
		title = "Ray tracing basic";
		settings.overlay = true;
		camera.type = Camera::CameraType::lookat;
		camera.setPerspective(60.0f, (float)width / (float)height, 0.1f, 512.0f);
		camera.setRotation(glm::vec3(0.0f, 0.0f, 0.0f));
		camera.setTranslation(glm::vec3(0.0f, 0.0f, -2.5f));
		// Enable instance and device extensions required to use VK_KHR_ray_tracing
		enabledInstanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
		enabledDeviceExtensions.push_back(VK_KHR_MAINTENANCE3_EXTENSION_NAME);
		enabledDeviceExtensions.push_back(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
		enabledDeviceExtensions.push_back(VK_KHR_RAY_TRACING_EXTENSION_NAME);
		enabledDeviceExtensions.push_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
		enabledDeviceExtensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
		enabledDeviceExtensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
		enabledDeviceExtensions.push_back(VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME);
		// We require Vulkan 1.2 for ray tracing
		apiVersion = VK_API_VERSION_1_2;
	}

	~VulkanExample()
	{
		vkDestroyPipeline(device, pipeline, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
		vkDestroyImageView(device, storageImage.view, nullptr);
		vkDestroyImage(device, storageImage.image, nullptr);
		vkFreeMemory(device, storageImage.memory, nullptr);
		vkDestroyAccelerationStructureKHR(device, bottomLevelAS.accelerationStructure, nullptr);
		vkDestroyAccelerationStructureKHR(device, topLevelAS.accelerationStructure, nullptr);
		vertexBuffer.destroy();
		indexBuffer.destroy();
		shaderBindingTable.destroy();
		ubo.destroy();
		deleteObjectMemory(bottomLevelAS.objectMemory);
		deleteObjectMemory(topLevelAS.objectMemory);
	}

	/*	
		Create a scratch buffer to hold temporary data for a ray tracing acceleration structure
	*/
	RayTracingScratchBuffer createScratchBuffer(VkAccelerationStructureKHR accelerationStructure)
	{
		RayTracingScratchBuffer scratchBuffer{};

		VkMemoryRequirements2 memoryRequirements2{};
		memoryRequirements2.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;

		VkAccelerationStructureMemoryRequirementsInfoKHR accelerationStructureMemoryRequirements{};
		accelerationStructureMemoryRequirements.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_KHR;
		accelerationStructureMemoryRequirements.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_KHR;
		accelerationStructureMemoryRequirements.buildType = VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR;
		accelerationStructureMemoryRequirements.accelerationStructure = accelerationStructure;
		vkGetAccelerationStructureMemoryRequirementsKHR(device, &accelerationStructureMemoryRequirements, &memoryRequirements2);

		VkBufferCreateInfo bufferCI{};
		bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCI.size = memoryRequirements2.memoryRequirements.size;
		bufferCI.usage = VK_BUFFER_USAGE_RAY_TRACING_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
		bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		VK_CHECK_RESULT(vkCreateBuffer(device, &bufferCI, nullptr, &scratchBuffer.buffer));

		VkMemoryRequirements memoryRequirements{};
		vkGetBufferMemoryRequirements(device, scratchBuffer.buffer, &memoryRequirements);

		VkMemoryAllocateFlagsInfo memoryAllocateFI{};
		memoryAllocateFI.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
		memoryAllocateFI.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;

		VkMemoryAllocateInfo memoryAI{};
		memoryAI.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memoryAI.pNext = &memoryAllocateFI;
		memoryAI.allocationSize = memoryRequirements.size;
		memoryAI.memoryTypeIndex = vulkanDevice->getMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device, &memoryAI, nullptr, &scratchBuffer.memory));
		VK_CHECK_RESULT(vkBindBufferMemory(device, scratchBuffer.buffer, scratchBuffer.memory, 0));

		VkBufferDeviceAddressInfoKHR buffer_device_address_info{};
		buffer_device_address_info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		buffer_device_address_info.buffer = scratchBuffer.buffer;
		scratchBuffer.deviceAddress = vkGetBufferDeviceAddressKHR(device, &buffer_device_address_info);

		return scratchBuffer;
	}

	void deleteScratchBuffer(RayTracingScratchBuffer& scratchBuffer) 
	{
		if (scratchBuffer.memory != VK_NULL_HANDLE) {
			vkFreeMemory(device, scratchBuffer.memory, nullptr);
		}
		if (scratchBuffer.buffer != VK_NULL_HANDLE) {
			vkDestroyBuffer(device, scratchBuffer.buffer, nullptr);
		}
	}

	/*
		Allocate memory that will be attached to a ray tracing acceleration structure
	*/
	RayTracingObjectMemory createObjectMemory(VkAccelerationStructureKHR acceleration_structure)
	{
		RayTracingObjectMemory objectMemory{};

		VkMemoryRequirements2 memoryRequirements2{};
		memoryRequirements2.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;

		VkAccelerationStructureMemoryRequirementsInfoKHR accelerationStructureMemoryRequirements{};
		accelerationStructureMemoryRequirements.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_KHR;
		accelerationStructureMemoryRequirements.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_KHR;
		accelerationStructureMemoryRequirements.buildType = VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR;
		accelerationStructureMemoryRequirements.accelerationStructure = acceleration_structure;
		vkGetAccelerationStructureMemoryRequirementsKHR(device, &accelerationStructureMemoryRequirements, &memoryRequirements2);

		VkMemoryRequirements memoryRequirements = memoryRequirements2.memoryRequirements;

		VkMemoryAllocateInfo memoryAI{};
		memoryAI.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memoryAI.allocationSize = memoryRequirements.size;
		memoryAI.memoryTypeIndex = vulkanDevice->getMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device, &memoryAI, nullptr, &objectMemory.memory));

		return objectMemory;
	}

	void deleteObjectMemory(RayTracingObjectMemory& objectMemory)
	{
		if (objectMemory.memory != VK_NULL_HANDLE) {
			vkFreeMemory(device, objectMemory.memory, nullptr);
		}
	}

	/*
		Gets the device address from a buffer that's required for some of the buffers used for ray tracing
	*/
	uint64_t getBufferDeviceAddress(VkBuffer buffer)
	{
		VkBufferDeviceAddressInfoKHR bufferDeviceAI{};
		bufferDeviceAI.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		bufferDeviceAI.buffer = buffer;
		return vkGetBufferDeviceAddressKHR(device, &bufferDeviceAI);
	}

	/*
		Set up a storage image that the ray generation shader will be writing to
	*/
	void createStorageImage()
	{
		VkImageCreateInfo image = vks::initializers::imageCreateInfo();
		image.imageType = VK_IMAGE_TYPE_2D;
		image.format = swapChain.colorFormat;
		image.extent.width = width;
		image.extent.height = height;
		image.extent.depth = 1;
		image.mipLevels = 1;
		image.arrayLayers = 1;
		image.samples = VK_SAMPLE_COUNT_1_BIT;
		image.tiling = VK_IMAGE_TILING_OPTIMAL;
		image.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
		image.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		VK_CHECK_RESULT(vkCreateImage(device, &image, nullptr, &storageImage.image));

		VkMemoryRequirements memReqs;
		vkGetImageMemoryRequirements(device, storageImage.image, &memReqs);
		VkMemoryAllocateInfo memoryAllocateInfo = vks::initializers::memoryAllocateInfo();
		memoryAllocateInfo.allocationSize = memReqs.size;
		memoryAllocateInfo.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &storageImage.memory));
		VK_CHECK_RESULT(vkBindImageMemory(device, storageImage.image, storageImage.memory, 0));

		VkImageViewCreateInfo colorImageView = vks::initializers::imageViewCreateInfo();
		colorImageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
		colorImageView.format = swapChain.colorFormat;
		colorImageView.subresourceRange = {};
		colorImageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		colorImageView.subresourceRange.baseMipLevel = 0;
		colorImageView.subresourceRange.levelCount = 1;
		colorImageView.subresourceRange.baseArrayLayer = 0;
		colorImageView.subresourceRange.layerCount = 1;
		colorImageView.image = storageImage.image;
		VK_CHECK_RESULT(vkCreateImageView(device, &colorImageView, nullptr, &storageImage.view));

		VkCommandBuffer cmdBuffer = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
		vks::tools::setImageLayout(cmdBuffer, storageImage.image,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_GENERAL,
			{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
		vulkanDevice->flushCommandBuffer(cmdBuffer, queue);
	}

	/*
		Create the bottom level acceleration structure contains the scene's actual geometry (vertices, triangles)
	*/
	void createBottomLevelAccelerationStructure()
	{
		// Setup vertices for a single triangle
		struct Vertex {
			float pos[3];
		};
		std::vector<Vertex> vertices = {
			{ {  1.0f,  1.0f, 0.0f } },
			{ { -1.0f,  1.0f, 0.0f } },
			{ {  0.0f, -1.0f, 0.0f } }
		};

		// Setup indices
		std::vector<uint32_t> indices = { 0, 1, 2 };
		indexCount = static_cast<uint32_t>(indices.size());

		// Create buffers
		// For the sake of simplicity we won't stage the vertex data to the GPU memory
		// Vertex buffer
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&vertexBuffer,
			vertices.size() * sizeof(Vertex),
			vertices.data()));
		// Index buffer
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&indexBuffer,
			indices.size() * sizeof(uint32_t),
			indices.data()));

		VkDeviceOrHostAddressConstKHR vertexBufferDeviceAddress{};
		VkDeviceOrHostAddressConstKHR indexBufferDeviceAddress{};

		vertexBufferDeviceAddress.deviceAddress = getBufferDeviceAddress(vertexBuffer.buffer);
		indexBufferDeviceAddress.deviceAddress = getBufferDeviceAddress(indexBuffer.buffer);

		VkAccelerationStructureCreateGeometryTypeInfoKHR accelerationCreateGeometryInfo{};
		accelerationCreateGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_GEOMETRY_TYPE_INFO_KHR;
		accelerationCreateGeometryInfo.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
		accelerationCreateGeometryInfo.maxPrimitiveCount = 1;
		accelerationCreateGeometryInfo.indexType = VK_INDEX_TYPE_UINT32;
		accelerationCreateGeometryInfo.maxVertexCount = static_cast<uint32_t>(vertices.size());
		accelerationCreateGeometryInfo.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
		accelerationCreateGeometryInfo.allowsTransforms = VK_FALSE;

		VkAccelerationStructureCreateInfoKHR accelerationCI{};
		accelerationCI.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		accelerationCI.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		accelerationCI.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		accelerationCI.maxGeometryCount = 1;
		accelerationCI.pGeometryInfos = &accelerationCreateGeometryInfo;
		VK_CHECK_RESULT(vkCreateAccelerationStructureKHR(device, &accelerationCI, nullptr, &bottomLevelAS.accelerationStructure));

		// Bind object memory to the top level acceleration structure
		bottomLevelAS.objectMemory = createObjectMemory(bottomLevelAS.accelerationStructure);

		VkBindAccelerationStructureMemoryInfoKHR bindAccelerationMemoryInfo{};
		bindAccelerationMemoryInfo.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_KHR;
		bindAccelerationMemoryInfo.accelerationStructure = bottomLevelAS.accelerationStructure;
		bindAccelerationMemoryInfo.memory = bottomLevelAS.objectMemory.memory;
		VK_CHECK_RESULT(vkBindAccelerationStructureMemoryKHR(device, 1, &bindAccelerationMemoryInfo));

		VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
		accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
		accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
		accelerationStructureGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
		accelerationStructureGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
		accelerationStructureGeometry.geometry.triangles.vertexData.deviceAddress = vertexBufferDeviceAddress.deviceAddress;
		accelerationStructureGeometry.geometry.triangles.vertexStride = sizeof(Vertex);
		accelerationStructureGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
		accelerationStructureGeometry.geometry.triangles.indexData.deviceAddress = indexBufferDeviceAddress.deviceAddress;

		std::vector<VkAccelerationStructureGeometryKHR> acceleration_geometries = { accelerationStructureGeometry };
		VkAccelerationStructureGeometryKHR* acceleration_structure_geometries = acceleration_geometries.data();

		// Create a small scratch buffer used during build of the bottom level acceleration structure
		RayTracingScratchBuffer scratchBuffer = createScratchBuffer(bottomLevelAS.accelerationStructure);

		VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
		accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		accelerationBuildGeometryInfo.update = VK_FALSE;
		accelerationBuildGeometryInfo.dstAccelerationStructure = bottomLevelAS.accelerationStructure;
		accelerationBuildGeometryInfo.geometryArrayOfPointers = VK_FALSE;
		accelerationBuildGeometryInfo.geometryCount = 1;
		accelerationBuildGeometryInfo.ppGeometries = &acceleration_structure_geometries;
		accelerationBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer.deviceAddress;

		VkAccelerationStructureBuildOffsetInfoKHR accelerationBuildOffsetInfo{};
		accelerationBuildOffsetInfo.primitiveCount = 1;
		accelerationBuildOffsetInfo.primitiveOffset = 0x0;
		accelerationBuildOffsetInfo.firstVertex = 0;
		accelerationBuildOffsetInfo.transformOffset = 0x0;

		std::vector<VkAccelerationStructureBuildOffsetInfoKHR*> accelerationBuildOffsets = { &accelerationBuildOffsetInfo };

		if (rayTracingFeatures.rayTracingHostAccelerationStructureCommands)
		{
			// Implementation supports building acceleration structure building on host
			VK_CHECK_RESULT(vkBuildAccelerationStructureKHR(device, 1, &accelerationBuildGeometryInfo, accelerationBuildOffsets.data()));
		}
		else
		{
			// Acceleration structure needs to be build on the device
			VkCommandBuffer commandBuffer = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
			vkCmdBuildAccelerationStructureKHR(commandBuffer, 1, &accelerationBuildGeometryInfo, accelerationBuildOffsets.data());
			vulkanDevice->flushCommandBuffer(commandBuffer, queue);
		}

		VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
		accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
		accelerationDeviceAddressInfo.accelerationStructure = bottomLevelAS.accelerationStructure;

		bottomLevelAS.handle = vkGetAccelerationStructureDeviceAddressKHR(device, &accelerationDeviceAddressInfo);

		deleteScratchBuffer(scratchBuffer);
	}

	/*
		The top level acceleration structure contains the scene's object instances
	*/
	void createTopLevelAccelerationStructure()
	{
		VkAccelerationStructureCreateGeometryTypeInfoKHR accelerationCreateGeometryInfo{};
		accelerationCreateGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_GEOMETRY_TYPE_INFO_KHR;
		accelerationCreateGeometryInfo.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
		accelerationCreateGeometryInfo.maxPrimitiveCount = 1;
		accelerationCreateGeometryInfo.allowsTransforms = VK_FALSE;

		VkAccelerationStructureCreateInfoKHR accelerationCI{};
		accelerationCI.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		accelerationCI.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		accelerationCI.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		accelerationCI.maxGeometryCount = 1;
		accelerationCI.pGeometryInfos = &accelerationCreateGeometryInfo;
		VK_CHECK_RESULT(vkCreateAccelerationStructureKHR(device, &accelerationCI, nullptr, &topLevelAS.accelerationStructure));

		// Bind object memory to the top level acceleration structure
		topLevelAS.objectMemory = createObjectMemory(topLevelAS.accelerationStructure);

		VkBindAccelerationStructureMemoryInfoKHR bindAccelerationMemoryInfo{};
		bindAccelerationMemoryInfo.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_KHR;
		bindAccelerationMemoryInfo.accelerationStructure = topLevelAS.accelerationStructure;
		bindAccelerationMemoryInfo.memory = topLevelAS.objectMemory.memory;
		VK_CHECK_RESULT(vkBindAccelerationStructureMemoryKHR(device, 1, &bindAccelerationMemoryInfo));

		VkTransformMatrixKHR transform_matrix = {
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f };

		VkAccelerationStructureInstanceKHR instance{};
		instance.transform = transform_matrix;
		instance.instanceCustomIndex = 0;
		instance.mask = 0xFF;
		instance.instanceShaderBindingTableRecordOffset = 0;
		instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
		instance.accelerationStructureReference = bottomLevelAS.handle;

		// Buffer for instance data
		vks::Buffer instancesBuffer;
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&instancesBuffer,
			sizeof(instance),
			&instance));

		VkDeviceOrHostAddressConstKHR instance_data_device_address{};
		instance_data_device_address.deviceAddress = getBufferDeviceAddress(instancesBuffer.buffer);

		VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
		accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
		accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
		accelerationStructureGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
		accelerationStructureGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
		accelerationStructureGeometry.geometry.instances.data.deviceAddress = instance_data_device_address.deviceAddress;

		std::vector<VkAccelerationStructureGeometryKHR> acceleration_geometries = { accelerationStructureGeometry };
		VkAccelerationStructureGeometryKHR* acceleration_structure_geometries = acceleration_geometries.data();

		// Create a small scratch buffer used during build of the top level acceleration structure
		RayTracingScratchBuffer scratchBuffer = createScratchBuffer(topLevelAS.accelerationStructure);

		VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
		accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		accelerationBuildGeometryInfo.update = VK_FALSE;
		accelerationBuildGeometryInfo.srcAccelerationStructure = VK_NULL_HANDLE;
		accelerationBuildGeometryInfo.dstAccelerationStructure = topLevelAS.accelerationStructure;
		accelerationBuildGeometryInfo.geometryArrayOfPointers = VK_FALSE;
		accelerationBuildGeometryInfo.geometryCount = 1;
		accelerationBuildGeometryInfo.ppGeometries = &acceleration_structure_geometries;
		accelerationBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer.deviceAddress;

		VkAccelerationStructureBuildOffsetInfoKHR accelerationBuildOffsetInfo{};
		accelerationBuildOffsetInfo.primitiveCount = 1;
		accelerationBuildOffsetInfo.primitiveOffset = 0x0;
		accelerationBuildOffsetInfo.firstVertex = 0;
		accelerationBuildOffsetInfo.transformOffset = 0x0;
		std::vector<VkAccelerationStructureBuildOffsetInfoKHR*> accelerationBuildOffsets = { &accelerationBuildOffsetInfo };

		if (rayTracingFeatures.rayTracingHostAccelerationStructureCommands)
		{
			// Implementation supports building acceleration structure building on host
			VK_CHECK_RESULT(vkBuildAccelerationStructureKHR(device, 1, &accelerationBuildGeometryInfo, accelerationBuildOffsets.data()));
		}
		else
		{
			// Acceleration structure needs to be build on the device
			VkCommandBuffer commandBuffer = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
			vkCmdBuildAccelerationStructureKHR(commandBuffer, 1, &accelerationBuildGeometryInfo, accelerationBuildOffsets.data());
			vulkanDevice->flushCommandBuffer(commandBuffer, queue);
		}

		VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
		accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
		accelerationDeviceAddressInfo.accelerationStructure = topLevelAS.accelerationStructure;

		topLevelAS.handle = vkGetAccelerationStructureDeviceAddressKHR(device, &accelerationDeviceAddressInfo);

		deleteScratchBuffer(scratchBuffer);
		instancesBuffer.destroy();
	}

	/*
		Create the Shader Binding Table that binds the programs and top-level acceleration structure
	*/
	void createShaderBindingTable() {
		const uint32_t groupCount = static_cast<uint32_t>(shaderGroups.size());

		const uint32_t sbtSize = rayTracingProperties.shaderGroupBaseAlignment * groupCount;
		VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_RAY_TRACING_BIT_KHR, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &shaderBindingTable, sbtSize));
		shaderBindingTable.map();

		// Write the shader handles to the shader binding table
		std::vector<uint8_t> shaderHandleStorage(sbtSize);
		VK_CHECK_RESULT(vkGetRayTracingShaderGroupHandlesKHR(device, pipeline, 0, groupCount, sbtSize, shaderHandleStorage.data()));

		auto* data = static_cast<uint8_t*>(shaderBindingTable.mapped);
		// This part is required, as the alignment and handle size may differ
		for (uint32_t i = 0; i < groupCount; i++)
		{
			memcpy(data, shaderHandleStorage.data() + i * rayTracingProperties.shaderGroupHandleSize, rayTracingProperties.shaderGroupHandleSize);
			data += rayTracingProperties.shaderGroupBaseAlignment;
		}
		shaderBindingTable.unmap();
	}

	/*
		Create the descriptor sets used for the ray tracing dispatch
	*/
	void createDescriptorSets()
	{
		std::vector<VkDescriptorPoolSize> poolSizes = {
			{ VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 }
		};
		VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, 1);
		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, nullptr, &descriptorPool));

		VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, &descriptorSet));

		VkWriteDescriptorSetAccelerationStructureKHR descriptorAccelerationStructureInfo{};
		descriptorAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
		descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
		descriptorAccelerationStructureInfo.pAccelerationStructures = &topLevelAS.accelerationStructure;

		VkWriteDescriptorSet accelerationStructureWrite{};
		accelerationStructureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		// The specialized acceleration structure descriptor has to be chained
		accelerationStructureWrite.pNext = &descriptorAccelerationStructureInfo;
		accelerationStructureWrite.dstSet = descriptorSet;
		accelerationStructureWrite.dstBinding = 0;
		accelerationStructureWrite.descriptorCount = 1;
		accelerationStructureWrite.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;

		VkDescriptorImageInfo storageImageDescriptor{};
		storageImageDescriptor.imageView = storageImage.view;
		storageImageDescriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		VkWriteDescriptorSet resultImageWrite = vks::initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, &storageImageDescriptor);
		VkWriteDescriptorSet uniformBufferWrite = vks::initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, &ubo.descriptor);

		std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
			accelerationStructureWrite,
			resultImageWrite,
			uniformBufferWrite
		};
		vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, VK_NULL_HANDLE);
	}

	/*
		Create our ray tracing pipeline
	*/
	void createRayTracingPipeline()
	{
		VkDescriptorSetLayoutBinding accelerationStructureLayoutBinding{};
		accelerationStructureLayoutBinding.binding = 0;
		accelerationStructureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
		accelerationStructureLayoutBinding.descriptorCount = 1;
		accelerationStructureLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

		VkDescriptorSetLayoutBinding resultImageLayoutBinding{};
		resultImageLayoutBinding.binding = 1;
		resultImageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		resultImageLayoutBinding.descriptorCount = 1;
		resultImageLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

		VkDescriptorSetLayoutBinding uniformBufferBinding{};
		uniformBufferBinding.binding = 2;
		uniformBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformBufferBinding.descriptorCount = 1;
		uniformBufferBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

		std::vector<VkDescriptorSetLayoutBinding> bindings({
			accelerationStructureLayoutBinding,
			resultImageLayoutBinding,
			uniformBufferBinding
			});

		VkDescriptorSetLayoutCreateInfo descriptorSetlayoutCI{};
		descriptorSetlayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorSetlayoutCI.bindingCount = static_cast<uint32_t>(bindings.size());
		descriptorSetlayoutCI.pBindings = bindings.data();
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorSetlayoutCI, nullptr, &descriptorSetLayout));

		VkPipelineLayoutCreateInfo pipelineLayoutCI{};
		pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCI.setLayoutCount = 1;
		pipelineLayoutCI.pSetLayouts = &descriptorSetLayout;
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &pipelineLayout));

		const uint32_t shaderIndexRaygen = 0;
		const uint32_t shaderIndexMiss = 1;
		const uint32_t shaderIndexClosestHit = 2;

		std::array<VkPipelineShaderStageCreateInfo, 3> shaderStages;
		shaderStages[shaderIndexRaygen] = loadShader(getShadersPath() + "raytracingbasic/raygen.rgen.spv", VK_SHADER_STAGE_RAYGEN_BIT_KHR);
		shaderStages[shaderIndexMiss] = loadShader(getShadersPath() + "raytracingbasic/miss.rmiss.spv", VK_SHADER_STAGE_MISS_BIT_KHR);
		shaderStages[shaderIndexClosestHit] = loadShader(getShadersPath() + "raytracingbasic/closesthit.rchit.spv", VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);

		/*
			Setup ray tracing shader groups
		*/
		VkRayTracingShaderGroupCreateInfoKHR raygenGroupCI{};
		raygenGroupCI.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		raygenGroupCI.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		raygenGroupCI.generalShader = shaderIndexRaygen;
		raygenGroupCI.closestHitShader = VK_SHADER_UNUSED_KHR;
		raygenGroupCI.anyHitShader = VK_SHADER_UNUSED_KHR;
		raygenGroupCI.intersectionShader = VK_SHADER_UNUSED_KHR;
		shaderGroups.push_back(raygenGroupCI);

		VkRayTracingShaderGroupCreateInfoKHR missGroupCI{};
		missGroupCI.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		missGroupCI.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		missGroupCI.generalShader = shaderIndexMiss;
		missGroupCI.closestHitShader = VK_SHADER_UNUSED_KHR;
		missGroupCI.anyHitShader = VK_SHADER_UNUSED_KHR;
		missGroupCI.intersectionShader = VK_SHADER_UNUSED_KHR;
		shaderGroups.push_back(missGroupCI);

		VkRayTracingShaderGroupCreateInfoKHR closesHitGroupCI{};
		closesHitGroupCI.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		closesHitGroupCI.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
		closesHitGroupCI.generalShader = VK_SHADER_UNUSED_KHR;
		closesHitGroupCI.closestHitShader = shaderIndexClosestHit;
		closesHitGroupCI.anyHitShader = VK_SHADER_UNUSED_KHR;
		closesHitGroupCI.intersectionShader = VK_SHADER_UNUSED_KHR;
		shaderGroups.push_back(closesHitGroupCI);

		VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCI{};
		rayTracingPipelineCI.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
		rayTracingPipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
		rayTracingPipelineCI.pStages = shaderStages.data();
		rayTracingPipelineCI.groupCount = static_cast<uint32_t>(shaderGroups.size());
		rayTracingPipelineCI.pGroups = shaderGroups.data();
		rayTracingPipelineCI.maxRecursionDepth = 1;
		rayTracingPipelineCI.layout = pipelineLayout;
		rayTracingPipelineCI.libraries.sType = VK_STRUCTURE_TYPE_PIPELINE_LIBRARY_CREATE_INFO_KHR;
		VK_CHECK_RESULT(vkCreateRayTracingPipelinesKHR(device, VK_NULL_HANDLE, 1, &rayTracingPipelineCI, nullptr, &pipeline));
	}

	/*
		Create the uniform buffer used to pass matrices to the ray tracing ray generation shader
	*/
	void createUniformBuffer()
	{
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&ubo,
			sizeof(uniformData),
			&uniformData));
		VK_CHECK_RESULT(ubo.map());

		updateUniformBuffers();
	}

	/*
		Command buffer generation
	*/
	void buildCommandBuffers()
	{
		VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

		VkImageSubresourceRange subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

		for (int32_t i = 0; i < drawCmdBuffers.size(); ++i)
		{
			VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo));

			/*
				Dispatch the ray tracing commands
			*/
			vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline);
			vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipelineLayout, 0, 1, &descriptorSet, 0, 0);


			/*
				Setup the buffer regions pointing to the shaders in our shader binding table
			*/
			const VkDeviceSize sbtSize = rayTracingProperties.shaderGroupBaseAlignment * (VkDeviceSize)shaderGroups.size();

			VkStridedBufferRegionKHR raygenShaderSBTEntry{};
			raygenShaderSBTEntry.buffer = shaderBindingTable.buffer;
			raygenShaderSBTEntry.offset = static_cast<VkDeviceSize>(rayTracingProperties.shaderGroupBaseAlignment * INDEX_RAYGEN_GROUP);
			raygenShaderSBTEntry.stride = rayTracingProperties.shaderGroupBaseAlignment;
			raygenShaderSBTEntry.size = sbtSize;

			VkStridedBufferRegionKHR missShaderSBTEntry{};
			missShaderSBTEntry.buffer = shaderBindingTable.buffer;
			missShaderSBTEntry.offset = static_cast<VkDeviceSize>(rayTracingProperties.shaderGroupBaseAlignment * INDEX_MISS_GROUP);
			missShaderSBTEntry.stride = rayTracingProperties.shaderGroupBaseAlignment;
			missShaderSBTEntry.size = sbtSize;

			VkStridedBufferRegionKHR hitShaderSBTEntry{};
			hitShaderSBTEntry.buffer = shaderBindingTable.buffer;
			hitShaderSBTEntry.offset = static_cast<VkDeviceSize>(rayTracingProperties.shaderGroupBaseAlignment * INDEX_CLOSEST_HIT_GROUP);
			hitShaderSBTEntry.stride = rayTracingProperties.shaderGroupBaseAlignment;
			hitShaderSBTEntry.size = sbtSize;

			VkStridedBufferRegionKHR callableShaderSBTEntry{};

			/*
				Dispatch the ray tracing commands
			*/
			vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline);
			vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipelineLayout, 0, 1, &descriptorSet, 0, 0);

			vkCmdTraceRaysKHR(
				drawCmdBuffers[i],
				&raygenShaderSBTEntry,
				&missShaderSBTEntry,
				&hitShaderSBTEntry,
				&callableShaderSBTEntry,
				width,
				height,
				1);

			/*
				Copy ray tracing output to swap chain image
			*/

			// Prepare current swap chain image as transfer destination
			vks::tools::setImageLayout(
				drawCmdBuffers[i],
				swapChain.images[i],
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				subresourceRange);

			// Prepare ray tracing output image as transfer source
			vks::tools::setImageLayout(
				drawCmdBuffers[i],
				storageImage.image,
				VK_IMAGE_LAYOUT_GENERAL,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				subresourceRange);

			VkImageCopy copyRegion{};
			copyRegion.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
			copyRegion.srcOffset = { 0, 0, 0 };
			copyRegion.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
			copyRegion.dstOffset = { 0, 0, 0 };
			copyRegion.extent = { width, height, 1 };
			vkCmdCopyImage(drawCmdBuffers[i], storageImage.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, swapChain.images[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

			// Transition swap chain image back for presentation
			vks::tools::setImageLayout(
				drawCmdBuffers[i],
				swapChain.images[i],
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
				subresourceRange);

			// Transition ray tracing output image back to general layout
			vks::tools::setImageLayout(
				drawCmdBuffers[i],
				storageImage.image,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_IMAGE_LAYOUT_GENERAL,
				subresourceRange);

			VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
		}
	}

	void updateUniformBuffers()
	{
		uniformData.projInverse = glm::inverse(camera.matrices.perspective);
		uniformData.viewInverse = glm::inverse(camera.matrices.view);
		memcpy(ubo.mapped, &uniformData, sizeof(uniformData));
	}

	void getEnabledFeatures()
	{
		// Enable features required for ray tracing using feature chaining via pNext		
		enabledBufferDeviceAddresFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
		enabledBufferDeviceAddresFeatures.bufferDeviceAddress = VK_TRUE;
		
		enabledRayTracingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_FEATURES_KHR;
		enabledRayTracingFeatures.rayTracing = VK_TRUE;
		enabledRayTracingFeatures.pNext = &enabledBufferDeviceAddresFeatures;

		deviceCreatepNextChain = &enabledRayTracingFeatures;
	}

	void prepare()
	{
		VulkanExampleBase::prepare();

		// Query the ray tracing properties of the current implementation, we will need them later on
		rayTracingProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_KHR;
		VkPhysicalDeviceProperties2 deviceProps2{};
		deviceProps2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		deviceProps2.pNext = &rayTracingProperties;
		vkGetPhysicalDeviceProperties2(physicalDevice, &deviceProps2);

		// Query the ray tracing properties of the current implementation, we will need them later on
		rayTracingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_FEATURES_KHR;
		VkPhysicalDeviceFeatures2 deviceFeatures2{};
		deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		deviceFeatures2.pNext = &rayTracingFeatures;
		vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures2);

		// Get the function pointers required for ray tracing
		vkGetBufferDeviceAddressKHR = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(vkGetDeviceProcAddr(device, "vkGetBufferDeviceAddressKHR"));
		vkBindAccelerationStructureMemoryKHR = reinterpret_cast<PFN_vkBindAccelerationStructureMemoryKHR>(vkGetDeviceProcAddr(device, "vkBindAccelerationStructureMemoryKHR"));
		vkCreateAccelerationStructureKHR = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureKHR"));
		vkDestroyAccelerationStructureKHR = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(vkGetDeviceProcAddr(device, "vkDestroyAccelerationStructureKHR"));
		vkGetAccelerationStructureMemoryRequirementsKHR = reinterpret_cast<PFN_vkGetAccelerationStructureMemoryRequirementsKHR>(vkGetDeviceProcAddr(device, "vkGetAccelerationStructureMemoryRequirementsKHR"));
		vkCmdBuildAccelerationStructureKHR = reinterpret_cast<PFN_vkCmdBuildAccelerationStructureKHR>(vkGetDeviceProcAddr(device, "vkCmdBuildAccelerationStructureKHR"));
		vkBuildAccelerationStructureKHR = reinterpret_cast<PFN_vkBuildAccelerationStructureKHR>(vkGetDeviceProcAddr(device, "vkBuildAccelerationStructureKHR"));
		vkGetAccelerationStructureDeviceAddressKHR = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(device, "vkGetAccelerationStructureDeviceAddressKHR"));
		vkCmdTraceRaysKHR = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(device, "vkCmdTraceRaysKHR"));
		vkGetRayTracingShaderGroupHandlesKHR = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(vkGetDeviceProcAddr(device, "vkGetRayTracingShaderGroupHandlesKHR"));
		vkCreateRayTracingPipelinesKHR = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesKHR"));

		// Create the acceleration structures used to render the ray traced scene
		createBottomLevelAccelerationStructure();
		createTopLevelAccelerationStructure();

		createStorageImage();
		createUniformBuffer();
		createRayTracingPipeline();
		createShaderBindingTable();
		createDescriptorSets();
		buildCommandBuffers();
		prepared = true;
	}

	void draw()
	{
		VulkanExampleBase::prepareFrame();
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
		VulkanExampleBase::submitFrame();
	}

	virtual void render()
	{
		if (!prepared)
			return;
		draw();
		if (camera.updated)
			updateUniformBuffers();
	}
};

VULKAN_EXAMPLE_MAIN()
