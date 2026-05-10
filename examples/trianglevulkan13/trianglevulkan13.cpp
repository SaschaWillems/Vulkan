/*
* Vulkan Example - Basic indexed triangle rendering using Vulkan 1.3
*
* Note:
* This is a variation of the the triangle sample that makes use of Vulkan 1.3 features
* This simplifies the api a bit, esp. with dynamic rendering replacing render passes (and with that framebuffers)
*
* Copyright (C) 2024-2026 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fstream>
#include <vector>
#include <exception>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vulkan/vulkan.h>
#include "vulkanexamplebase.h"

// We want to keep GPU and CPU busy. To do that we may start building a new command buffer while the previous one is still being executed
// This number defines how many frames may be worked on simultaneously at once
// Increasing this number may improve performance but will also introduce additional latency
constexpr auto MAX_CONCURRENT_FRAMES = 2;

class VulkanExample : public VulkanExampleBase
{
public:
	// Vertex layout used in this example
	struct Vertex {
		float position[3];
		float color[3];
	};

	struct VulkanBuffer {
		VkDeviceMemory memory{ VK_NULL_HANDLE };
		VkBuffer handle{ VK_NULL_HANDLE };
	};

	VulkanBuffer vertexBuffer;
	VulkanBuffer indexBuffer;
	uint32_t indexCount{ 0 };

	// Uniform buffer block object
	struct UniformBuffer : VulkanBuffer {
		// The descriptor set stores the resources bound to the binding points in a shader
		// It connects the binding points of the different shaders with the buffers and images used for those bindings
		VkDescriptorSet descriptorSet{ VK_NULL_HANDLE };
		// We keep a pointer to the mapped buffer, so we can easily update it's contents via a memcpy
		uint8_t* mapped{ nullptr };
	};
	// We use one UBO per frame, so we can have a frame overlap and make sure that uniforms aren't updated while still in use
	std::array<UniformBuffer, MAX_CONCURRENT_FRAMES> uniformBuffers;

	// For simplicity we use the same uniform block layout as in the shader
	// This way we can just memcpy the data to the ubo
	// Note: You should use data types that align with the GPU in order to avoid manual padding (vec4, mat4)
	struct ShaderData {
		glm::mat4 projectionMatrix;
		glm::mat4 modelMatrix;
		glm::mat4 viewMatrix;
	};

	// The pipeline layout is used by a pipeline to access the descriptor sets
	// It defines interface (without binding any actual data) between the shader stages used by the pipeline and the shader resources
	// A pipeline layout can be shared among multiple pipelines as long as their interfaces match
	VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };

	// Pipelines (often called "pipeline state objects") are used to bake all states that affect a pipeline
	// While in OpenGL every state can be changed at (almost) any time, Vulkan requires to layout the graphics (and compute) pipeline states upfront
	// So for each combination of non-dynamic pipeline states you need a new pipeline (there are a few exceptions to this not discussed here)
	// Even though this adds a new dimension of planning ahead, it's a great opportunity for performance optimizations by the driver
	VkPipeline pipeline{ VK_NULL_HANDLE };

	// The descriptor set layout describes the shader binding layout (without actually referencing descriptor)
	// Like the pipeline layout it's pretty much a blueprint and can be used with different descriptor sets as long as their layout matches
	VkDescriptorSetLayout descriptorSetLayout{ VK_NULL_HANDLE };

	// Synchronization primitives
	// Synchronization is an important concept of Vulkan that OpenGL mostly hid away. Getting this right is crucial to using Vulkan.
	// Semaphores are used to coordinate operations within the graphics queue and ensure correct command ordering
	std::vector<VkSemaphore> presentCompleteSemaphores{};
	std::vector<VkSemaphore> renderCompleteSemaphores{};
	// Fences are used to make sure command buffers aren't rerecorded until they've finished executing
	std::array<VkFence, MAX_CONCURRENT_FRAMES> waitFences{};

	VkCommandPool commandPool{ VK_NULL_HANDLE };
	std::array<VkCommandBuffer, MAX_CONCURRENT_FRAMES> commandBuffers{};

	// To select the correct sync and command objects, we need to keep track of the current frame
	uint32_t currentFrame{ 0 };

	VkPhysicalDeviceVulkan13Features enabledFeatures{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };

	VulkanExample() : VulkanExampleBase()
	{
		title = "Basic indexed triangle using Vulkan 1.3";
		// To keep things simple, we don't use the UI overlay from the framework
		settings.overlay = false;
		// Setup a default look-at camera
		camera.type = Camera::CameraType::lookat;
		camera.setPosition(glm::vec3(0.0f, 0.0f, -2.5f));
		camera.setRotation(glm::vec3(0.0f));
		camera.setPerspective(60.0f, (float)width / (float)height, 1.0f, 256.0f);
		// We want to use Vulkan 1.3 with the dynamic rendering and sync 2 features
		apiVersion = VK_API_VERSION_1_3;
		enabledFeatures.dynamicRendering = VK_TRUE;
		enabledFeatures.synchronization2 = VK_TRUE;
		deviceCreatepNextChain = &enabledFeatures;
	}

	~VulkanExample() override
	{
		// Clean up used Vulkan resources
		// Note: Inherited destructor cleans up resources stored in base class
		if (device) {
			vkDestroyPipeline(device, pipeline, nullptr);
			vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
			vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
			vkDestroyBuffer(device, vertexBuffer.handle, nullptr);
			vkFreeMemory(device, vertexBuffer.memory, nullptr);
			vkDestroyBuffer(device, indexBuffer.handle, nullptr);
			vkFreeMemory(device, indexBuffer.memory, nullptr);
			vkDestroyCommandPool(device, commandPool, nullptr);
			for (size_t i = 0; i < presentCompleteSemaphores.size(); i++) {
				vkDestroySemaphore(device, presentCompleteSemaphores[i], nullptr);
			}
			for (size_t i = 0; i < renderCompleteSemaphores.size(); i++) {
				vkDestroySemaphore(device, renderCompleteSemaphores[i], nullptr);
			}
			for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; i++) {
				vkDestroyFence(device, waitFences[i], nullptr);
				vkDestroyBuffer(device, uniformBuffers[i].handle, nullptr);
				vkFreeMemory(device, uniformBuffers[i].memory, nullptr);
			}
		}
	}

	virtual void getEnabledFeatures() override
	{
		// Vulkan 1.3 device support is required for this example
		if (deviceProperties.apiVersion < VK_API_VERSION_1_3) {
			vks::tools::exitFatal("Selected GPU does not support support Vulkan 1.3", VK_ERROR_INCOMPATIBLE_DRIVER);
		}
	}

	// This function is used to request a device memory type that supports all the property flags we request (e.g. device local, host visible)
	// Upon success it will return the index of the memory type that fits our requested memory properties
	// This is necessary as implementations can offer an arbitrary number of memory types with different memory properties
	// You can check https://vulkan.gpuinfo.org/ for details on different memory configurations
	uint32_t getMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties)
	{
		// Iterate over all memory types available for the device used in this example
		for (uint32_t i = 0; i < deviceMemoryProperties.memoryTypeCount; i++) {
			if ((typeBits & 1) == 1) {
				if ((deviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
					return i;
				}
			}
			typeBits >>= 1;
		}
		throw "Could not find a suitable memory type!";
	}

	// Create the per-frame (in flight) and per (swapchain image) Vulkan synchronization primitives used in this example
	void createSynchronizationPrimitives()
	{
		// Fences are per frame in flight
		for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; i++) {		
			// Fence used to ensure that command buffer has completed exection before using it again
			VkFenceCreateInfo fenceCI{
				.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
				// Create the fences in signaled state (so we don't wait on first render of each command buffer)
				.flags = VK_FENCE_CREATE_SIGNALED_BIT,
			};
			VK_CHECK_RESULT(vkCreateFence(device, &fenceCI, nullptr, &waitFences[i]));
		}
		// Semaphores are used for correct command ordering within a queue
		VkSemaphoreCreateInfo semaphoreCI{ .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, };
		// Used to ensure that image presentation is complete before starting to submit again
		presentCompleteSemaphores.resize(MAX_CONCURRENT_FRAMES);
		for (auto& semaphore : presentCompleteSemaphores) {
			VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCI, nullptr, &semaphore));
		}
		// Render completion
		// Semaphore used to ensure that all commands submitted have been finished before submitting the image to the queue
		renderCompleteSemaphores.resize(swapChain.images.size());
		for (auto& semaphore : renderCompleteSemaphores) {
			VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCI, nullptr, &semaphore));
		}
	}

	// Command buffers are used to record commands to and are submitted to a queue for execution ("rendering")
	void createCommandBuffers()
	{
		// All command buffers are allocated from the same command pool
		VkCommandPoolCreateInfo commandPoolCI{
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.pNext = nullptr,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex = swapChain.queueNodeIndex,
		};
		VK_CHECK_RESULT(vkCreateCommandPool(device, &commandPoolCI, nullptr, &commandPool));
		// Allocate one command buffer per max. concurrent frame from above pool
		VkCommandBufferAllocateInfo cmdBufAllocateInfo = vks::initializers::commandBufferAllocateInfo(commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, MAX_CONCURRENT_FRAMES);
		VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, commandBuffers.data()));
	}

	// Prepare vertex and index buffers for an indexed triangle
	// Also uploads them to device local memory using staging and initializes vertex input and attribute binding to match the vertex shader
	void createVertexBuffer()
	{
		// A note on memory management in Vulkan in general:
		//	This is a complex topic and while it's fine for an example application to small individual memory allocations that is not
		//	what should be done a real-world application, where you should allocate large chunks of memory at once instead.

		// Setup vertices
		const std::vector<Vertex> vertices{
			{ {  1.0f,  1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
			{ { -1.0f,  1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
			{ {  0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } }
		};
		uint32_t vertexBufferSize = static_cast<uint32_t>(vertices.size()) * sizeof(Vertex);

		// Setup indices
		// We do this for demonstration purposes, a triangle doesn't require indices to be rendered (because of the 1:1 mapping), but more complex shapes usually make use of indices
		std::vector<uint32_t> indices{ 0, 1, 2 };
		indexCount = static_cast<uint32_t>(indices.size());
		uint32_t indexBufferSize = indexCount * sizeof(uint32_t);


		// Static data like vertex and index buffer should be stored on the device memory for optimal (and fastest) access by the GPU
		//
		// To achieve this we use so-called "staging buffers" :
		// - Create a buffer that's visible to the host (and can be mapped)
		// - Copy the data to this buffer
		// - Create another buffer that's local on the device (VRAM) with the same size
		// - Copy the data from the host to the device using a command buffer
		// - Delete the host visible (staging) buffer
		// - Use the device local buffers for rendering
		//
		// Note: On unified memory architectures where host (CPU) and GPU share the same memory, staging is not necessary
		// To keep this sample easy to follow, there is no check for that in place

		// Create the host visible staging buffer that we copy vertices and indices too, and from which we copy to the device
		VulkanBuffer stagingBuffer;
		VkBufferCreateInfo stagingBufferCI{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = vertexBufferSize + indexBufferSize,
			// Buffer is used as the copy source
			.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT
		};
		// Create a host-visible buffer to copy the vertex data to (staging buffer)
		VK_CHECK_RESULT(vkCreateBuffer(device, &stagingBufferCI, nullptr, &stagingBuffer.handle));
		VkMemoryRequirements memReqs;
		vkGetBufferMemoryRequirements(device, stagingBuffer.handle, &memReqs);
		VkMemoryAllocateInfo memAlloc{
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = memReqs.size,
			// Request a host visible memory type that can be used to copy our data to
			// Also request it to be coherent, so that writes are visible to the GPU right after unmapping the buffer
			.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
		};
		VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &stagingBuffer.memory));
		VK_CHECK_RESULT(vkBindBufferMemory(device, stagingBuffer.handle, stagingBuffer.memory, 0));
		// Map the buffer and copy vertices and indices into it, this way we can use a single buffer as the source for both vertex and index GPU buffers
		uint8_t* data{ nullptr };
		VK_CHECK_RESULT(vkMapMemory(device, stagingBuffer.memory, 0, memAlloc.allocationSize, 0, (void**)&data));
		memcpy(data, vertices.data(), vertexBufferSize);
		memcpy(((char*)data) + vertexBufferSize, indices.data(), indexBufferSize);

		// Create a device local buffer to which the (host local) vertex data will be copied and which will be used for rendering
		VkBufferCreateInfo vertexbufferCI{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = vertexBufferSize,
			.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
		};
		VK_CHECK_RESULT(vkCreateBuffer(device, &vertexbufferCI, nullptr, &vertexBuffer.handle));
		vkGetBufferMemoryRequirements(device, vertexBuffer.handle, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		memAlloc.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &vertexBuffer.memory));
		VK_CHECK_RESULT(vkBindBufferMemory(device, vertexBuffer.handle, vertexBuffer.memory, 0));

		// Create a device local buffer to which the (host local) index data will be copied and which will be used for rendering
		VkBufferCreateInfo indexbufferCI{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = indexBufferSize,
			.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
		};
		VK_CHECK_RESULT(vkCreateBuffer(device, &indexbufferCI, nullptr, &indexBuffer.handle));
		vkGetBufferMemoryRequirements(device, indexBuffer.handle, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		memAlloc.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &indexBuffer.memory));
		VK_CHECK_RESULT(vkBindBufferMemory(device, indexBuffer.handle, indexBuffer.memory, 0));

		// Buffer copies have to be submitted to a queue, so we need a command buffer for them
		VkCommandBuffer copyCmd;

		VkCommandBufferAllocateInfo cmdBufAllocateInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = commandPool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1
		};
		VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &copyCmd));

		VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();
		VK_CHECK_RESULT(vkBeginCommandBuffer(copyCmd, &cmdBufInfo));
		// Copy vertex and index buffers to the device
		VkBufferCopy copyRegion{};
		copyRegion.size = vertexBufferSize;
		vkCmdCopyBuffer(copyCmd, stagingBuffer.handle, vertexBuffer.handle, 1, &copyRegion);
		copyRegion.size = indexBufferSize;
		// Indices are stored after the vertices in the source buffer, so we need to add an offset
		copyRegion.srcOffset = vertexBufferSize;
		vkCmdCopyBuffer(copyCmd, stagingBuffer.handle, indexBuffer.handle,	1, &copyRegion);
		VK_CHECK_RESULT(vkEndCommandBuffer(copyCmd));

		// Submit the command buffer to the queue to finish the copy
		VkSubmitInfo submitInfo{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.commandBufferCount = 1,
			.pCommandBuffers = &copyCmd
		};

		// Create fence to ensure that the command buffer has finished executing
		VkFenceCreateInfo fenceCI{ .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
		VkFence fence;
		VK_CHECK_RESULT(vkCreateFence(device, &fenceCI, nullptr, &fence));
		// Submit copies to the queue
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));
		// Wait for the fence to signal that command buffer has finished executing
		VK_CHECK_RESULT(vkWaitForFences(device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));
		vkDestroyFence(device, fence, nullptr);
		vkFreeCommandBuffers(device, commandPool, 1, &copyCmd);

		// The fence made sure copies are finished, so we can safely delete the staging buffer
		vkDestroyBuffer(device, stagingBuffer.handle, nullptr);
		vkFreeMemory(device, stagingBuffer.memory, nullptr);
	}

	// Decriptors are used to pass data to shaders, for our sample we use a descriptor to pass parameters like matrices to the shader
	void createDescriptors()
	{
		// Descriptors are allocated from a pool, that tells the implementation how many and what types of descriptors we are going to use (at maximum)
		VkDescriptorPoolSize descriptorTypeCounts[1]{};
		// This example only one descriptor type (uniform buffer)
		descriptorTypeCounts[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		// We have one buffer (and as such descriptor) per frame
		descriptorTypeCounts[0].descriptorCount = MAX_CONCURRENT_FRAMES;
		// For additional types you need to add new entries in the type count list
		// E.g. for two combined image samplers :
		// typeCounts[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		// typeCounts[1].descriptorCount = 2;

		// Create the global descriptor pool
		// All descriptors used in this example are allocated from this pool
		VkDescriptorPoolCreateInfo descriptorPoolCI{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			// Set the max. number of descriptor sets that can be requested from this pool (requesting beyond this limit will result in an error)
			// Our sample will create one set per uniform buffer per frame
			.maxSets = MAX_CONCURRENT_FRAMES,
			.poolSizeCount = 1,
			.pPoolSizes = descriptorTypeCounts,
		};
		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolCI, nullptr, &descriptorPool));

		// Descriptor set layouts define the interface between our application and the shader
		// Basically connects the different shader stages to descriptors for binding uniform buffers, image samplers, etc.
		// So every shader binding should map to one descriptor set layout binding
		// Binding 0: Uniform buffer (Vertex shader)
		VkDescriptorSetLayoutBinding layoutBinding{
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT
		};

		VkDescriptorSetLayoutCreateInfo descriptorLayoutCI{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = 1,
			.pBindings = &layoutBinding
		};
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayoutCI, nullptr, &descriptorSetLayout));

		// Where the descriptor set layout is the interface, the descriptor set points to actual data
		// Descriptors that are changed per frame need to be multiplied, so we can update descriptor n+1 while n is still used by the GPU, so we create one per max frame in flight
		for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; i++) {
			VkDescriptorSetAllocateInfo allocInfo{
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
				.descriptorPool = descriptorPool,
				.descriptorSetCount = 1,
				.pSetLayouts = &descriptorSetLayout
			};
			VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &uniformBuffers[i].descriptorSet));

			// The buffer's information is passed using a descriptor info structure
			VkDescriptorBufferInfo bufferInfo{
				.buffer = uniformBuffers[i].handle,
				.range = sizeof(ShaderData),
			};

			// Update the descriptor set determining the shader binding points
			// For every binding point used in a shader there needs to be one
			// descriptor set matching that binding point
			// Binding 0 : Uniform buffer
			VkWriteDescriptorSet writeDescriptorSet{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = uniformBuffers[i].descriptorSet,
				.dstBinding = 0,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.pBufferInfo = &bufferInfo
			};
			vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
		}
	}

	// Create the depth (and stencil) buffer attachments
	// While we don't do any depth testing in this sample, having depth testing is very common so it's a good idea to learn it from the very start
	void setupDepthStencil() override
	{
		// Create an optimal tiled image used as the depth stencil attachment
		VkImageCreateInfo imageCI{
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = depthFormat,
			.extent = { width, height, 1 },
			.mipLevels = 1,
			.arrayLayers = 1,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = VK_IMAGE_TILING_OPTIMAL,
			.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
		};
		VK_CHECK_RESULT(vkCreateImage(device, &imageCI, nullptr, &depthStencil.image));

		// Allocate memory for the image (device local) and bind it to our image
		VkMemoryRequirements memReqs;
		vkGetImageMemoryRequirements(device, depthStencil.image, &memReqs);
		VkMemoryAllocateInfo memAlloc{
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = memReqs.size,
			.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
		};
		VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &depthStencil.memory));
		VK_CHECK_RESULT(vkBindImageMemory(device, depthStencil.image, depthStencil.memory, 0));

		// Create a view for the depth stencil image
		// Images aren't directly accessed in Vulkan, but rather through views described by a subresource range
		// This allows for multiple views of one image with differing ranges (e.g. for different layers)
		VkImageViewCreateInfo depthStencilViewCI{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = depthStencil.image,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = depthFormat,
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
		};
		// Stencil aspect should only be set on depth + stencil formats (VK_FORMAT_D16_UNORM_S8_UINT..VK_FORMAT_D32_SFLOAT_S8_UINT)
		if (depthFormat >= VK_FORMAT_D16_UNORM_S8_UINT) {
			depthStencilViewCI.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		VK_CHECK_RESULT(vkCreateImageView(device, &depthStencilViewCI, nullptr, &depthStencil.view));
	}

	// Vulkan loads its shaders from an immediate binary representation called SPIR-V
	// Shaders are compiled offline from e.g. GLSL using the reference glslang compiler
	// This function loads such a shader from a binary file and returns a shader module structure
	VkShaderModule loadSPIRVShader(const std::string& filename)
	{
		size_t shaderSize;
		char* shaderCode{ nullptr };

#if defined(__ANDROID__)
		// Load shader from compressed asset
		AAsset* asset = AAssetManager_open(androidApp->activity->assetManager, filename.c_str(), AASSET_MODE_STREAMING);
		assert(asset);
		shaderSize = AAsset_getLength(asset);
		assert(shaderSize > 0);

		shaderCode = new char[shaderSize];
		AAsset_read(asset, shaderCode, shaderSize);
		AAsset_close(asset);
#else
		std::ifstream is(filename, std::ios::binary | std::ios::in | std::ios::ate);

		if (is.is_open()) {
			shaderSize = is.tellg();
			is.seekg(0, std::ios::beg);
			// Copy file contents into a buffer
			shaderCode = new char[shaderSize];
			is.read(shaderCode, shaderSize);
			is.close();
			assert(shaderSize > 0);
		}
#endif
		if (shaderCode) {
			// Create a new shader module that will be used for pipeline creation
			VkShaderModuleCreateInfo shaderModuleCI{
				.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
				.codeSize = shaderSize,
				.pCode = (uint32_t*)shaderCode,
			};
			VkShaderModule shaderModule;
			VK_CHECK_RESULT(vkCreateShaderModule(device, &shaderModuleCI, nullptr, &shaderModule));
			delete[] shaderCode;
			return shaderModule;
		} else {
			std::cerr << "Error: Could not open shader file \"" << filename << "\"" << std::endl;
			return VK_NULL_HANDLE;
		}
	}

	void createPipeline()
	{
		// The pipeline layout is the interface telling the pipeline what type of descriptors will later be bound
		VkPipelineLayoutCreateInfo pipelineLayoutCI{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = 1,
			.pSetLayouts = &descriptorSetLayout,
		};
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &pipelineLayout));

		// Create the graphics pipeline used in this example
		// Vulkan uses the concept of rendering pipelines to encapsulate fixed states, replacing OpenGL's complex state machine
		// A pipeline is then stored and hashed on the GPU making pipeline changes very fast

		// Construct the different states making up the pipeline

		// Input assembly state describes how primitives are assembled
		// This pipeline will assemble vertex data as a triangle lists (though we only use one triangle)
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		};

		// Rasterization state
		VkPipelineRasterizationStateCreateInfo rasterizationStateCI{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.depthClampEnable = VK_FALSE,
			.rasterizerDiscardEnable = VK_FALSE,
			.polygonMode = VK_POLYGON_MODE_FILL,
			.cullMode = VK_CULL_MODE_NONE,
			.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
			.depthBiasEnable = VK_FALSE,
			.lineWidth = 1.0f,
		};

		// Color blend state describes how blend factors are calculated (if used)
		// We need one blend attachment state per color attachment (even if blending is not used)
		VkPipelineColorBlendAttachmentState blendAttachmentState{};
		blendAttachmentState.colorWriteMask = 0xf;
		blendAttachmentState.blendEnable = VK_FALSE;
		VkPipelineColorBlendStateCreateInfo colorBlendStateCI{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			.attachmentCount = 1,
			.pAttachments = &blendAttachmentState,
		};

		// Viewport state sets the number of viewports and scissor used in this pipeline
		// Note: This is actually overridden by the dynamic states (see below)
		VkPipelineViewportStateCreateInfo viewportStateCI{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.viewportCount = 1,
			.scissorCount = 1,
		};

		// Enable dynamic states
		// Most states are baked into the pipeline, but there is somee state that can be dynamically changed within the command buffer to mak e things easuer
		// To be able to change these we need do specify which dynamic states will be changed using this pipeline. Their actual states are set later on in the command buffer
		std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicStateCI{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
			.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size()),
			.pDynamicStates = dynamicStateEnables.data(),
		};

		// Depth and stencil state containing depth and stencil compare and test operations
		// We only use depth tests and want depth tests and writes to be enabled and compare with less or equal
		VkPipelineDepthStencilStateCreateInfo depthStencilStateCI{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			.depthTestEnable = VK_TRUE,
			.depthWriteEnable = VK_TRUE,
			.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
			.depthBoundsTestEnable = VK_FALSE,
			.stencilTestEnable = VK_FALSE,
			.front = { .failOp = VK_STENCIL_OP_KEEP, .passOp = VK_STENCIL_OP_KEEP, .compareOp = VK_COMPARE_OP_ALWAYS},
			.back = { .failOp = VK_STENCIL_OP_KEEP, .passOp = VK_STENCIL_OP_KEEP, .compareOp = VK_COMPARE_OP_ALWAYS },
		};

		// This example does not make use of multi sampling (for anti-aliasing), the state must still be set and passed to the pipeline
		VkPipelineMultisampleStateCreateInfo multisampleStateCI{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		};

		// Vertex input descriptions
		// Specifies the vertex input parameters for a pipeline

		// Vertex input binding
		// This example uses a single vertex input binding at binding point 0 (see vkCmdBindVertexBuffers)
		VkVertexInputBindingDescription vertexInputBinding{
			.binding = 0,
			.stride = sizeof(Vertex),
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
		};

		// Input attribute bindings describe shader attribute locations and memory layouts
		std::array<VkVertexInputAttributeDescription, 2> vertexInputAttributs{};
		// These match the following shader layout (see triangle.vert):
		//	layout (location = 0) in vec3 inPos;
		//	layout (location = 1) in vec3 inColor;
		// Attribute location 0: Position
		vertexInputAttributs[0].binding = 0;
		vertexInputAttributs[0].location = 0;
		// Position attribute is three 32 bit signed (SFLOAT) floats (R32 G32 B32)
		vertexInputAttributs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		vertexInputAttributs[0].offset = offsetof(Vertex, position);
		// Attribute location 1: Color
		vertexInputAttributs[1].binding = 0;
		vertexInputAttributs[1].location = 1;
		// Color attribute is three 32 bit signed (SFLOAT) floats (R32 G32 B32)
		vertexInputAttributs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		vertexInputAttributs[1].offset = offsetof(Vertex, color);

		// Vertex input state used for pipeline creation
		VkPipelineVertexInputStateCreateInfo vertexInputStateCI{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.vertexBindingDescriptionCount = 1,
			.pVertexBindingDescriptions = &vertexInputBinding,
			.vertexAttributeDescriptionCount = 2,
			.pVertexAttributeDescriptions = vertexInputAttributs.data()
		};

		// Shaders
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};

		// Vertex shader
		shaderStages[0] = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_VERTEX_BIT,
			.module = loadSPIRVShader(getShadersPath() + "triangle/triangle.vert.spv"),
			.pName = "main"
		};
		assert(shaderStages[0].module != VK_NULL_HANDLE);

		// Fragment shader
		shaderStages[1] = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = loadSPIRVShader(getShadersPath() + "triangle/triangle.frag.spv"),
			.pName = "main"
		};
		assert(shaderStages[1].module != VK_NULL_HANDLE);

		// Attachment information for dynamic rendering
		VkPipelineRenderingCreateInfoKHR pipelineRenderingCI{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
			.colorAttachmentCount = 1,
			.pColorAttachmentFormats = &swapChain.colorFormat,
			.depthAttachmentFormat = depthFormat,
			.stencilAttachmentFormat = depthFormat,
		};

		// Assign the pipeline states to the pipeline creation info structure
		VkGraphicsPipelineCreateInfo pipelineCI{
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			// Dynamic rendering settings are chained
			.pNext = &pipelineRenderingCI,
			// Pass shaders used for this pipeline
			.stageCount = static_cast<uint32_t>(shaderStages.size()),
			.pStages = shaderStages.data(),
			.pVertexInputState = &vertexInputStateCI,
			.pInputAssemblyState = &inputAssemblyStateCI,
			.pViewportState = &viewportStateCI,
			.pRasterizationState = &rasterizationStateCI,
			.pMultisampleState = &multisampleStateCI,
			.pDepthStencilState = &depthStencilStateCI,
			.pColorBlendState = &colorBlendStateCI,
			.pDynamicState = &dynamicStateCI,
			// The layout used for this pipeline (can be shared among multiple pipelines using the same layout)
			.layout = pipelineLayout,
		};

		// Create rendering pipeline using the specified states
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipeline));

		// Shader modules can safely be destroyed when the pipeline has been created
		vkDestroyShaderModule(device, shaderStages[0].module, nullptr);
		vkDestroyShaderModule(device, shaderStages[1].module, nullptr);
	}

	void createUniformBuffers()
	{
		// Prepare and initialize the per-frame uniform buffer blocks containing shader uniforms
		// Single uniforms like in OpenGL are no longer present in Vulkan. All shader uniforms are passed via uniform buffer blocks
		VkBufferCreateInfo bufferInfo{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = sizeof(ShaderData),
			// This buffer will be used as a uniform buffer
			.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
		};

		// Create the buffers
		for (uint32_t i = 0; i < MAX_CONCURRENT_FRAMES; i++) {
			VK_CHECK_RESULT(vkCreateBuffer(device, &bufferInfo, nullptr, &uniformBuffers[i].handle));
			// Get memory requirements including size, alignment and memory type based on the buffer type we request (uniform buffer)
			VkMemoryRequirements memReqs;
			vkGetBufferMemoryRequirements(device, uniformBuffers[i].handle, &memReqs);
		   VkMemoryAllocateInfo allocInfo{
				.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
				// Note that we use the size we got from the memory requirements and not the actual buffer size, as the former may be larger due to alignment requirements of the device
				.allocationSize = memReqs.size,
				// Get the memory type index that supports host visible memory access
				// Most implementations offer multiple memory types and selecting the correct one to allocate memory from is crucial
				// We also want the buffer to be host coherent so we don't have to flush (or sync after every update).
				.memoryTypeIndex = getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
			};
			// Allocate memory for the uniform buffer
			VK_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, &(uniformBuffers[i].memory)));
			// Bind memory to buffer
			VK_CHECK_RESULT(vkBindBufferMemory(device, uniformBuffers[i].handle, uniformBuffers[i].memory, 0));
			// We map the buffer once, so we can update it without having to map it again
			VK_CHECK_RESULT(vkMapMemory(device, uniformBuffers[i].memory, 0, sizeof(ShaderData), 0, (void**)&uniformBuffers[i].mapped));
		}

	}

	void prepare() override
	{
		VulkanExampleBase::prepare();
		createSynchronizationPrimitives();
		createCommandBuffers();
		createVertexBuffer();
		createUniformBuffers();
		createDescriptors();
		createPipeline();
		prepared = true;
	}

	void render() override
	{
		// Use a fence to wait until the command buffer has finished execution before using it again
		vkWaitForFences(device, 1, &waitFences[currentFrame], VK_TRUE, UINT64_MAX);
		VK_CHECK_RESULT(vkResetFences(device, 1, &waitFences[currentFrame]));

		// Get the next swap chain image from the implementation
		// Note that the implementation is free to return the images in any order, so we must use the acquire function and can't just cycle through the images/imageIndex on our own
		uint32_t imageIndex{ 0 };
		VkResult result = vkAcquireNextImageKHR(device, swapChain.swapChain, UINT64_MAX, presentCompleteSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			windowResize();
			return;
		} else if ((result != VK_SUCCESS) && (result != VK_SUBOPTIMAL_KHR)) {
			throw "Could not acquire the next swap chain image!";
		}

		// Update the uniform buffer for the next frame
		ShaderData shaderData{
			.projectionMatrix = camera.matrices.perspective,
			.modelMatrix = glm::mat4(1.0f),
			.viewMatrix = camera.matrices.view,
		};
		// Copy the current matrices to the current frame's uniform buffer. As we requested a host coherent memory type for the uniform buffer, the write is instantly visible to the GPU.
		memcpy(uniformBuffers[currentFrame].mapped, &shaderData, sizeof(ShaderData));

		// Build the command buffer for the next frame to render
		vkResetCommandBuffer(commandBuffers[currentFrame], 0);
		VkCommandBufferBeginInfo cmdBufInfo{ .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		const VkCommandBuffer commandBuffer = commandBuffers[currentFrame];
		VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &cmdBufInfo));

		// With dynamic rendering we need to explicitly add layout transitions by using barriers, this set of barriers prepares the color and depth images for output
		vks::tools::insertImageMemoryBarrier(commandBuffer, swapChain.images[imageIndex], 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
		vks::tools::insertImageMemoryBarrier(commandBuffer, depthStencil.image, 0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VkImageSubresourceRange{ VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1 });

		// New structures are used to define the attachments used in dynamic rendering
		// Color attachment
		VkRenderingAttachmentInfo colorAttachment{
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView = swapChain.imageViews[imageIndex],
			.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.clearValue = {
				.color = { 0.0f, 0.0f, 0.2f, 0.0f },
			}
		};
		// Depth/stencil attachment
		VkRenderingAttachmentInfo depthStencilAttachment{
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView = depthStencil.view,
			.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.clearValue = {
				.depthStencil = { 1.0f,  0 },
			}
		};

		VkRenderingInfo renderingInfo{
			.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
			.renderArea = { 0, 0, width, height },
			.layerCount = 1,
			.colorAttachmentCount = 1,
			.pColorAttachments = &colorAttachment,
			.pDepthAttachment = &depthStencilAttachment,
			.pStencilAttachment = &depthStencilAttachment
		};

		// Start a dynamic rendering section
		vkCmdBeginRendering(commandBuffer, &renderingInfo);
		// Update dynamic viewport state
		VkViewport viewport{ 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		// Update dynamic scissor state
		VkRect2D scissor{ 0, 0, width, height };
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
		// Bind descriptor set for the current frame's uniform buffer, so the shader uses the data from that buffer for this draw
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &uniformBuffers[currentFrame].descriptorSet, 0, nullptr);
		// The pipeline (state object) contains all states of the rendering pipeline, binding it will set all the states specified at pipeline creation time
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		// Bind triangle vertex buffer (contains position and colors)
		VkDeviceSize offsets[1]{ 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.handle, offsets);
		// Bind triangle index buffer
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer.handle, 0, VK_INDEX_TYPE_UINT32);
		// Draw indexed triangle
		vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
		// Finish the current dynamic rendering section
		vkCmdEndRendering(commandBuffer);

		// This barrier prepares the color image for presentation, we don't need to care for the depth image
		vks::tools::insertImageMemoryBarrier(commandBuffer, swapChain.images[imageIndex], VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 0, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_NONE, VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
		VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));

		// Submit the command buffer to the graphics queue

		// Pipeline stage at which the queue submission will wait (via pWaitSemaphores)
		VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		// The submit info structure specifies a command buffer queue submission batch
		VkSubmitInfo submitInfo{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			// Semaphore to wait upon before the submitted command buffer starts executing
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &presentCompleteSemaphores[currentFrame],
			// Pointer to the list of pipeline stages that the semaphore waits will occur at
			.pWaitDstStageMask = &waitStageMask,
			// We submit a single command buffer
			.commandBufferCount = 1,                
			// Command buffers(s) to execute in this batch (submission)
			.pCommandBuffers = &commandBuffer,		
			// Semaphore to be signaled when command buffers have completed
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = &renderCompleteSemaphores[imageIndex]
		};

		// Submit to the graphics queue passing a wait fence
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, waitFences[currentFrame]));

		// Present the current frame buffer to the swap chain
		// Pass the semaphore signaled by the command buffer submission from the submit info as the wait semaphore for swap chain presentation
		// This ensures that the image is not presented to the windowing system until all commands have been submitted
		VkPresentInfoKHR presentInfo{
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &renderCompleteSemaphores[imageIndex],
			.swapchainCount = 1,
			.pSwapchains = &swapChain.swapChain,
			.pImageIndices = &imageIndex
		};
		result = vkQueuePresentKHR(queue, &presentInfo);
		if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR)) {
			windowResize();
		} else if (result != VK_SUCCESS) {
			throw "Could not present the image to the swap chain!";
		}

		// Select the next frame to render to, based on the max. no. of concurrent frames
		currentFrame = (currentFrame + 1) % MAX_CONCURRENT_FRAMES;
	}

	// Override these as otherwise the base class would generate frame buffers and render passes
	void setupFrameBuffer() override {}
	void setupRenderPass() override {}
};

// OS specific main entry points
// Most of the code base is shared for the different supported operating systems, but stuff like message handling differs

#if defined(_WIN32)
// Windows entry point
VulkanExample *vulkanExample;
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (vulkanExample != NULL)
	{
		vulkanExample->handleMessages(hWnd, uMsg, wParam, lParam);
	}
	return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}
int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_  HINSTANCE hPrevInstance, _In_ LPSTR, _In_ int)
{
	for (size_t i = 0; i < __argc; i++) { VulkanExample::args.push_back(__argv[i]); };
	vulkanExample = new VulkanExample();
	vulkanExample->initVulkan();
	vulkanExample->setupWindow(hInstance, WndProc);
	vulkanExample->prepare();
	vulkanExample->renderLoop();
	delete(vulkanExample);
	return 0;
}

#elif defined(__ANDROID__)
// Android entry point
VulkanExample *vulkanExample;
void android_main(android_app* state)
{
	vulkanExample = new VulkanExample();
	state->userData = vulkanExample;
	state->onAppCmd = VulkanExample::handleAppCommand;
	state->onInputEvent = VulkanExample::handleAppInput;
	androidApp = state;
	vulkanExample->renderLoop();
	delete(vulkanExample);
}
#elif defined(_DIRECT2DISPLAY)

// Linux entry point with direct to display wsi
// Direct to Displays (D2D) is used on embedded platforms
VulkanExample *vulkanExample;
static void handleEvent()
{
}
int main(const int argc, const char *argv[])
{
	for (size_t i = 0; i < argc; i++) { VulkanExample::args.push_back(argv[i]); };
	vulkanExample = new VulkanExample();
	vulkanExample->initVulkan();
	vulkanExample->prepare();
	vulkanExample->renderLoop();
	delete(vulkanExample);
	return 0;
}
#elif defined(VK_USE_PLATFORM_DIRECTFB_EXT)
VulkanExample *vulkanExample;
static void handleEvent(const DFBWindowEvent *event)
{
	if (vulkanExample != NULL)
	{
		vulkanExample->handleEvent(event);
	}
}
int main(const int argc, const char *argv[])
{
	for (size_t i = 0; i < argc; i++) { VulkanExample::args.push_back(argv[i]); };
	vulkanExample = new VulkanExample();
	vulkanExample->initVulkan();
	vulkanExample->setupWindow();
	vulkanExample->prepare();
	vulkanExample->renderLoop();
	delete(vulkanExample);
	return 0;
}
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
VulkanExample *vulkanExample;
int main(const int argc, const char *argv[])
{
	for (size_t i = 0; i < argc; i++) { VulkanExample::args.push_back(argv[i]); };
	vulkanExample = new VulkanExample();
	vulkanExample->initVulkan();
	vulkanExample->setupWindow();
	vulkanExample->prepare();
	vulkanExample->renderLoop();
	delete(vulkanExample);
	return 0;
}
#elif defined(__linux__) || defined(__FreeBSD__)

// Linux entry point
VulkanExample *vulkanExample;
#if defined(VK_USE_PLATFORM_XCB_KHR)
static void handleEvent(const xcb_generic_event_t *event)
{
	if (vulkanExample != NULL)
	{
		vulkanExample->handleEvent(event);
	}
}
#else
static void handleEvent()
{
}
#endif
int main(const int argc, const char *argv[])
{
	for (size_t i = 0; i < argc; i++) { VulkanExample::args.push_back(argv[i]); };
	vulkanExample = new VulkanExample();
	vulkanExample->initVulkan();
	vulkanExample->setupWindow();
	vulkanExample->prepare();
	vulkanExample->renderLoop();
	delete(vulkanExample);
	return 0;
}
#elif (defined(VK_USE_PLATFORM_MACOS_MVK) || defined(VK_USE_PLATFORM_METAL_EXT)) && defined(VK_EXAMPLE_XCODE_GENERATED)
VulkanExample *vulkanExample;
int main(const int argc, const char *argv[])
{
	@autoreleasepool
	{
		for (size_t i = 0; i < argc; i++) { VulkanExample::args.push_back(argv[i]); };
		vulkanExample = new VulkanExample();
		vulkanExample->initVulkan();
		vulkanExample->setupWindow(nullptr);
		vulkanExample->prepare();
		vulkanExample->renderLoop();
		delete(vulkanExample);
	}
	return 0;
}
#elif defined(VK_USE_PLATFORM_SCREEN_QNX)
VULKAN_EXAMPLE_MAIN()
#endif
