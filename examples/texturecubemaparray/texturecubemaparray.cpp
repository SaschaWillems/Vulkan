/*
* Vulkan Example - Cube map array texture loading and displaying
*
* Copyright (C) 2020 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "vulkanexamplebase.h"
#include "VulkanglTFModel.h"
#include <ktx.h>
#include <ktxvulkan.h>

#define ENABLE_VALIDATION false

class VulkanExample : public VulkanExampleBase
{
public:
	bool displaySkybox = true;

	vks::Texture cubeMapArray;

	struct Meshes {
		vkglTF::Model skybox;
		std::vector<vkglTF::Model> objects;
		int32_t objectIndex = 0;
	} models;

	struct {
		vks::Buffer object;
		vks::Buffer skybox;
	} uniformBuffers;

	struct ShaderData {
		glm::mat4 projection;
		glm::mat4 modelView;
		glm::mat4 inverseModelview;
		float lodBias = 0.0f;
		int cubeMapIndex = 1;
	} shaderData;

	struct {
		VkPipeline skybox;
		VkPipeline reflect;
	} pipelines;

	struct {
		VkDescriptorSet object;
		VkDescriptorSet skybox;
	} descriptorSets;

	VkPipelineLayout pipelineLayout;
	VkDescriptorSetLayout descriptorSetLayout;

	std::vector<std::string> objectNames;

	VulkanExample() : VulkanExampleBase(ENABLE_VALIDATION)
	{
		title = "Cube map textures";
		camera.type = Camera::CameraType::lookat;
		camera.setPosition(glm::vec3(0.0f, 0.0f, -4.0f));
		camera.setRotationSpeed(0.25f);
		camera.setPerspective(60.0f, (float)width / (float)height, 0.1f, 256.0f);
	}

	~VulkanExample()
	{
		// Clean up texture resources
		vkDestroyImageView(device, cubeMapArray.view, nullptr);
		vkDestroyImage(device, cubeMapArray.image, nullptr);
		vkDestroySampler(device, cubeMapArray.sampler, nullptr);
		vkFreeMemory(device, cubeMapArray.deviceMemory, nullptr);

		vkDestroyPipeline(device, pipelines.skybox, nullptr);
		vkDestroyPipeline(device, pipelines.reflect, nullptr);

		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

		uniformBuffers.object.destroy();
		uniformBuffers.skybox.destroy();
	}

	// Enable physical device features required for this example
	virtual void getEnabledFeatures()
	{
		if (deviceFeatures.imageCubeArray) {
			enabledFeatures.imageCubeArray = VK_TRUE;
		} else {
			vks::tools::exitFatal("Selected GPU does not support cube map arrays!", VK_ERROR_FEATURE_NOT_PRESENT);
		}
		enabledFeatures.imageCubeArray = VK_TRUE;
		if (deviceFeatures.samplerAnisotropy) {
			enabledFeatures.samplerAnisotropy = VK_TRUE;
		}
	};

	void loadCubemapArray(std::string filename, VkFormat format, bool forceLinearTiling)
	{
		ktxResult result;
		ktxTexture* ktxTexture;

#if defined(__ANDROID__)
		// Textures are stored inside the apk on Android (compressed)
		// So they need to be loaded via the asset manager
		AAsset* asset = AAssetManager_open(androidApp->activity->assetManager, filename.c_str(), AASSET_MODE_STREAMING);
		if (!asset) {
			vks::tools::exitFatal("Could not load texture from " + filename + "\n\nThe file may be part of the additional asset pack.\n\nRun \"download_assets.py\" in the repository root to download the latest version.", -1);
		}
		size_t size = AAsset_getLength(asset);
		assert(size > 0);

		ktx_uint8_t *textureData = new ktx_uint8_t[size];
		AAsset_read(asset, textureData, size);
		AAsset_close(asset);
		result = ktxTexture_CreateFromMemory(textureData, size, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture);
		delete[] textureData;
#else
		if (!vks::tools::fileExists(filename)) {
			vks::tools::exitFatal("Could not load texture from " + filename + "\n\nThe file may be part of the additional asset pack.\n\nRun \"download_assets.py\" in the repository root to download the latest version.", -1);
		}
		result = ktxTexture_CreateFromNamedFile(filename.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture);
#endif
		assert(result == KTX_SUCCESS);

		// Get properties required for using and upload texture data from the ktx texture object
		cubeMapArray.width = ktxTexture->baseWidth;
		cubeMapArray.height = ktxTexture->baseHeight;
		cubeMapArray.mipLevels = ktxTexture->numLevels;
		cubeMapArray.layerCount = ktxTexture->numLayers;
		ktx_uint8_t *ktxTextureData = ktxTexture_GetData(ktxTexture);
		ktx_size_t ktxTextureSize = ktxTexture_GetSize(ktxTexture);

		vks::Buffer sourceData;

		// Create a host-visible source buffer that contains the raw image data
		VkBufferCreateInfo bufferCreateInfo = vks::initializers::bufferCreateInfo();
		bufferCreateInfo.size = ktxTextureSize;
		bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		VK_CHECK_RESULT(vkCreateBuffer(device, &bufferCreateInfo, nullptr, &sourceData.buffer));

		// Get memory requirements for the source buffer (alignment, memory type bits)
		VkMemoryRequirements memReqs;
		vkGetBufferMemoryRequirements(device, sourceData.buffer, &memReqs);
		VkMemoryAllocateInfo memAllocInfo = vks::initializers::memoryAllocateInfo();
		memAllocInfo.allocationSize = memReqs.size;
		// Get memory type index for a host visible buffer
		memAllocInfo.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device, &memAllocInfo, nullptr, &sourceData.memory));
		VK_CHECK_RESULT(vkBindBufferMemory(device, sourceData.buffer, sourceData.memory, 0));

		// Copy the ktx image data into the source buffer
		uint8_t *data;
		VK_CHECK_RESULT(vkMapMemory(device, sourceData.memory, 0, memReqs.size, 0, (void **)&data));
		memcpy(data, ktxTextureData, ktxTextureSize);
		vkUnmapMemory(device, sourceData.memory);

		// Create optimal tiled target image
		VkImageCreateInfo imageCreateInfo = vks::initializers::imageCreateInfo();
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.format = format;
		imageCreateInfo.mipLevels = cubeMapArray.mipLevels;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageCreateInfo.extent = { cubeMapArray.width, cubeMapArray.height, 1 };
		imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		// Cube faces count as array layers in Vulkan
		imageCreateInfo.arrayLayers = 6 * cubeMapArray.layerCount;
		// This flag is required for cube map images
		imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
		VK_CHECK_RESULT(vkCreateImage(device, &imageCreateInfo, nullptr, &cubeMapArray.image));

		// Allocate memory for the cube map array image
		vkGetImageMemoryRequirements(device, cubeMapArray.image, &memReqs);
		memAllocInfo.allocationSize = memReqs.size;
		memAllocInfo.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device, &memAllocInfo, nullptr, &cubeMapArray.deviceMemory));
		VK_CHECK_RESULT(vkBindImageMemory(device, cubeMapArray.image, cubeMapArray.deviceMemory, 0));

		/*
			We now copy the parts that make up the cube map array to our image via a command buffer
			Cube map arrays in ktx are stored with a layout like this:
			- Mip Level 0
				- Layer 0 (= Cube map 0)
					- Face +X
					- Face -X
					- Face +Y
					- Face -Y
					- Face +Z
					- Face -Z
				- Layer 1 (= Cube map 1)
					- Face +X
					...
			- Mip Level 1
				- Layer 0 (= Cube map 0)
					- Face +X
					...
				- Layer 1 (= Cube map 1)
					- Face +X
					...
		*/

		VkCommandBuffer copyCmd = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

		// Setup buffer copy regions for each face including all of its miplevels
		std::vector<VkBufferImageCopy> bufferCopyRegions;
		uint32_t offset = 0;
		for (uint32_t face = 0; face < 6; face++) {
			for (uint32_t layer = 0; layer < ktxTexture->numLayers; layer++) {
				for (uint32_t level = 0; level < ktxTexture->numLevels; level++) {
					ktx_size_t offset;
					KTX_error_code ret = ktxTexture_GetImageOffset(ktxTexture, level, layer, face, &offset);
					assert(ret == KTX_SUCCESS);
					VkBufferImageCopy bufferCopyRegion = {};
					bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					bufferCopyRegion.imageSubresource.mipLevel = level;
					bufferCopyRegion.imageSubresource.baseArrayLayer = layer * 6 + face;
					bufferCopyRegion.imageSubresource.layerCount = 1;
					bufferCopyRegion.imageExtent.width = ktxTexture->baseWidth >> level;
					bufferCopyRegion.imageExtent.height = ktxTexture->baseHeight >> level;
					bufferCopyRegion.imageExtent.depth = 1;
					bufferCopyRegion.bufferOffset = offset;
					bufferCopyRegions.push_back(bufferCopyRegion);
				}
			}
		}

		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount = cubeMapArray.mipLevels;
		subresourceRange.layerCount = 6 * cubeMapArray.layerCount;

		// Transition target image to accept the writes from our buffer to image copies
		vks::tools::setImageLayout(copyCmd, cubeMapArray.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);

		// Copy the cube map array buffer parts from the staging buffer to the optimal tiled image
		vkCmdCopyBufferToImage(
			copyCmd,
			sourceData.buffer,
			cubeMapArray.image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			static_cast<uint32_t>(bufferCopyRegions.size()),
			bufferCopyRegions.data()
			);

		// Transition image to shader read layout
		cubeMapArray.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		vks::tools::setImageLayout(copyCmd, cubeMapArray.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, cubeMapArray.imageLayout, subresourceRange);

		vulkanDevice->flushCommandBuffer(copyCmd, queue, true);

		// Create sampler
		VkSamplerCreateInfo sampler = vks::initializers::samplerCreateInfo();
		sampler.magFilter = VK_FILTER_LINEAR;
		sampler.minFilter = VK_FILTER_LINEAR;
		sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler.addressModeV = sampler.addressModeU;
		sampler.addressModeW = sampler.addressModeU;
		sampler.mipLodBias = 0.0f;
		sampler.compareOp = VK_COMPARE_OP_NEVER;
		sampler.minLod = 0.0f;
		sampler.maxLod = static_cast<float>(cubeMapArray.mipLevels);
		sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		sampler.maxAnisotropy = 1.0f;
		if (vulkanDevice->features.samplerAnisotropy)
		{
			sampler.maxAnisotropy = vulkanDevice->properties.limits.maxSamplerAnisotropy;
			sampler.anisotropyEnable = VK_TRUE;
		}
		VK_CHECK_RESULT(vkCreateSampler(device, &sampler, nullptr, &cubeMapArray.sampler));

		// Create the image view for a cube map array
		VkImageViewCreateInfo view = vks::initializers::imageViewCreateInfo();
		view.viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
		view.format = format;
		view.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		view.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		view.subresourceRange.layerCount = 6 * cubeMapArray.layerCount;
		view.subresourceRange.levelCount = cubeMapArray.mipLevels;
		view.image = cubeMapArray.image;
		VK_CHECK_RESULT(vkCreateImageView(device, &view, nullptr, &cubeMapArray.view));

		// Clean up staging resources
		vkFreeMemory(device, sourceData.memory, nullptr);
		vkDestroyBuffer(device, sourceData.buffer, nullptr);
		ktxTexture_Destroy(ktxTexture);
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
			// Set target frame buffer
			renderPassBeginInfo.framebuffer = frameBuffers[i];

			VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo));

			vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport viewport = vks::initializers::viewport((float)width,	(float)height, 0.0f, 1.0f);
			vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

			VkRect2D scissor = vks::initializers::rect2D(width,	height,	0, 0);
			vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

			VkDeviceSize offsets[1] = { 0 };

			// Skybox
			if (displaySkybox)
			{
				vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets.skybox, 0, NULL);
				vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.skybox);
				models.skybox.draw(drawCmdBuffers[i]);
			}

			// 3D object
			vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets.object, 0, NULL);
			vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.reflect);
			models.objects[models.objectIndex].draw(drawCmdBuffers[i]);

			drawUI(drawCmdBuffers[i]);

			vkCmdEndRenderPass(drawCmdBuffers[i]);

			VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
		}
	}

	void loadAssets()
	{
		uint32_t glTFLoadingFlags = vkglTF::FileLoadingFlags::PreTransformVertices | vkglTF::FileLoadingFlags::FlipY;
		// Skybox
		models.skybox.loadFromFile(getAssetPath() + "models/cube.gltf", vulkanDevice, queue, glTFLoadingFlags);
		// Objects
		std::vector<std::string> filenames = { "sphere.gltf", "teapot.gltf", "torusknot.gltf", "venus.gltf" };
		objectNames = { "Sphere", "Teapot", "Torusknot", "Venus" };
		models.objects.resize(filenames.size());
		for (size_t i = 0; i < filenames.size(); i++) {
			models.objects[i].loadFromFile(getAssetPath() + "models/" + filenames[i], vulkanDevice, queue, glTFLoadingFlags);
		}
		// Load the cube map array from a ktx texture file
		loadCubemapArray(getAssetPath() + "textures/cubemap_array.ktx", VK_FORMAT_R8G8B8A8_UNORM, false);
	}

	void setupDescriptorPool()
	{
		const std::vector<VkDescriptorPoolSize> poolSizes = {
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2),
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2)
		};
		const VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, 2);
		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));
	}

	void setupDescriptorSetLayout()
	{
		const std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
			// Binding 0 : Uniform buffer
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0),
			// Binding 1 : Fragment shader image sampler
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1)
		};

		const VkDescriptorSetLayoutCreateInfo descriptorLayout = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayout));

		const VkPipelineLayoutCreateInfo pipelineLayoutCI = vks::initializers::pipelineLayoutCreateInfo(&descriptorSetLayout, 1);
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &pipelineLayout));
	}

	void setupDescriptorSets()
	{
		// Image descriptor for the cube map texture
		VkDescriptorImageInfo textureDescriptor = vks::initializers::descriptorImageInfo(cubeMapArray.sampler, cubeMapArray.view, cubeMapArray.imageLayout);
		VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);

		// 3D object descriptor set
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.object));
		std::vector<VkWriteDescriptorSet> writeDescriptorSets =
		{
			// Binding 0 : Vertex shader uniform buffer
			vks::initializers::writeDescriptorSet(descriptorSets.object, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffers.object.descriptor),
			// Binding 1 : Fragment shader cubemap sampler
			vks::initializers::writeDescriptorSet(descriptorSets.object, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &textureDescriptor)
		};
		vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);

		// Sky box descriptor set
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.skybox));
		writeDescriptorSets =
		{
			// Binding 0 : Vertex shader uniform buffer
			vks::initializers::writeDescriptorSet(descriptorSets.skybox, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffers.skybox.descriptor),
			// Binding 1 : Fragment shader cubemap sampler
			vks::initializers::writeDescriptorSet(descriptorSets.skybox, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &textureDescriptor)
		};
		vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
	}

	void preparePipelines()
	{
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
		VkPipelineRasterizationStateCreateInfo rasterizationState = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
		VkPipelineColorBlendAttachmentState blendAttachmentState = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
		VkPipelineColorBlendStateCreateInfo colorBlendState = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
		VkPipelineDepthStencilStateCreateInfo depthStencilState = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_FALSE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL);
		VkPipelineViewportStateCreateInfo viewportState = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
		VkPipelineMultisampleStateCreateInfo multisampleState = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
		std::vector<VkDynamicState> dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
		VkPipelineDynamicStateCreateInfo dynamicState = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

		VkGraphicsPipelineCreateInfo pipelineCI = vks::initializers::pipelineCreateInfo(pipelineLayout, renderPass, 0);
		pipelineCI.pInputAssemblyState = &inputAssemblyState;
		pipelineCI.pRasterizationState = &rasterizationState;
		pipelineCI.pColorBlendState = &colorBlendState;
		pipelineCI.pMultisampleState = &multisampleState;
		pipelineCI.pViewportState = &viewportState;
		pipelineCI.pDepthStencilState = &depthStencilState;
		pipelineCI.pDynamicState = &dynamicState;
		pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineCI.pStages = shaderStages.data();
		pipelineCI.pVertexInputState = vkglTF::Vertex::getPipelineVertexInputState({ vkglTF::VertexComponent::Position, vkglTF::VertexComponent::Normal });

		// Skybox pipeline (background cube)
		shaderStages[0] = loadShader(getShadersPath() + "texturecubemaparray/skybox.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getShadersPath() + "texturecubemaparray/skybox.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		rasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.skybox));

		// Cube map reflect pipeline
		shaderStages[0] = loadShader(getShadersPath() + "texturecubemaparray/reflect.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getShadersPath() + "texturecubemaparray/reflect.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		// Enable depth test and write
		depthStencilState.depthWriteEnable = VK_TRUE;
		depthStencilState.depthTestEnable = VK_TRUE;
		// Flip cull mode
		rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.reflect));
	}

	void prepareUniformBuffers()
	{
		// Object vertex shader uniform buffer
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&uniformBuffers.object,
			sizeof(ShaderData)));

		// Skybox vertex shader uniform buffer
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&uniformBuffers.skybox,
			sizeof(ShaderData)));

		// Map persistent
		VK_CHECK_RESULT(uniformBuffers.object.map());
		VK_CHECK_RESULT(uniformBuffers.skybox.map());

		updateUniformBuffers();
	}

	void updateUniformBuffers()
	{
		// 3D object
		shaderData.projection = camera.matrices.perspective;
		shaderData.modelView = camera.matrices.view;
		shaderData.inverseModelview = glm::inverse(camera.matrices.view);
		memcpy(uniformBuffers.object.mapped, &shaderData, sizeof(ShaderData));

		// Skybox
		shaderData.modelView = camera.matrices.view;
		// Cancel out translation
		shaderData.modelView[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		memcpy(uniformBuffers.skybox.mapped, &shaderData, sizeof(ShaderData));
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
		preparePipelines();
		setupDescriptorPool();
		setupDescriptorSets();
		buildCommandBuffers();
		prepared = true;
	}

	virtual void render()
	{
		if (!prepared)
			return;
		draw();
		if (camera.updated) {
			updateUniformBuffers();
		}
	}

	virtual void OnUpdateUIOverlay(vks::UIOverlay *overlay)
	{
		if (overlay->header("Settings")) {
			if (overlay->sliderInt("Cube map", &shaderData.cubeMapIndex, 0, cubeMapArray.layerCount - 1)) {
				updateUniformBuffers();
			}
			if (overlay->sliderFloat("LOD bias", &shaderData.lodBias, 0.0f, (float)cubeMapArray.mipLevels)) {
				updateUniformBuffers();
			}
			if (overlay->comboBox("Object type", &models.objectIndex, objectNames)) {
				buildCommandBuffers();
			}
			if (overlay->checkBox("Skybox", &displaySkybox)) {
				buildCommandBuffers();
			}
		}
	}
};

VULKAN_EXAMPLE_MAIN()