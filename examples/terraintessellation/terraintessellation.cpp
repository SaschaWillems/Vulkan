/*
* Vulkan Example - Dynamic terrain tessellation
* 
* This samples draw a terrain from a heightmap texture and uses tessellation to add in details based on camera distance
* The height level is generated in the vertex shader by reading from the heightmap image
*
* Copyright (C) 2016-2025 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "vulkanexamplebase.h"
#include "VulkanglTFModel.h"
#include "frustum.hpp"
#include <ktx.h>
#include <ktxvulkan.h>

class VulkanExample : public VulkanExampleBase
{
public:
	bool wireframe = false;
	bool tessellation = true;

	// Holds the buffers for rendering the tessellated terrain
	struct {
		vks::Buffer vertexBuffer;
		vks::Buffer indexBuffer;
		uint32_t indexCount{};
	} terrain;

	struct {
		vks::Texture2D heightMap;
		vks::Texture2D skySphere;
		vks::Texture2DArray terrainArray;
	} textures;

	struct {
		vkglTF::Model skysphere;
	} models;

	struct UniformBuffers {
		vks::Buffer terrainTessellation;
		vks::Buffer skysphereVertex;
	};
	std::array<UniformBuffers, maxConcurrentFrames> uniformBuffers;

	// Shared values for vertex, tessellation control and evaluation stages
	struct UniformDataTessellation {
		glm::mat4 projection;
		glm::mat4 modelview;
		glm::vec4 lightPos = glm::vec4(-48.0f, -40.0f, 46.0f, 0.0f);
		glm::vec4 frustumPlanes[6];
		float displacementFactor = 32.0f;
		float tessellationFactor = 0.75f;
		glm::vec2 viewportDim;
		// Desired size of tessellated quad patch edge
		float tessellatedEdgeSize = 20.0f;
	} uniformDataTessellation;

	// Skysphere vertex shader stage
	struct UniformDataVertex {
		glm::mat4 mvp;
	} uniformDataVertex;

	struct Pipelines {
		VkPipeline terrain{ VK_NULL_HANDLE };
		VkPipeline wireframe{ VK_NULL_HANDLE };
		VkPipeline skysphere{ VK_NULL_HANDLE };
	} pipelines;

	struct {
		VkDescriptorSetLayout terrain{ VK_NULL_HANDLE };
		VkDescriptorSetLayout skysphere{ VK_NULL_HANDLE };
	} descriptorSetLayouts;

	struct {
		VkPipelineLayout terrain{ VK_NULL_HANDLE };
		VkPipelineLayout skysphere{ VK_NULL_HANDLE };
	} pipelineLayouts;

	struct DescriptorSets {
		VkDescriptorSet terrain{ VK_NULL_HANDLE };
		VkDescriptorSet skysphere{ VK_NULL_HANDLE };
	};
	std::array<DescriptorSets, maxConcurrentFrames> descriptorSets;

	// If supported, this sample will gather pipeline statistics to show e.g. tessellation related information
	struct {
		VkBuffer buffer{ VK_NULL_HANDLE };
		VkDeviceMemory memory{ VK_NULL_HANDLE };
	} queryResult;
	VkQueryPool queryPool{ VK_NULL_HANDLE };
	uint64_t pipelineStats[2] = { 0 };

	// View frustum passed to tessellation control shader for culling
	vks::Frustum frustum;

	VulkanExample() : VulkanExampleBase()
	{
		title = "Dynamic terrain tessellation";
		camera.type = Camera::CameraType::firstperson;
		camera.setPerspective(60.0f, (float)width / (float)height, 0.1f, 512.0f);
		camera.setRotation(glm::vec3(-12.0f, 159.0f, 0.0f));
		camera.setTranslation(glm::vec3(18.0f, 22.5f, 57.5f));
		camera.movementSpeed = 10.0f;
	}

	~VulkanExample()
	{
		if (device) {
			vkDestroyPipeline(device, pipelines.terrain, nullptr);
			if (pipelines.wireframe != VK_NULL_HANDLE) {
				vkDestroyPipeline(device, pipelines.wireframe, nullptr);
			}
			vkDestroyPipeline(device, pipelines.skysphere, nullptr);
			vkDestroyPipelineLayout(device, pipelineLayouts.skysphere, nullptr);
			vkDestroyPipelineLayout(device, pipelineLayouts.terrain, nullptr);
			vkDestroyDescriptorSetLayout(device, descriptorSetLayouts.terrain, nullptr);
			vkDestroyDescriptorSetLayout(device, descriptorSetLayouts.skysphere, nullptr);
			for (auto& buffer : uniformBuffers) {
				buffer.skysphereVertex.destroy();
				buffer.terrainTessellation.destroy();
			}
			textures.heightMap.destroy();
			textures.skySphere.destroy();
			textures.terrainArray.destroy();
			terrain.vertexBuffer.destroy();
			terrain.indexBuffer.destroy();
			if (queryPool != VK_NULL_HANDLE) {
				vkDestroyQueryPool(device, queryPool, nullptr);
				vkDestroyBuffer(device, queryResult.buffer, nullptr);
				vkFreeMemory(device, queryResult.memory, nullptr);
			}
		}
	}

	// Enable physical device features required for this example
	virtual void getEnabledFeatures()
	{
		// Tessellation shader support is required for this example
		if (deviceFeatures.tessellationShader) {
			enabledFeatures.tessellationShader = VK_TRUE;
		} else {
			vks::tools::exitFatal("Selected GPU does not support tessellation shaders!", VK_ERROR_FEATURE_NOT_PRESENT);
		}
		// Fill mode non solid is required for wireframe display
		if (deviceFeatures.fillModeNonSolid) {
			enabledFeatures.fillModeNonSolid = VK_TRUE;
		};
		// Enable pipeline statistics if supported (to display them in the UI)
		if (deviceFeatures.pipelineStatisticsQuery) {
			enabledFeatures.pipelineStatisticsQuery = VK_TRUE;
		};
		// Enable anisotropic filtering if supported
		if (deviceFeatures.samplerAnisotropy) {
			enabledFeatures.samplerAnisotropy = VK_TRUE;
		}
	}

	// Setup a pool and a buffer for storing pipeline statistics results
	void setupQueryResultBuffer()
	{
		uint32_t bufSize = 2 * sizeof(uint64_t);

		VkMemoryRequirements memReqs;
		VkMemoryAllocateInfo memAlloc = vks::initializers::memoryAllocateInfo();
		VkBufferCreateInfo bufferCreateInfo =
			vks::initializers::bufferCreateInfo(
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				bufSize);

		// Results are saved in a host visible buffer for easy access by the application
		VK_CHECK_RESULT(vkCreateBuffer(device, &bufferCreateInfo, nullptr, &queryResult.buffer));
		vkGetBufferMemoryRequirements(device, queryResult.buffer, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		memAlloc.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &queryResult.memory));
		VK_CHECK_RESULT(vkBindBufferMemory(device, queryResult.buffer, queryResult.memory, 0));

		// Create query pool
		if (deviceFeatures.pipelineStatisticsQuery) {
			VkQueryPoolCreateInfo queryPoolInfo = {};
			queryPoolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
			queryPoolInfo.queryType = VK_QUERY_TYPE_PIPELINE_STATISTICS;
			queryPoolInfo.pipelineStatistics =
				VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT |
				VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT;
			queryPoolInfo.queryCount = 2;
			VK_CHECK_RESULT(vkCreateQueryPool(device, &queryPoolInfo, NULL, &queryPool));
		}
	}

	void loadAssets()
	{
		const uint32_t glTFLoadingFlags = vkglTF::FileLoadingFlags::PreTransformVertices | vkglTF::FileLoadingFlags::PreMultiplyVertexColors | vkglTF::FileLoadingFlags::FlipY;
		models.skysphere.loadFromFile(getAssetPath() + "models/sphere.gltf", vulkanDevice, queue, glTFLoadingFlags);

		textures.skySphere.loadFromFile(getAssetPath() + "textures/skysphere_rgba.ktx", VK_FORMAT_R8G8B8A8_UNORM, vulkanDevice, queue);
		// Terrain textures are stored in a texture array with layers corresponding to terrain height
		textures.terrainArray.loadFromFile(getAssetPath() + "textures/terrain_texturearray_rgba.ktx", VK_FORMAT_R8G8B8A8_UNORM, vulkanDevice, queue);

		// Height data is stored in a one-channel texture
		textures.heightMap.loadFromFile(getAssetPath() + "textures/terrain_heightmap_r16.ktx", VK_FORMAT_R16_UNORM, vulkanDevice, queue);

		VkSamplerCreateInfo samplerInfo = vks::initializers::samplerCreateInfo();

		// Setup a mirroring sampler for the height map
		vkDestroySampler(device, textures.heightMap.sampler, nullptr);
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		samplerInfo.addressModeV = samplerInfo.addressModeU;
		samplerInfo.addressModeW = samplerInfo.addressModeU;
		samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = (float)textures.heightMap.mipLevels;
		samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		VK_CHECK_RESULT(vkCreateSampler(device, &samplerInfo, nullptr, &textures.heightMap.sampler));
		textures.heightMap.descriptor.sampler = textures.heightMap.sampler;

		// Setup a repeating sampler for the terrain texture layers
		vkDestroySampler(device, textures.terrainArray.sampler, nullptr);
		samplerInfo = vks::initializers::samplerCreateInfo();
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = samplerInfo.addressModeU;
		samplerInfo.addressModeW = samplerInfo.addressModeU;
		samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = (float)textures.terrainArray.mipLevels;
		samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		if (deviceFeatures.samplerAnisotropy) {
			samplerInfo.maxAnisotropy = 4.0f;
			samplerInfo.anisotropyEnable = VK_TRUE;
		}
		VK_CHECK_RESULT(vkCreateSampler(device, &samplerInfo, nullptr, &textures.terrainArray.sampler));
		textures.terrainArray.descriptor.sampler = textures.terrainArray.sampler;
	}

	// Generate a terrain quad patch with normals based on heightmap data
	void generateTerrain()
	{
		const uint32_t patchSize{ 64 };
		const float uvScale{ 1.0f };

		uint16_t* heightdata;
		uint32_t dim;
		uint32_t scale;

		ktxResult result;
		ktxTexture* ktxTexture;

		// We load the heightmap from an un-compressed ktx image with one channel that contains heights
		std::string filename = getAssetPath() + "textures/terrain_heightmap_r16.ktx";
#if defined(__ANDROID__)
		// On Android we need to load the file using the asset manager
		AAsset* asset = AAssetManager_open(androidApp->activity->assetManager, filename.c_str(), AASSET_MODE_STREAMING);
		assert(asset);
		size_t size = AAsset_getLength(asset);
		assert(size > 0);
		ktx_uint8_t* textureData = new ktx_uint8_t[size];
		AAsset_read(asset, textureData, size);
		AAsset_close(asset);
		result = ktxTexture_CreateFromMemory(textureData, size, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture);
		delete[] textureData;

#else
		result = ktxTexture_CreateFromNamedFile(filename.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture);
#endif
		assert(result == KTX_SUCCESS);
		ktx_size_t ktxSize = ktxTexture_GetImageSize(ktxTexture, 0);
		ktx_uint8_t* ktxImage = ktxTexture_GetData(ktxTexture);
		dim = ktxTexture->baseWidth;
		heightdata = new uint16_t[dim * dim];
		memcpy(heightdata, ktxImage, ktxSize);
		scale = dim / patchSize;
		ktxTexture_Destroy(ktxTexture);

		const uint32_t vertexCount = patchSize * patchSize;
		// We use the Vertex definition from the glTF model loader, so we can re-use the vertex input state
		vkglTF::Vertex *vertices = new vkglTF::Vertex[vertexCount];

		const float wx = 2.0f;
		const float wy = 2.0f;

		// Generate a two-dimensional vertex patch
		for (auto x = 0; x < patchSize; x++) {
			for (auto y = 0; y < patchSize; y++) {
				uint32_t index = (x + y * patchSize);
				vertices[index].pos[0] = x * wx + wx / 2.0f - (float)patchSize * wx / 2.0f;
				vertices[index].pos[1] = 0.0f;
				vertices[index].pos[2] = y * wy + wy / 2.0f - (float)patchSize * wy / 2.0f;
				vertices[index].uv = glm::vec2((float)x / (patchSize - 1), (float)y / (patchSize - 1)) * uvScale;
			}
		}

		// Calculate normals from the height map using a sobel filter
		for (auto x = 0; x < patchSize; x++) {
			for (auto y = 0; y < patchSize; y++) {
				// We get 
				float heights[3][3];
				for (auto sx = -1; sx <= 1; sx++) {
					for (auto sy = -1; sy <= 1; sy++) {
						// Get height at sampled position from heightmap
						glm::ivec2 rpos = glm::ivec2(x + sx, y + sy) * glm::ivec2(scale);
						rpos.x = std::max(0, std::min(rpos.x, (int)dim - 1));
						rpos.y = std::max(0, std::min(rpos.y, (int)dim - 1));
						rpos /= glm::ivec2(scale);
						heights[sx + 1][sy + 1] = *(heightdata + (rpos.x + rpos.y * dim) * scale) / 65535.0f;
					}
				}
				glm::vec3 normal;
				// Gx sobel filter
				normal.x = heights[0][0] - heights[2][0] + 2.0f * heights[0][1] - 2.0f * heights[2][1] + heights[0][2] - heights[2][2];
				// Gy sobel filter
				normal.z = heights[0][0] + 2.0f * heights[1][0] + heights[2][0] - heights[0][2] - 2.0f * heights[1][2] - heights[2][2];
				// Calculate missing up component of the normal using the filtered x and y axis
				// The first value controls the bump strength
				normal.y = 0.25f * sqrt( 1.0f - normal.x * normal.x - normal.z * normal.z);

				vertices[x + y * patchSize].normal = glm::normalize(normal * glm::vec3(2.0f, 1.0f, 2.0f));
			}
		}

		delete[] heightdata;

		// Generate indices
		const uint32_t w = (patchSize - 1);
		terrain.indexCount = w * w * 4;
		uint32_t *indices = new uint32_t[terrain.indexCount];
		for (auto x = 0; x < w; x++)
		{
			for (auto y = 0; y < w; y++)
			{
				uint32_t index = (x + y * w) * 4;
				indices[index] = (x + y * patchSize);
				indices[index + 1] = indices[index] + patchSize;
				indices[index + 2] = indices[index + 1] + 1;
				indices[index + 3] = indices[index] + 1;
			}
		}

		// Upload vertices and indices to device

		uint32_t vertexBufferSize = vertexCount * sizeof(vkglTF::Vertex);
		uint32_t indexBufferSize = terrain.indexCount * sizeof(uint32_t);

		vks::Buffer vertexStaging, indexStaging;

		// Stage the terrain vertex data to the device

		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&vertexStaging,
			vertexBufferSize,
			vertices));

		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&indexStaging,
			indexBufferSize,
			indices));

		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			&terrain.vertexBuffer,
			vertexBufferSize));

		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			&terrain.indexBuffer,
			indexBufferSize));

		// Copy from staging buffers
		VkCommandBuffer copyCmd = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

		VkBufferCopy copyRegion = {};

		copyRegion.size = vertexBufferSize;
		vkCmdCopyBuffer(copyCmd, vertexStaging.buffer, terrain.vertexBuffer.buffer, 1, &copyRegion);

		copyRegion.size = indexBufferSize;
		vkCmdCopyBuffer(copyCmd, indexStaging.buffer, terrain.indexBuffer.buffer, 1, &copyRegion);

		vulkanDevice->flushCommandBuffer(copyCmd, queue, true);

		vkDestroyBuffer(device, vertexStaging.buffer, nullptr);
		vkFreeMemory(device, vertexStaging.memory, nullptr);
		vkDestroyBuffer(device, indexStaging.buffer, nullptr);
		vkFreeMemory(device, indexStaging.memory, nullptr);

		delete[] vertices;
		delete[] indices;
	}

	void setupDescriptors()
	{
		// Pool
		std::vector<VkDescriptorPoolSize> poolSizes = {
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, maxConcurrentFrames * 3),
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, maxConcurrentFrames * 3)
		};
		VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, maxConcurrentFrames * 2);
		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));

		// Layouts
		VkDescriptorSetLayoutCreateInfo descriptorLayout;
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings;

		// Terrain
		setLayoutBindings = {
			// Binding 0 : Shared Tessellation shader ubo
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, 0),
			// Binding 1 : Height map
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1),
			// Binding 2 : Terrain texture array layers
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2),
		};
		descriptorLayout = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings.data(), static_cast<uint32_t>(setLayoutBindings.size()));
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayouts.terrain));

		// Skysphere
		setLayoutBindings = {
			// Binding 0 : Vertex shader ubo
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
			// Binding 1 : Color map
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
		};
		descriptorLayout = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings.data(), static_cast<uint32_t>(setLayoutBindings.size()));
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayouts.skysphere));

		// Sets per frame, just like the buffers themselves
		// Images do not need to be duplicated per frame, we reuse the same one for each frame
		for (auto i = 0; i < uniformBuffers.size(); i++) {
			// Terrain
			VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayouts.terrain, 1);
			VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets[i].terrain));
			std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
				// Binding 0 : Shared tessellation shader ubo
				vks::initializers::writeDescriptorSet(descriptorSets[i].terrain, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffers[i].terrainTessellation.descriptor),
				// Binding 1 : Height map
				vks::initializers::writeDescriptorSet(descriptorSets[i].terrain, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &textures.heightMap.descriptor),
				// Binding 2 : Terrain texture array layers
				vks::initializers::writeDescriptorSet(descriptorSets[i].terrain, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, &textures.terrainArray.descriptor),
			};
			vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);

			// Skysphere
			allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayouts.skysphere, 1);
			VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets[i].skysphere));
			writeDescriptorSets = {
				// Binding 0 : Vertex shader ubo
				vks::initializers::writeDescriptorSet(descriptorSets[i].skysphere, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffers[i].skysphereVertex.descriptor),
				// Binding 1 : Fragment shader color map
				vks::initializers::writeDescriptorSet(descriptorSets[i].skysphere, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &textures.skySphere.descriptor),
			};
			vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
		}
	}

	void preparePipelines()	
	{
		// Layouts
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;

		pipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(&descriptorSetLayouts.terrain, 1);
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayouts.terrain));

		pipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(&descriptorSetLayouts.skysphere, 1);
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayouts.skysphere));

		// Pipelines
		VkPipelineRasterizationStateCreateInfo rasterizationState = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
		VkPipelineColorBlendAttachmentState blendAttachmentState = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
		VkPipelineColorBlendStateCreateInfo colorBlendState = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
		VkPipelineDepthStencilStateCreateInfo depthStencilState = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
		VkPipelineViewportStateCreateInfo viewportState = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
		VkPipelineMultisampleStateCreateInfo multisampleState = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
		std::vector<VkDynamicState> dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_LINE_WIDTH };
		VkPipelineDynamicStateCreateInfo dynamicState = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
		std::array<VkPipelineShaderStageCreateInfo, 4> shaderStages{};

		// We render the terrain as a grid of quad patches
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_PATCH_LIST, 0, VK_FALSE);
		VkPipelineTessellationStateCreateInfo tessellationState = vks::initializers::pipelineTessellationStateCreateInfo(4);
		// Terrain tessellation pipeline
		shaderStages[0] = loadShader(getShadersPath() + "terraintessellation/terrain.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getShadersPath() + "terraintessellation/terrain.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		shaderStages[2] = loadShader(getShadersPath() + "terraintessellation/terrain.tesc.spv", VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);
		shaderStages[3] = loadShader(getShadersPath() + "terraintessellation/terrain.tese.spv", VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);

		VkGraphicsPipelineCreateInfo pipelineCI = vks::initializers::pipelineCreateInfo(pipelineLayouts.terrain, renderPass);
		pipelineCI.pInputAssemblyState = &inputAssemblyState;
		pipelineCI.pRasterizationState = &rasterizationState;
		pipelineCI.pColorBlendState = &colorBlendState;
		pipelineCI.pMultisampleState = &multisampleState;
		pipelineCI.pViewportState = &viewportState;
		pipelineCI.pDepthStencilState = &depthStencilState;
		pipelineCI.pDynamicState = &dynamicState;
		pipelineCI.pTessellationState = &tessellationState;
		pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineCI.pStages = shaderStages.data();
		pipelineCI.pVertexInputState = vkglTF::Vertex::getPipelineVertexInputState({ vkglTF::VertexComponent::Position, vkglTF::VertexComponent::Normal, vkglTF::VertexComponent::UV });
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.terrain));

		// Terrain wireframe pipeline (if devie supports it)
		if (deviceFeatures.fillModeNonSolid) {
			rasterizationState.polygonMode = VK_POLYGON_MODE_LINE;
			VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.wireframe));
		};

		// Skysphere pipeline
		rasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;
		rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
		// Revert to triangle list topology
		inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		// Reset tessellation state
		pipelineCI.pTessellationState = nullptr;
		// Don't write to depth buffer
		depthStencilState.depthWriteEnable = VK_FALSE;
		pipelineCI.stageCount = 2;
		pipelineCI.layout = pipelineLayouts.skysphere;
		shaderStages[0] = loadShader(getShadersPath() + "terraintessellation/skysphere.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getShadersPath() + "terraintessellation/skysphere.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.skysphere));
	}

	// Prepare and initialize uniform buffer containing shader uniforms
	void prepareUniformBuffers()
	{
		for (auto& buffer : uniformBuffers) {
			// Tessellation shader stages uniform buffer
			VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &buffer.terrainTessellation, sizeof(UniformDataTessellation)));
			VK_CHECK_RESULT(buffer.terrainTessellation.map());
			// Skysphere vertex shader uniform buffer
			VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &buffer.skysphereVertex, sizeof(UniformDataVertex)));
			VK_CHECK_RESULT(buffer.skysphereVertex.map());
		}
	}

	void updateUniformBuffers()
	{
		// Tessellation
		uniformDataTessellation.projection = camera.matrices.perspective;
		uniformDataTessellation.modelview = camera.matrices.view * glm::mat4(1.0f);
		uniformDataTessellation.lightPos.y = -0.5f - uniformDataTessellation.displacementFactor; // todo: Not uesed yet
		uniformDataTessellation.viewportDim = glm::vec2((float)width, (float)height);

		frustum.update(uniformDataTessellation.projection * uniformDataTessellation.modelview);
		memcpy(uniformDataTessellation.frustumPlanes, frustum.planes.data(), sizeof(glm::vec4) * 6);

		float savedFactor = uniformDataTessellation.tessellationFactor;
		if (!tessellation)
		{
			// Setting this to zero sets all tessellation factors to 1.0 in the shader
			uniformDataTessellation.tessellationFactor = 0.0f;
		}

		memcpy(uniformBuffers[currentBuffer].terrainTessellation.mapped, &uniformDataTessellation, sizeof(UniformDataTessellation));

		if (!tessellation)
		{
			uniformDataTessellation.tessellationFactor = savedFactor;
		}

		// Vertex shader
		uniformDataVertex.mvp = camera.matrices.perspective * glm::mat4(glm::mat3(camera.matrices.view));
		memcpy(uniformBuffers[currentBuffer].skysphereVertex.mapped, &uniformDataVertex, sizeof(UniformDataVertex));
	}

	void prepare()
	{
		VulkanExampleBase::prepare();
		loadAssets();
		generateTerrain();
		if (deviceFeatures.pipelineStatisticsQuery) {
			setupQueryResultBuffer();
		}
		prepareUniformBuffers();
		setupDescriptors();
		preparePipelines();
		prepared = true;
	}

	void buildCommandBuffer()
	{
		VkCommandBuffer cmdBuffer = drawCmdBuffers[currentBuffer];
		
		VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

		VkClearValue clearValues[2]{};
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
		renderPassBeginInfo.framebuffer = frameBuffers[currentImageIndex];

		VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));

		if (deviceFeatures.pipelineStatisticsQuery) {
			vkCmdResetQueryPool(cmdBuffer, queryPool, 0, 2);
		}

		vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
		vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

		VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);
		vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

		vkCmdSetLineWidth(cmdBuffer, 1.0f);

		VkDeviceSize offsets[1] = { 0 };

		// Skysphere
		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.skysphere);
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.skysphere, 0, 1, &descriptorSets[currentBuffer].skysphere, 0, nullptr);
		models.skysphere.draw(cmdBuffer);

		// Tessellated terrain
		if (deviceFeatures.pipelineStatisticsQuery) {
			// Begin pipeline statistics query
			vkCmdBeginQuery(cmdBuffer, queryPool, 0, 0);
		}
		// Render
		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, wireframe ? pipelines.wireframe : pipelines.terrain);
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.terrain, 0, 1, &descriptorSets[currentBuffer].terrain, 0, nullptr);
		vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &terrain.vertexBuffer.buffer, offsets);
		vkCmdBindIndexBuffer(cmdBuffer, terrain.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(cmdBuffer, terrain.indexCount, 1, 0, 0, 0);
		if (deviceFeatures.pipelineStatisticsQuery) {
			// End pipeline statistics query
			vkCmdEndQuery(cmdBuffer, queryPool, 0);
		}

		drawUI(cmdBuffer);

		vkCmdEndRenderPass(cmdBuffer);

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
		// Read query results for displaying in next frame (if the device supports pipeline statistics)
		if (deviceFeatures.pipelineStatisticsQuery) {
			// We use vkGetQueryResults to copy the results into a host visible buffer
			vkGetQueryPoolResults(
				device,
				queryPool,
				0,
				1,
				sizeof(pipelineStats),
				pipelineStats,
				sizeof(uint64_t),
				VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
		}
	}

	virtual void OnUpdateUIOverlay(vks::UIOverlay *overlay)
	{
		if (overlay->header("Settings")) {

			overlay->checkBox("Tessellation", &tessellation);
			overlay->inputFloat("Factor", &uniformDataTessellation.tessellationFactor, 0.05f, 2);
			if (deviceFeatures.fillModeNonSolid) {
				overlay->checkBox("Wireframe", &wireframe);
			}
		}
		if (deviceFeatures.pipelineStatisticsQuery) {
			if (overlay->header("Pipeline statistics")) {
				overlay->text("VS invocations: %d", pipelineStats[0]);
				overlay->text("TE invocations: %d", pipelineStats[1]);
			}
		}
	}
};

VULKAN_EXAMPLE_MAIN()
