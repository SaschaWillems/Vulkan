/*
* Vulkan Example - Dynamic terrain tessellation
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "vulkanexamplebase.h"
#include "VulkanglTFModel.h"
#include "frustum.hpp"
#include <ktx.h>
#include <ktxvulkan.h>

#define ENABLE_VALIDATION false

class VulkanExample : public VulkanExampleBase
{
public:
	bool wireframe = false;
	bool tessellation = true;

	// Holds the buffers for rendering the tessellated terrain
	struct {
		struct Vertices {
			VkBuffer buffer;
			VkDeviceMemory memory;
		} vertices;
		struct Indices {
			int count;
			VkBuffer buffer;
			VkDeviceMemory memory;
		} indices;
	} terrain;

	struct {
		vks::Texture2D heightMap;
		vks::Texture2D skySphere;
		vks::Texture2DArray terrainArray;
	} textures;

	struct {
		vkglTF::Model skysphere;
	} models;

	struct {
		vks::Buffer terrainTessellation;
		vks::Buffer skysphereVertex;
	} uniformBuffers;

	// Shared values for tessellation control and evaluation stages
	struct {
		glm::mat4 projection;
		glm::mat4 modelview;
		glm::vec4 lightPos = glm::vec4(-48.0f, -40.0f, 46.0f, 0.0f);
		glm::vec4 frustumPlanes[6];
		float displacementFactor = 32.0f;
		float tessellationFactor = 0.75f;
		glm::vec2 viewportDim;
		// Desired size of tessellated quad patch edge
		float tessellatedEdgeSize = 20.0f;
	} uboTess;

	// Skysphere vertex shader stage
	struct {
		glm::mat4 mvp;
	} uboVS;

	struct Pipelines {
		VkPipeline terrain;
		VkPipeline wireframe = VK_NULL_HANDLE;
		VkPipeline skysphere;
	} pipelines;

	struct {
		VkDescriptorSetLayout terrain;
		VkDescriptorSetLayout skysphere;
	} descriptorSetLayouts;

	struct {
		VkPipelineLayout terrain;
		VkPipelineLayout skysphere;
	} pipelineLayouts;

	struct {
		VkDescriptorSet terrain;
		VkDescriptorSet skysphere;
	} descriptorSets;

	// Pipeline statistics
	struct {
		VkBuffer buffer;
		VkDeviceMemory memory;
	} queryResult;
	VkQueryPool queryPool = VK_NULL_HANDLE;
	uint64_t pipelineStats[2] = { 0 };

	// View frustum passed to tessellation control shader for culling
	vks::Frustum frustum;

	VulkanExample() : VulkanExampleBase(ENABLE_VALIDATION)
	{
		title = "Dynamic terrain tessellation";
		camera.type = Camera::CameraType::firstperson;
		camera.setPerspective(60.0f, (float)width / (float)height, 0.1f, 512.0f);
		camera.setRotation(glm::vec3(-12.0f, 159.0f, 0.0f));
		camera.setTranslation(glm::vec3(18.0f, 22.5f, 57.5f));
		camera.movementSpeed = 7.5f;
	}

	~VulkanExample()
	{
		// Clean up used Vulkan resources
		// Note : Inherited destructor cleans up resources stored in base class
		vkDestroyPipeline(device, pipelines.terrain, nullptr);
		if (pipelines.wireframe != VK_NULL_HANDLE) {
			vkDestroyPipeline(device, pipelines.wireframe, nullptr);
		}
		vkDestroyPipeline(device, pipelines.skysphere, nullptr);

		vkDestroyPipelineLayout(device, pipelineLayouts.skysphere, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayouts.terrain, nullptr);

		vkDestroyDescriptorSetLayout(device, descriptorSetLayouts.terrain, nullptr);
		vkDestroyDescriptorSetLayout(device, descriptorSetLayouts.skysphere, nullptr);

		uniformBuffers.skysphereVertex.destroy();
		uniformBuffers.terrainTessellation.destroy();

		textures.heightMap.destroy();
		textures.skySphere.destroy();
		textures.terrainArray.destroy();

		vkDestroyBuffer(device, terrain.vertices.buffer, nullptr);
		vkFreeMemory(device, terrain.vertices.memory, nullptr);
		vkDestroyBuffer(device, terrain.indices.buffer, nullptr);
		vkFreeMemory(device, terrain.indices.memory, nullptr);

		if (queryPool != VK_NULL_HANDLE) {
			vkDestroyQueryPool(device, queryPool, nullptr);
			vkDestroyBuffer(device, queryResult.buffer, nullptr);
			vkFreeMemory(device, queryResult.memory, nullptr);
		}
	}

	// Enable physical device features required for this example
	virtual void getEnabledFeatures()
	{
		// Tessellation shader support is required for this example
		if (deviceFeatures.tessellationShader) {
			enabledFeatures.tessellationShader = VK_TRUE;
		}
		else {
			vks::tools::exitFatal("Selected GPU does not support tessellation shaders!", VK_ERROR_FEATURE_NOT_PRESENT);
		}
		// Fill mode non solid is required for wireframe display
		if (deviceFeatures.fillModeNonSolid) {
			enabledFeatures.fillModeNonSolid = VK_TRUE;
		};
		// Pipeline statistics
		if (deviceFeatures.pipelineStatisticsQuery) {
			enabledFeatures.pipelineStatisticsQuery = VK_TRUE;
		};
		// Enable anisotropic filtering if supported
		if (deviceFeatures.samplerAnisotropy) {
			enabledFeatures.samplerAnisotropy = VK_TRUE;
		}
		// Enable texture compression
		if (deviceFeatures.textureCompressionBC) {
			enabledFeatures.textureCompressionBC = VK_TRUE;
		}
		else if (deviceFeatures.textureCompressionASTC_LDR) {
			enabledFeatures.textureCompressionASTC_LDR = VK_TRUE;
		}
		else if (deviceFeatures.textureCompressionETC2) {
			enabledFeatures.textureCompressionETC2 = VK_TRUE;
		}
	}

	// Setup pool and buffer for storing pipeline statistics results
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

	// Retrieves the results of the pipeline statistics query submitted to the command buffer
	void getQueryResults()
	{
		// We use vkGetQueryResults to copy the results into a host visible buffer
		vkGetQueryPoolResults(
			device,
			queryPool,
			0,
			1,
			sizeof(pipelineStats),
			pipelineStats,
			sizeof(uint64_t),
			VK_QUERY_RESULT_64_BIT);
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
		if (deviceFeatures.samplerAnisotropy)
		{
			samplerInfo.maxAnisotropy = 4.0f;
			samplerInfo.anisotropyEnable = VK_TRUE;
		}
		VK_CHECK_RESULT(vkCreateSampler(device, &samplerInfo, nullptr, &textures.terrainArray.sampler));
		textures.terrainArray.descriptor.sampler = textures.terrainArray.sampler;
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

			if (deviceFeatures.pipelineStatisticsQuery) {
				vkCmdResetQueryPool(drawCmdBuffers[i], queryPool, 0, 2);
			}

			vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
			vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

			VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);
			vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

			vkCmdSetLineWidth(drawCmdBuffers[i], 1.0f);

			VkDeviceSize offsets[1] = { 0 };

			// Skysphere
			vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.skysphere);
			vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.skysphere, 0, 1, &descriptorSets.skysphere, 0, nullptr);
			models.skysphere.draw(drawCmdBuffers[i]);

			// Tessellated terrain
			if (deviceFeatures.pipelineStatisticsQuery) {
				// Begin pipeline statistics query
				vkCmdBeginQuery(drawCmdBuffers[i], queryPool, 0, 0);
			}
			// Render
			vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, wireframe ? pipelines.wireframe : pipelines.terrain);
			vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.terrain, 0, 1, &descriptorSets.terrain, 0, nullptr);
			vkCmdBindVertexBuffers(drawCmdBuffers[i], 0, 1, &terrain.vertices.buffer, offsets);
			vkCmdBindIndexBuffer(drawCmdBuffers[i], terrain.indices.buffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(drawCmdBuffers[i], terrain.indices.count, 1, 0, 0, 0);
			if (deviceFeatures.pipelineStatisticsQuery) {
				// End pipeline statistics query
				vkCmdEndQuery(drawCmdBuffers[i], queryPool, 0);
			}

			drawUI(drawCmdBuffers[i]);

			vkCmdEndRenderPass(drawCmdBuffers[i]);

			VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
		}
	}

	// Encapsulate height map data for easy sampling
	struct HeightMap
	{
	private:
		uint16_t *heightdata;
		uint32_t dim;
		uint32_t scale;
	public:
#if defined(__ANDROID__)
		HeightMap(std::string filename, uint32_t patchsize, AAssetManager* assetManager)
#else
		HeightMap(std::string filename, uint32_t patchsize)
#endif
		{
			ktxResult result;
			ktxTexture* ktxTexture;
#if defined(__ANDROID__)
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
			this->scale = dim / patchsize;
			ktxTexture_Destroy(ktxTexture);
		};

		~HeightMap()
		{
			delete[] heightdata;
		}

		float getHeight(uint32_t x, uint32_t y)
		{
			glm::ivec2 rpos = glm::ivec2(x, y) * glm::ivec2(scale);
			rpos.x = std::max(0, std::min(rpos.x, (int)dim-1));
			rpos.y = std::max(0, std::min(rpos.y, (int)dim-1));
			rpos /= glm::ivec2(scale);
			return *(heightdata + (rpos.x + rpos.y * dim) * scale) / 65535.0f;
		}
	};

	// Generate a terrain quad patch for feeding to the tessellation control shader
	void generateTerrain()
	{
		#define PATCH_SIZE 64
		#define UV_SCALE 1.0f

		const uint32_t vertexCount = PATCH_SIZE * PATCH_SIZE;
		// We use the Vertex definition from the glTF model loader, so we can re-use the vertex input state
		vkglTF::Vertex *vertices = new vkglTF::Vertex[vertexCount];

		const float wx = 2.0f;
		const float wy = 2.0f;

		for (auto x = 0; x < PATCH_SIZE; x++)
		{
			for (auto y = 0; y < PATCH_SIZE; y++)
			{
				uint32_t index = (x + y * PATCH_SIZE);
				vertices[index].pos[0] = x * wx + wx / 2.0f - (float)PATCH_SIZE * wx / 2.0f;
				vertices[index].pos[1] = 0.0f;
				vertices[index].pos[2] = y * wy + wy / 2.0f - (float)PATCH_SIZE * wy / 2.0f;
				vertices[index].uv = glm::vec2((float)x / PATCH_SIZE, (float)y / PATCH_SIZE) * UV_SCALE;
			}
		}

		// Calculate normals from height map using a sobel filter
#if defined(__ANDROID__)
		HeightMap heightMap(getAssetPath() + "textures/terrain_heightmap_r16.ktx", PATCH_SIZE, androidApp->activity->assetManager);
#else
		HeightMap heightMap(getAssetPath() + "textures/terrain_heightmap_r16.ktx", PATCH_SIZE);
#endif
		for (auto x = 0; x < PATCH_SIZE; x++)
		{
			for (auto y = 0; y < PATCH_SIZE; y++)
			{
				// Get height samples centered around current position
				float heights[3][3];
				for (auto hx = -1; hx <= 1; hx++)
				{
					for (auto hy = -1; hy <= 1; hy++)
					{
						heights[hx+1][hy+1] = heightMap.getHeight(x + hx, y + hy);
					}
				}

				// Calculate the normal
				glm::vec3 normal;
				// Gx sobel filter
				normal.x = heights[0][0] - heights[2][0] + 2.0f * heights[0][1] - 2.0f * heights[2][1] + heights[0][2] - heights[2][2];
				// Gy sobel filter
				normal.z = heights[0][0] + 2.0f * heights[1][0] + heights[2][0] - heights[0][2] - 2.0f * heights[1][2] - heights[2][2];
				// Calculate missing up component of the normal using the filtered x and y axis
				// The first value controls the bump strength
				normal.y = 0.25f * sqrt( 1.0f - normal.x * normal.x - normal.z * normal.z);

				vertices[x + y * PATCH_SIZE].normal = glm::normalize(normal * glm::vec3(2.0f, 1.0f, 2.0f));
			}
		}

		// Indices
		const uint32_t w = (PATCH_SIZE - 1);
		const uint32_t indexCount = w * w * 4;
		uint32_t *indices = new uint32_t[indexCount];
		for (auto x = 0; x < w; x++)
		{
			for (auto y = 0; y < w; y++)
			{
				uint32_t index = (x + y * w) * 4;
				indices[index] = (x + y * PATCH_SIZE);
				indices[index + 1] = indices[index] + PATCH_SIZE;
				indices[index + 2] = indices[index + 1] + 1;
				indices[index + 3] = indices[index] + 1;
			}
		}
		terrain.indices.count = indexCount;

		uint32_t vertexBufferSize = vertexCount * sizeof(vkglTF::Vertex);
		uint32_t indexBufferSize = indexCount * sizeof(uint32_t);

		struct {
			VkBuffer buffer;
			VkDeviceMemory memory;
		} vertexStaging, indexStaging;

		// Create staging buffers

		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			vertexBufferSize,
			&vertexStaging.buffer,
			&vertexStaging.memory,
			vertices));

		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			indexBufferSize,
			&indexStaging.buffer,
			&indexStaging.memory,
			indices));

		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			vertexBufferSize,
			&terrain.vertices.buffer,
			&terrain.vertices.memory));

		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			indexBufferSize,
			&terrain.indices.buffer,
			&terrain.indices.memory));

		// Copy from staging buffers
		VkCommandBuffer copyCmd = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

		VkBufferCopy copyRegion = {};

		copyRegion.size = vertexBufferSize;
		vkCmdCopyBuffer(
			copyCmd,
			vertexStaging.buffer,
			terrain.vertices.buffer,
			1,
			&copyRegion);

		copyRegion.size = indexBufferSize;
		vkCmdCopyBuffer(
			copyCmd,
			indexStaging.buffer,
			terrain.indices.buffer,
			1,
			&copyRegion);

		vulkanDevice->flushCommandBuffer(copyCmd, queue, true);

		vkDestroyBuffer(device, vertexStaging.buffer, nullptr);
		vkFreeMemory(device, vertexStaging.memory, nullptr);
		vkDestroyBuffer(device, indexStaging.buffer, nullptr);
		vkFreeMemory(device, indexStaging.memory, nullptr);

		delete[] vertices;
		delete[] indices;
	}

	void setupDescriptorPool()
	{
		std::vector<VkDescriptorPoolSize> poolSizes =
		{
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3),
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3)
		};

		VkDescriptorPoolCreateInfo descriptorPoolInfo =
			vks::initializers::descriptorPoolCreateInfo(
				static_cast<uint32_t>(poolSizes.size()),
				poolSizes.data(),
				2);

		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));
	}

	void setupDescriptorSetLayouts()
	{
		VkDescriptorSetLayoutCreateInfo descriptorLayout;
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings;

		// Terrain
		setLayoutBindings =
		{
			// Binding 0 : Shared Tessellation shader ubo
			vks::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
				0),
			// Binding 1 : Height map
			vks::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				1),
			// Binding 3 : Terrain texture array layers
			vks::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_SHADER_STAGE_FRAGMENT_BIT,
				2),
		};

		descriptorLayout = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings.data(), static_cast<uint32_t>(setLayoutBindings.size()));
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayouts.terrain));
		pipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(&descriptorSetLayouts.terrain, 1);
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayouts.terrain));

		// Skysphere
		setLayoutBindings =
		{
			// Binding 0 : Vertex shader ubo
			vks::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				VK_SHADER_STAGE_VERTEX_BIT,
				0),
			// Binding 1 : Color map
			vks::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_SHADER_STAGE_FRAGMENT_BIT,
				1),
		};

		descriptorLayout = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings.data(), static_cast<uint32_t>(setLayoutBindings.size()));
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayouts.skysphere));
		pipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(&descriptorSetLayouts.skysphere, 1);
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayouts.skysphere));
	}

	void setupDescriptorSets()
	{
		VkDescriptorSetAllocateInfo allocInfo;
		std::vector<VkWriteDescriptorSet> writeDescriptorSets;

		// Terrain
		allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayouts.terrain, 1);
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.terrain));

		writeDescriptorSets =
		{
			// Binding 0 : Shared tessellation shader ubo
			vks::initializers::writeDescriptorSet(
				descriptorSets.terrain,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				0,
				&uniformBuffers.terrainTessellation.descriptor),
			// Binding 1 : Displacement map
			vks::initializers::writeDescriptorSet(
				descriptorSets.terrain,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				1,
				&textures.heightMap.descriptor),
			// Binding 2 : Color map (alpha channel)
			vks::initializers::writeDescriptorSet(
				descriptorSets.terrain,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				2,
				&textures.terrainArray.descriptor),
		};
		vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, NULL);

		// Skysphere
		allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayouts.skysphere, 1);
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.skysphere));

		writeDescriptorSets =
		{
			// Binding 0 : Vertex shader ubo
			vks::initializers::writeDescriptorSet(
				descriptorSets.skysphere,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				0,
				&uniformBuffers.skysphereVertex.descriptor),
			// Binding 1 : Fragment shader color map
			vks::initializers::writeDescriptorSet(
				descriptorSets.skysphere,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				1,
				&textures.skySphere.descriptor),
		};
		vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, NULL);
	}

	void preparePipelines()
	{
		VkPipelineRasterizationStateCreateInfo rasterizationState = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
		VkPipelineColorBlendAttachmentState blendAttachmentState = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
		VkPipelineColorBlendStateCreateInfo colorBlendState = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
		VkPipelineDepthStencilStateCreateInfo depthStencilState = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
		VkPipelineViewportStateCreateInfo viewportState = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
		VkPipelineMultisampleStateCreateInfo multisampleState = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
		std::vector<VkDynamicState> dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_LINE_WIDTH };
		VkPipelineDynamicStateCreateInfo dynamicState = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
		std::array<VkPipelineShaderStageCreateInfo, 4> shaderStages;

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

		// Terrain wireframe pipeline
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
		// Shared tessellation shader stages uniform buffer
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&uniformBuffers.terrainTessellation,
			sizeof(uboTess)));

		// Skysphere vertex shader uniform buffer
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&uniformBuffers.skysphereVertex,
			sizeof(uboVS)));

		// Map persistent
		VK_CHECK_RESULT(uniformBuffers.terrainTessellation.map());
		VK_CHECK_RESULT(uniformBuffers.skysphereVertex.map());

		updateUniformBuffers();
	}

	void updateUniformBuffers()
	{
		// Tessellation

		uboTess.projection = camera.matrices.perspective;
		uboTess.modelview = camera.matrices.view * glm::mat4(1.0f);
		uboTess.lightPos.y = -0.5f - uboTess.displacementFactor; // todo: Not uesed yet
		uboTess.viewportDim = glm::vec2((float)width, (float)height);

		frustum.update(uboTess.projection * uboTess.modelview);
		memcpy(uboTess.frustumPlanes, frustum.planes.data(), sizeof(glm::vec4) * 6);

		float savedFactor = uboTess.tessellationFactor;
		if (!tessellation)
		{
			// Setting this to zero sets all tessellation factors to 1.0 in the shader
			uboTess.tessellationFactor = 0.0f;
		}

		memcpy(uniformBuffers.terrainTessellation.mapped, &uboTess, sizeof(uboTess));

		if (!tessellation)
		{
			uboTess.tessellationFactor = savedFactor;
		}

		// Skysphere vertex shader
		uboVS.mvp = camera.matrices.perspective * glm::mat4(glm::mat3(camera.matrices.view));
		memcpy(uniformBuffers.skysphereVertex.mapped, &uboVS, sizeof(uboVS));
	}

	void draw()
	{
		VulkanExampleBase::prepareFrame();

		// Command buffer to be submitted to the queue
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];

		// Submit to queue
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

		if (deviceFeatures.pipelineStatisticsQuery) {
			// Read query results for displaying in next frame
			getQueryResults();
		}

		VulkanExampleBase::submitFrame();
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
		setupDescriptorSetLayouts();
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

			if (overlay->checkBox("Tessellation", &tessellation)) {
				updateUniformBuffers();
			}
			if (overlay->inputFloat("Factor", &uboTess.tessellationFactor, 0.05f, 2)) {
				updateUniformBuffers();
			}
			if (deviceFeatures.fillModeNonSolid) {
				if (overlay->checkBox("Wireframe", &wireframe)) {
					buildCommandBuffers();
				}
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