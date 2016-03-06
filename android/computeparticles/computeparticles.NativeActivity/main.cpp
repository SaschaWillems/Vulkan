/*
* Vulkan Example - Compute shader particle system
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

#define GLM_FORCE_RADIANS
#define GLM_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "AndroidProject1.NativeActivity", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "AndroidProject1.NativeActivity", __VA_ARGS__))

#define VERTEX_BUFFER_BIND_ID 0
#define PARTICLE_COUNT 4 * 1024

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

	float timer = 0.0f;
	float animStart = 50.0f;
	bool animate = true;

	// Vulkan
	struct Vertex {
		float pos[3];
		float uv[2];
	};

	struct Texture {
		VkSampler sampler;
		VkImage image;
		VkImageLayout imageLayout;
		VkDeviceMemory deviceMemory;
		VkImageView view;
		uint32_t width, height;
		uint32_t mipLevels;
	} texture;

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
		VkPipelineVertexInputStateCreateInfo inputState;
		std::vector<VkVertexInputBindingDescription> bindingDescriptions;
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
	} vertices;

	struct {
		VkPipeline solid;
		VkPipeline compute;
	} pipelines;

	uint32_t currentBuffer = 0;

	struct
	{
		VkImage image;
		VkDeviceMemory mem;
		VkImageView view;
	} depthStencil;

	std::vector<VkFramebuffer>frameBuffers;

	VkQueue computeQueue;
	VkCommandBuffer computeCmdBuffer;
	VkPipelineLayout computePipelineLayout;
	VkDescriptorSet computeDescriptorSet;
	VkDescriptorSetLayout computeDescriptorSetLayout;

	vkTools::UniformData computeStorageBuffer;

	struct Particle {
		glm::vec4 pos;
		glm::vec4 col;
		glm::vec4 vel;
	};

	struct {
		float deltaT;
		float destX;
		float destY;
		int32_t particleCount = PARTICLE_COUNT;
	} computeUbo;

	vkTools::UniformData uniformDataCompute;

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

	void loadTexture(const char* fileName, VkFormat format, bool forceLinearTiling)
	{
		VkFormatProperties formatProperties;
		VkResult err;

		AAsset* asset = AAssetManager_open(app->activity->assetManager, fileName, AASSET_MODE_STREAMING);
		assert(asset);
		size_t size = AAsset_getLength(asset);
		assert(size > 0);

		//char *textureData = new char[size];
		void *textureData = malloc(size);
		AAsset_read(asset, textureData, size);
		AAsset_close(asset);

		gli::texture2D tex2D(gli::load((const char*)textureData, size));
		assert(!tex2D.empty());

		texture.width = tex2D[0].dimensions().x;
		texture.height = tex2D[0].dimensions().y;
		texture.mipLevels = tex2D.levels();

		// Get device properites for the requested texture format
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);

		// Only use linear tiling if requested (and supported by the device)
		// Support for linear tiling is mostly limited, so prefer to use
		// optimal tiling instead
		// On most implementations linear tiling will only support a very
		// limited amount of formats and features (mip maps, cubemaps, arrays, etc.)
		VkBool32 useStaging = true;

		// Only use linear tiling if forced
		if (forceLinearTiling)
		{
			// Don't use linear if format is not supported for (linear) shader sampling
			useStaging = !(formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);
		}

		VkImageCreateInfo imageCreateInfo = vkTools::initializers::imageCreateInfo();
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.format = format;
		imageCreateInfo.mipLevels = 1;
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
		imageCreateInfo.usage = (useStaging) ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : VK_IMAGE_USAGE_SAMPLED_BIT;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfo.flags = 0;
		imageCreateInfo.extent = { texture.width, texture.height, 1 };

		VkMemoryAllocateInfo memAllocInfo = vkTools::initializers::memoryAllocateInfo();
		VkMemoryRequirements memReqs;

		startSetupCommandBuffer();

		if (useStaging)
		{
			// Load all available mip levels into linear textures
			// and copy to optimal tiling target
			struct MipLevel {
				VkImage image;
				VkDeviceMemory memory;
			};
			std::vector<MipLevel> mipLevels;
			mipLevels.resize(texture.mipLevels);

			// Copy mip levels
			for (uint32_t level = 0; level < texture.mipLevels; ++level)
			{
				imageCreateInfo.extent.width = tex2D[level].dimensions().x;
				imageCreateInfo.extent.height = tex2D[level].dimensions().y;
				imageCreateInfo.extent.depth = 1;

				err = vkCreateImage(device, &imageCreateInfo, nullptr, &mipLevels[level].image);
				assert(!err);

				vkGetImageMemoryRequirements(device, mipLevels[level].image, &memReqs);
				memAllocInfo.allocationSize = memReqs.size;
				getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memAllocInfo.memoryTypeIndex);
				err = vkAllocateMemory(device, &memAllocInfo, nullptr, &mipLevels[level].memory);
				assert(!err);
				err = vkBindImageMemory(device, mipLevels[level].image, mipLevels[level].memory, 0);
				assert(!err);

				VkImageSubresource subRes = {};
				subRes.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

				VkSubresourceLayout subResLayout;
				void *data;

				vkGetImageSubresourceLayout(device, mipLevels[level].image, &subRes, &subResLayout);
				assert(!err);
				err = vkMapMemory(device, mipLevels[level].memory, 0, memReqs.size, 0, &data);
				assert(!err);
				size_t levelSize = tex2D[level].size();
				memcpy(data, tex2D[level].data(), levelSize);
				vkUnmapMemory(device, mipLevels[level].memory);

				LOGW("setImageLayout %d", 1);

				// Image barrier for linear image (base)
				// Linear image will be used as a source for the copy
				vkTools::setImageLayout(
					setupCmdBuffer,
					mipLevels[level].image,
					VK_IMAGE_ASPECT_COLOR_BIT,
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
			}

			// Setup texture as blit target with optimal tiling
			imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			imageCreateInfo.mipLevels = texture.mipLevels;
			imageCreateInfo.extent = { texture.width, texture.height, 1 };

			err = vkCreateImage(device, &imageCreateInfo, nullptr, &texture.image);
			assert(!err);

			vkGetImageMemoryRequirements(device, texture.image, &memReqs);

			memAllocInfo.allocationSize = memReqs.size;

			getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memAllocInfo.memoryTypeIndex);
			err = vkAllocateMemory(device, &memAllocInfo, nullptr, &texture.deviceMemory);
			assert(!err);
			err = vkBindImageMemory(device, texture.image, texture.deviceMemory, 0);
			assert(!err);

			// Image barrier for optimal image (target)
			// Optimal image will be used as destination for the copy
			vkTools::setImageLayout(
				setupCmdBuffer,
				texture.image,
				VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

			// Copy mip levels one by one
			for (uint32_t level = 0; level < texture.mipLevels; ++level)
			{
				// Copy region for image blit
				VkImageCopy copyRegion = {};

				copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				copyRegion.srcSubresource.baseArrayLayer = 0;
				copyRegion.srcSubresource.mipLevel = 0;
				copyRegion.srcSubresource.layerCount = 1;
				copyRegion.srcOffset = { 0, 0, 0 };

				copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				copyRegion.dstSubresource.baseArrayLayer = 0;
				// Set mip level to copy the linear image to
				copyRegion.dstSubresource.mipLevel = level;
				copyRegion.dstSubresource.layerCount = 1;
				copyRegion.dstOffset = { 0, 0, 0 };

				copyRegion.extent.width = tex2D[level].dimensions().x;
				copyRegion.extent.height = tex2D[level].dimensions().y;
				copyRegion.extent.depth = 1;

				// Put image copy into command buffer
				vkCmdCopyImage(
					setupCmdBuffer,
					mipLevels[level].image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1, &copyRegion);

				// Change texture image layout to shader read after the copy
				texture.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				vkTools::setImageLayout(
					setupCmdBuffer,
					texture.image,
					VK_IMAGE_ASPECT_COLOR_BIT,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					texture.imageLayout);
			}

			// Clean up linear images 
			// No longer required after mip levels
			// have been transformed over to optimal tiling
			for (auto& level : mipLevels)
			{
				vkDestroyImage(device, level.image, nullptr);
				vkFreeMemory(device, level.memory, nullptr);
			}
		}
		else
		{
			// Prefer using optimal tiling, as linear tiling 
			// may support only a small set of features 
			// depending on implementation (e.g. no mip maps, only one layer, etc.)

			VkImage mappableImage;
			VkDeviceMemory mappableMemory;

			// Load mip map level 0 to linear tiling image
			err = vkCreateImage(device, &imageCreateInfo, nullptr, &mappableImage);
			assert(!err);

			// Get memory requirements for this image 
			// like size and alignment
			vkGetImageMemoryRequirements(device, mappableImage, &memReqs);
			// Set memory allocation size to required memory size
			memAllocInfo.allocationSize = memReqs.size;

			// Get memory type that can be mapped to host memory
			getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memAllocInfo.memoryTypeIndex);

			// Allocate host memory
			err = vkAllocateMemory(device, &memAllocInfo, nullptr, &mappableMemory);
			assert(!err);

			// Bind allocated image for use
			err = vkBindImageMemory(device, mappableImage, mappableMemory, 0);
			assert(!err);

			// Get sub resource layout
			// Mip map count, array layer, etc.
			VkImageSubresource subRes = {};
			subRes.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

			VkSubresourceLayout subResLayout;
			void *data;

			// Get sub resources layout 
			// Includes row pitch, size offsets, etc.
			vkGetImageSubresourceLayout(device, mappableImage, &subRes, &subResLayout);
			assert(!err);

			// Map image memory
			err = vkMapMemory(device, mappableMemory, 0, memReqs.size, 0, &data);
			assert(!err);

			// Copy image data into memory
			memcpy(data, tex2D[subRes.mipLevel].data(), tex2D[subRes.mipLevel].size());

			vkUnmapMemory(device, mappableMemory);

			// Linear tiled images don't need to be staged
			// and can be directly used as textures
			texture.image = mappableImage;
			texture.deviceMemory = mappableMemory;
			texture.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			// Setup image memory barrier
			vkTools::setImageLayout(
				setupCmdBuffer,
				texture.image,
				VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
				texture.imageLayout);
		}

		flushSetupCommandBuffer();

		// Create sampler
		// In Vulkan textures are accessed by samplers
		// This separates all the sampling information from the 
		// texture data
		// This means you could have multiple sampler objects
		// for the same texture with different settings
		// Similar to the samplers available with OpenGL 3.3
		VkSamplerCreateInfo sampler = vkTools::initializers::samplerCreateInfo();
		sampler.magFilter = VK_FILTER_LINEAR;
		sampler.minFilter = VK_FILTER_LINEAR;
		sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler.addressModeV = sampler.addressModeU;
		sampler.addressModeW = sampler.addressModeU;
		sampler.mipLodBias = 0.0f;
		sampler.compareOp = VK_COMPARE_OP_NEVER;
		sampler.minLod = 0.0f;
		// Max level-of-detail should match mip level count
		sampler.maxLod = (useStaging) ? (float)texture.mipLevels : 0.0f;
		// Enable anisotropic filtering
		sampler.maxAnisotropy = 8;
		sampler.anisotropyEnable = VK_TRUE;
		sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		err = vkCreateSampler(device, &sampler, nullptr, &texture.sampler);
		assert(!err);

		// Create image view
		// Textures are not directly accessed by the shaders and
		// are abstracted by image views containing additional
		// information and sub resource ranges
		VkImageViewCreateInfo view = vkTools::initializers::imageViewCreateInfo();
		view.image = VK_NULL_HANDLE;
		view.viewType = VK_IMAGE_VIEW_TYPE_2D;
		view.format = format;
		view.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		view.subresourceRange.baseMipLevel = 0;
		view.subresourceRange.baseArrayLayer = 0;
		view.subresourceRange.layerCount = 1;
		// Linear tiling usually won't support mip maps
		// Only set mip map count if optimal tiling is used
		view.subresourceRange.levelCount = (useStaging) ? texture.mipLevels : 1;
		view.image = texture.image;
		err = vkCreateImageView(device, &view, nullptr, &texture.view);
		assert(!err);
	}

	// Free staging resources used while creating a texture
	void destroyTextureImage(struct Texture texture)
	{
		vkDestroyImage(device, texture.image, nullptr);
		vkFreeMemory(device, texture.deviceMemory, nullptr);
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

		loadTexture(
			"textures/android_robot.ktx", 
			VK_FORMAT_R8G8B8A8_UNORM, 
			false);

		createCommandBuffers();

		// Compute stuff
		getComputeQueue();
		createComputeCommandBuffer();
		prepareStorageBuffers();

		prepareUniformBuffers();
		setupDescriptorSetLayout();
		preparePipelines();
		setupDescriptorPool();
		setupDescriptorSet();

		prepareCompute();

		buildCommandBuffers();
		buildComputeCommandBuffer();

		state.zoom = -5.0f;
		state.rotation = glm::vec3();

		prepared = true;
	}

	void cleanupVulkan()
	{
		prepared = false;
		vkDestroyPipeline(device, pipelines.solid, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyPipelineLayout(device, computePipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, computeDescriptorSetLayout, nullptr);
		vkDestroyBuffer(device, uniformDataCompute.buffer, nullptr);
		vkFreeMemory(device, uniformDataCompute.memory, nullptr);
		vkDestroyBuffer(device, computeStorageBuffer.buffer, nullptr);
		vkFreeMemory(device, computeStorageBuffer.memory, nullptr);

		destroyTextureImage(texture);

		swapChain.cleanup();

		vkDestroyDescriptorPool(device, descriptorPool, nullptr);
		if (setupCmdBuffer != VK_NULL_HANDLE)
		{
			vkFreeCommandBuffers(device, cmdPool, 1, &setupCmdBuffer);

		}
		vkFreeCommandBuffers(device, cmdPool, drawCmdBuffers.size(), drawCmdBuffers.data());
		vkFreeCommandBuffers(device, cmdPool, 1, &postPresentCmdBuffer);
		vkFreeCommandBuffers(device, cmdPool, 1, &computeCmdBuffer);

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

	// Find and create a compute capable device queue
	void getComputeQueue()
	{
		uint32_t queueIndex = 0;
		uint32_t queueCount;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, NULL);
		assert(queueCount >= 1);

		std::vector<VkQueueFamilyProperties> queueProps;
		queueProps.resize(queueCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, queueProps.data());

		for (queueIndex = 0; queueIndex < queueCount; queueIndex++)
		{
			if (queueProps[queueIndex].queueFlags & VK_QUEUE_COMPUTE_BIT)
				break;
		}
		assert(queueIndex < queueCount);

		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.queueFamilyIndex = queueIndex;
		queueCreateInfo.queueCount = 1;
		vkGetDeviceQueue(device, queueIndex, 0, &computeQueue);
	}

	void createComputeCommandBuffer()
	{
		VkCommandBufferAllocateInfo cmdBufAllocateInfo =
			vkTools::initializers::commandBufferAllocateInfo(
				cmdPool,
				VK_COMMAND_BUFFER_LEVEL_PRIMARY,
				1);

		VkResult vkRes = vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &computeCmdBuffer);
		assert(!vkRes);
	}

	void buildComputeCommandBuffer()
	{
		VkCommandBufferBeginInfo cmdBufInfo = vkTools::initializers::commandBufferBeginInfo();;

		vkBeginCommandBuffer(computeCmdBuffer, &cmdBufInfo);

		vkCmdBindPipeline(computeCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelines.compute);
		vkCmdBindDescriptorSets(computeCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 0, 1, &computeDescriptorSet, 0, 0);

		vkCmdDispatch(computeCmdBuffer, PARTICLE_COUNT / 16, 1, 1);

		vkEndCommandBuffer(computeCmdBuffer);
	}

	void updateUniformBuffers()
	{
		computeUbo.deltaT = (1.0f / 60.0f) * 4.0f;
		computeUbo.destX = sin(glm::radians(timer*360.0)) * 0.75f;
		computeUbo.destY = cos(glm::radians(timer*360.0)) * 0.10f;
		uint8_t *pData;
		VkResult err = vkMapMemory(device, uniformDataCompute.memory, 0, sizeof(computeUbo), 0, (void **)&pData);
		assert(!err);
		memcpy(pData, &computeUbo, sizeof(computeUbo));
		vkUnmapMemory(device, uniformDataCompute.memory);
	}

	void prepareUniformBuffers()
	{
		// Prepare and initialize uniform buffer containing shader uniforms
		VkMemoryRequirements memReqs;

		// Vertex shader uniform buffer block
		VkBufferCreateInfo bufferInfo = {};
		VkMemoryAllocateInfo allocInfo = vkTools::initializers::memoryAllocateInfo();
		VkResult err;

		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = sizeof(computeUbo);
		bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

		err = vkCreateBuffer(device, &bufferInfo, nullptr, &uniformDataCompute.buffer);
		assert(!err);
		vkGetBufferMemoryRequirements(device, uniformDataCompute.buffer, &memReqs);
		allocInfo.allocationSize = memReqs.size;
		getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &allocInfo.memoryTypeIndex);
		err = vkAllocateMemory(device, &allocInfo, nullptr, &(uniformDataCompute.memory));
		assert(!err);
		err = vkBindBufferMemory(device, uniformDataCompute.buffer, uniformDataCompute.memory, 0);
		assert(!err);

		uniformDataCompute.descriptor.buffer = uniformDataCompute.buffer;
		uniformDataCompute.descriptor.offset = 0;
		uniformDataCompute.descriptor.range = sizeof(computeUbo);

		updateUniformBuffers();
	}

	void preparePipelines()
	{
		VkResult err;

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
			vkTools::initializers::pipelineInputAssemblyStateCreateInfo(
				VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
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

		shaderStages[0] = loadShader("shaders/particle.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader("shaders/particle.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

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

		// Additive blending
		blendAttachmentState.colorWriteMask = 0xF;
		blendAttachmentState.blendEnable = VK_TRUE;
		blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
		blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
		blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
		blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;

		err = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.solid);
		assert(!err);
	}

	// Setup and fill the compute shader storage buffers for
	// vertex positions and velocities
	void prepareStorageBuffers()
	{
		float destPosX = 0.0f;
		float destPosY = 0.0f;

		// Initial particle positions
		std::vector<Particle> particleBuffer;
		for (int i = 0; i < PARTICLE_COUNT; ++i)
		{
			// Position
			float aspectRatio = (float)height / (float)width;
			float rndVal = (float)rand() / (float)(RAND_MAX / (360.0f * 3.14f * 2.0f));
			float rndRad = (float)rand() / (float)(RAND_MAX)* 0.65f;
			Particle p;
			p.pos = glm::vec4(
				destPosX + cos(rndVal) * rndRad * aspectRatio,
				destPosY + sin(rndVal) * rndRad,
				0.0f,
				1.0f);
			p.col = glm::vec4(
				(float)(rand() % 255) / 255.0f,
				(float)(rand() % 255) / 255.0f,
				(float)(rand() % 255) / 255.0f,
				1.0f);
			p.vel = glm::vec4(0.0f);
			particleBuffer.push_back(p);
		}

		// Buffer size is the same for all storage buffers
		uint32_t storageBufferSize = particleBuffer.size() * sizeof(Particle);

		VkMemoryAllocateInfo memAlloc = vkTools::initializers::memoryAllocateInfo();
		VkMemoryRequirements memReqs;

		VkResult err;
		void *data;

		// Allocate and fill storage buffer object
		VkBufferCreateInfo vBufferInfo =
			vkTools::initializers::bufferCreateInfo(
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				storageBufferSize);
		err = vkCreateBuffer(device, &vBufferInfo, nullptr, &computeStorageBuffer.buffer);
		assert(!err);
		vkGetBufferMemoryRequirements(device, computeStorageBuffer.buffer, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memAlloc.memoryTypeIndex);
		err = vkAllocateMemory(device, &memAlloc, nullptr, &computeStorageBuffer.memory);
		assert(!err);
		err = vkMapMemory(device, computeStorageBuffer.memory, 0, storageBufferSize, 0, &data);
		assert(!err);
		memcpy(data, particleBuffer.data(), storageBufferSize);
		vkUnmapMemory(device, computeStorageBuffer.memory);
		err = vkBindBufferMemory(device, computeStorageBuffer.buffer, computeStorageBuffer.memory, 0);
		assert(!err);

		computeStorageBuffer.descriptor.buffer = computeStorageBuffer.buffer;
		computeStorageBuffer.descriptor.offset = 0;
		computeStorageBuffer.descriptor.range = storageBufferSize;

		// Binding description
		vertices.bindingDescriptions.resize(1);
		vertices.bindingDescriptions[0] =
			vkTools::initializers::vertexInputBindingDescription(
				VERTEX_BUFFER_BIND_ID,
				sizeof(Particle),
				VK_VERTEX_INPUT_RATE_VERTEX);

		// Attribute descriptions
		// Describes memory layout and shader positions
		vertices.attributeDescriptions.resize(2);
		// Location 0 : Position
		vertices.attributeDescriptions[0] =
			vkTools::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				0,
				VK_FORMAT_R32G32B32A32_SFLOAT,
				0);
		// Location 1 : Color
		vertices.attributeDescriptions[1] =
			vkTools::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				1,
				VK_FORMAT_R32G32B32A32_SFLOAT,
				sizeof(float) * 4);

		// Assign to vertex buffer
		vertices.inputState = vkTools::initializers::pipelineVertexInputStateCreateInfo();
		vertices.inputState.vertexBindingDescriptionCount = vertices.bindingDescriptions.size();
		vertices.inputState.pVertexBindingDescriptions = vertices.bindingDescriptions.data();
		vertices.inputState.vertexAttributeDescriptionCount = vertices.attributeDescriptions.size();
		vertices.inputState.pVertexAttributeDescriptions = vertices.attributeDescriptions.data();
	}

	void prepareCompute()
	{
		// Create compute pipeline
		// Compute pipelines are created separate from graphics pipelines
		// even if they use the same queue

		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings;
		setLayoutBindings.push_back(
			// Binding 0 : Particle position storage buffer
			vkTools::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				VK_SHADER_STAGE_COMPUTE_BIT,
				0));
		setLayoutBindings.push_back(
			// Binding 1 : Uniform buffer
			vkTools::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				VK_SHADER_STAGE_COMPUTE_BIT,
				1));

		VkDescriptorSetLayoutCreateInfo descriptorLayout =
			vkTools::initializers::descriptorSetLayoutCreateInfo(
				setLayoutBindings.data(),
				setLayoutBindings.size());

		VkResult err = vkCreateDescriptorSetLayout(
			device,
			&descriptorLayout,
			nullptr,
			&computeDescriptorSetLayout);
		assert(!err);

		VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo =
			vkTools::initializers::pipelineLayoutCreateInfo(
				&computeDescriptorSetLayout,
				1);

		err = vkCreatePipelineLayout(
			device,
			&pPipelineLayoutCreateInfo,
			nullptr,
			&computePipelineLayout);
		assert(!err);

		VkDescriptorSetAllocateInfo allocInfo =
			vkTools::initializers::descriptorSetAllocateInfo(
				descriptorPool,
				&computeDescriptorSetLayout,
				1);

		err = vkAllocateDescriptorSets(device, &allocInfo, &computeDescriptorSet);
		assert(!err);

		std::vector<VkWriteDescriptorSet> computeWriteDescriptorSets;
		computeWriteDescriptorSets.push_back(
			// Binding 0 : Particle position storage buffer
			vkTools::initializers::writeDescriptorSet(
				computeDescriptorSet,
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				0,
				&computeStorageBuffer.descriptor));
		computeWriteDescriptorSets.push_back(
			// Binding 1 : Uniform buffer
			vkTools::initializers::writeDescriptorSet(
				computeDescriptorSet,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				1,
				&uniformDataCompute.descriptor));

		vkUpdateDescriptorSets(device, computeWriteDescriptorSets.size(), computeWriteDescriptorSets.data(), 0, NULL);

		// Create pipeline		
		VkComputePipelineCreateInfo computePipelineCreateInfo =
			vkTools::initializers::computePipelineCreateInfo(
				computePipelineLayout,
				0);
		computePipelineCreateInfo.stage = loadShader("shaders/particle.comp.spv", VK_SHADER_STAGE_COMPUTE_BIT);

		err = vkCreateComputePipelines(device, pipelineCache, 1, &computePipelineCreateInfo, nullptr, &pipelines.compute);
		assert(!err);
	}

	void setupDescriptorPool()
	{
		std::vector<VkDescriptorPoolSize> poolSizes;
		poolSizes.push_back(vkTools::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1));
		poolSizes.push_back(vkTools::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1));
		poolSizes.push_back(vkTools::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1));

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
			// Binding 0 : Fragment shader image sampler
			vkTools::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_SHADER_STAGE_FRAGMENT_BIT,
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

		// Image descriptor for the color map texture
		VkDescriptorImageInfo texDescriptor =
			vkTools::initializers::descriptorImageInfo(
				texture.sampler,
				texture.view,
				VK_IMAGE_LAYOUT_GENERAL);

		std::vector<VkWriteDescriptorSet> writeDescriptorSets;
		writeDescriptorSets.push_back(
			// Binding 0 : Fragment shader texture sampler
			vkTools::initializers::writeDescriptorSet(
				descriptorSet,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				0,
				&texDescriptor));

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
		VkCommandBufferBeginInfo cmdBufInfo = vkTools::initializers::commandBufferBeginInfo();

		VkClearValue clearValues[2];
		clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderPassBeginInfo = vkTools::initializers::renderPassBeginInfo();
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

			// Buffer memory barrier to make sure that compute shader
			// writes are finished before using the storage buffer 
			// in the vertex shader
			VkBufferMemoryBarrier bufferBarrier = vkTools::initializers::bufferMemoryBarrier();
			// Source access : Compute shader buffer write
			bufferBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
			// Dest access : Vertex shader access (attribute binding)
			bufferBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
			bufferBarrier.buffer = computeStorageBuffer.buffer;
			bufferBarrier.offset = 0;
			bufferBarrier.size = computeStorageBuffer.descriptor.range;
			bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

			vkCmdPipelineBarrier(
				drawCmdBuffers[i],
				VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_FLAGS_NONE,
				0, nullptr,
				1, &bufferBarrier,
				0, nullptr);

			vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport viewport = vkTools::initializers::viewport(
				(float)width,
				(float)height,
				0.0f,
				1.0f
				);
			vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

			VkRect2D scissor = vkTools::initializers::rect2D(
				width,
				height,
				0,
				0
				);
			vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

			vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);
			vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.solid);

			VkDeviceSize offsets[1] = { 0 };
			vkCmdBindVertexBuffers(drawCmdBuffers[i], VERTEX_BUFFER_BIND_ID, 1, &computeStorageBuffer.buffer, offsets);
			vkCmdDraw(drawCmdBuffers[i], PARTICLE_COUNT, 1, 0, 0);

			vkCmdEndRenderPass(drawCmdBuffers[i]);

			VkImageMemoryBarrier prePresentBarrier = vkTools::prePresentBarrier(swapChain.buffers[i].image);
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
		VkSemaphoreCreateInfo presentCompleteSemaphoreCreateInfo = vkTools::initializers::semaphoreCreateInfo();

		err = vkCreateSemaphore(device, &presentCompleteSemaphoreCreateInfo, nullptr, &presentCompleteSemaphore);
		assert(!err);

		// Get next image in the swap chain (back/front buffer)
		err = swapChain.acquireNextImage(presentCompleteSemaphore, &currentBuffer);
		assert(!err);

		VkSubmitInfo submitInfo = vkTools::initializers::submitInfo();
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &presentCompleteSemaphore;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];

		// Submit draw command buffer
		err = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
		assert(!err);

		err = swapChain.queuePresent(queue, currentBuffer);
		assert(!err);

		vkDestroySemaphore(device, presentCompleteSemaphore, nullptr);

		VkImageMemoryBarrier postPresentBarrier = vkTools::postPresentBarrier(swapChain.buffers[currentBuffer].image);

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

		// Compute
		VkSubmitInfo computeSubmitInfo = vkTools::initializers::submitInfo();
		computeSubmitInfo.commandBufferCount = 1;
		computeSubmitInfo.pCommandBuffers = &computeCmdBuffer;

		err = vkQueueSubmit(computeQueue, 1, &computeSubmitInfo, VK_NULL_HANDLE);
		assert(!err);

		err = vkQueueWaitIdle(computeQueue);
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
				if (engine->animStart > 0.0f)
				{
					engine->animStart -= (1.0f / 60.0f) * 5.0f;
				}
				if ((engine->animate) & (engine->animStart <= 0.0f))
				{
					engine->timer += (1.0f / 60.0f) * 0.1f;
					if (engine->timer > 1.0)
					{
						engine->timer -= 1.0f;
					}
				}
				engine->updateUniformBuffers();
			}
			engine->draw();
		}
	}
}
