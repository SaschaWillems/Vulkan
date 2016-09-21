/*
* Mesh loader for creating Vulkan resources from models loaded with ASSIMP
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#include <stdlib.h>
#include <string>
#include <fstream>
#include <assert.h>
#include <stdio.h>
#include <vector>
#include <map>
#ifdef _WIN32
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#else
#endif

#include "vulkan/vulkan.h"

#include <assimp/Importer.hpp> 
#include <assimp/scene.h>     
#include <assimp/postprocess.h>
#include <assimp/cimport.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "vulkandevice.hpp"

#if defined(__ANDROID__)
#include <android/asset_manager.h>
#endif

namespace vkMeshLoader 
{
	typedef enum VertexLayout {
		VERTEX_LAYOUT_POSITION = 0x0,
		VERTEX_LAYOUT_NORMAL = 0x1,
		VERTEX_LAYOUT_COLOR = 0x2,
		VERTEX_LAYOUT_UV = 0x3,
		VERTEX_LAYOUT_TANGENT = 0x4,
		VERTEX_LAYOUT_BITANGENT = 0x5,
		VERTEX_LAYOUT_DUMMY_FLOAT = 0x6,
		VERTEX_LAYOUT_DUMMY_VEC4 = 0x7
	} VertexLayout;

	struct MeshBufferInfo 
	{
		VkBuffer buf = VK_NULL_HANDLE;
		VkDeviceMemory mem = VK_NULL_HANDLE;
		size_t size = 0;
	};

	/** @brief Stores a mesh's vertex and index descriptions */
	struct MeshDescriptor
	{
		uint32_t vertexCount;
		uint32_t indexBase;
		uint32_t indexCount;
	};

	/** @brief Mesh representation storing all data required to generate buffers */
	struct MeshBuffer 
	{
		std::vector<MeshDescriptor> meshDescriptors;
		MeshBufferInfo vertices;
		MeshBufferInfo indices;
		uint32_t indexCount;
		glm::vec3 dim;
	};

	/** @brief Holds parameters for mesh creation */
	struct MeshCreateInfo
	{
		glm::vec3 center;
		glm::vec3 scale;
		glm::vec2 uvscale;
	};

	/** 
	* Get the size of a vertex layout	
	* 
	* @param layout VertexLayout to get the size for
	*
	* @return Size of the vertex layout in bytes
	*/
	static uint32_t vertexSize(std::vector<vkMeshLoader::VertexLayout> layout)
	{
		uint32_t vSize = 0;
		for (auto& layoutDetail : layout)
		{
			switch (layoutDetail)
			{
			// UV only has two components
			case VERTEX_LAYOUT_UV: 
				vSize += 2 * sizeof(float);
				break;
			default:
				vSize += 3 * sizeof(float);
			}
		}
		return vSize;
	}

	// Note: Always assumes float formats
	/**
	* Generate vertex attribute descriptions for a layout at the given binding point
	*
	* @param layout VertexLayout from which to generate the descriptions 
	* @param attributeDescriptions Refernce to a vector of the descriptions to generate
	* @param binding Index of the attribute description binding point
	*
	* @note Always assumes float formats
	*/
	static void getVertexInputAttributeDescriptions(std::vector<vkMeshLoader::VertexLayout> layout, std::vector<VkVertexInputAttributeDescription> &attributeDescriptions, uint32_t binding)
	{
		uint32_t offset = 0;
		uint32_t location = 0;
		for (auto& layoutDetail : layout)
		{
			VkVertexInputAttributeDescription inputAttribDescription = {};
			inputAttribDescription.binding = binding;
			inputAttribDescription.location = location;
			inputAttribDescription.offset = offset;

			switch (layoutDetail)
			{
			// UV only has two components
			case VERTEX_LAYOUT_UV:
				offset += 2 * sizeof(float);
				inputAttribDescription.format = VK_FORMAT_R32G32_SFLOAT;
				break;
			default:
				offset += 3 * sizeof(float);
				inputAttribDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
			}
			attributeDescriptions.push_back(inputAttribDescription);
			location++;
		}
	}

	// Stores some additonal info and functions for 
	// specifying pipelines, vertex bindings, etc.
	class Mesh
	{
	public:
		MeshBuffer buffers;

		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
		VkPipeline pipeline = VK_NULL_HANDLE;
		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

		uint32_t vertexBufferBinding = 0;

		VkPipelineVertexInputStateCreateInfo vertexInputState;
		VkVertexInputBindingDescription bindingDescription;
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;

		void setupVertexInputState(std::vector<vkMeshLoader::VertexLayout> layout)
		{
			bindingDescription = vkTools::initializers::vertexInputBindingDescription(
				vertexBufferBinding,
				vertexSize(layout),
				VK_VERTEX_INPUT_RATE_VERTEX);

			attributeDescriptions.clear();
			uint32_t offset = 0;
			uint32_t binding = 0;
			for (auto& layoutDetail : layout)
			{
				// Format (layout)
				VkFormat format = (layoutDetail == VERTEX_LAYOUT_UV) ? VK_FORMAT_R32G32_SFLOAT : VK_FORMAT_R32G32B32_SFLOAT;

				attributeDescriptions.push_back(
					vkTools::initializers::vertexInputAttributeDescription(
						vertexBufferBinding,
						binding,
						format,
						offset));

				// Offset
				offset += (layoutDetail == VERTEX_LAYOUT_UV) ? (2 * sizeof(float)) : (3 * sizeof(float));
				binding++;
			}

			vertexInputState = vkTools::initializers::pipelineVertexInputStateCreateInfo();
			vertexInputState.vertexBindingDescriptionCount = 1;
			vertexInputState.pVertexBindingDescriptions = &bindingDescription;
			vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
			vertexInputState.pVertexAttributeDescriptions = attributeDescriptions.data();
		}

		void drawIndexed(VkCommandBuffer cmdBuffer)
		{
			VkDeviceSize offsets[1] = { 0 };
			if (pipeline != VK_NULL_HANDLE)
			{
				vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
			}
			if ((pipelineLayout != VK_NULL_HANDLE) && (descriptorSet != VK_NULL_HANDLE))
			{
				vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);
			}
			vkCmdBindVertexBuffers(cmdBuffer, vertexBufferBinding, 1, &buffers.vertices.buf, offsets);
			vkCmdBindIndexBuffer(cmdBuffer, buffers.indices.buf, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(cmdBuffer, buffers.indexCount, 1, 0, 0, 0);
		}
	};

	static void freeMeshBufferResources(VkDevice device, vkMeshLoader::MeshBuffer *meshBuffer)
	{
		vkDestroyBuffer(device, meshBuffer->vertices.buf, nullptr);
		vkFreeMemory(device, meshBuffer->vertices.mem, nullptr);
		if (meshBuffer->indices.buf != VK_NULL_HANDLE)
		{
			vkDestroyBuffer(device, meshBuffer->indices.buf, nullptr);
			vkFreeMemory(device, meshBuffer->indices.mem, nullptr);
		}
	}
}

// Simple mesh class for getting all the necessary stuff from models loaded via ASSIMP
class VulkanMeshLoader 
{
private:
	vk::VulkanDevice *vulkanDevice;

	static const int defaultFlags = aiProcess_FlipWindingOrder | aiProcess_Triangulate | aiProcess_PreTransformVertices | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals;

	struct Vertex
	{
		glm::vec3 m_pos;
		glm::vec2 m_tex;
		glm::vec3 m_normal;
		glm::vec3 m_color;
		glm::vec3 m_tangent;
		glm::vec3 m_binormal;

		Vertex() {}

		Vertex(const glm::vec3& pos, const glm::vec2& tex, const glm::vec3& normal, const glm::vec3& tangent, const glm::vec3& bitangent, const glm::vec3& color)
		{
			m_pos = pos;
			m_tex = tex;
			m_normal = normal;
			m_color = color;
			m_tangent = tangent;
			m_binormal = bitangent;
		}
	};

	struct MeshEntry {
		uint32_t NumIndices;
		uint32_t MaterialIndex;
		uint32_t vertexBase;
		std::vector<Vertex> Vertices;
		std::vector<unsigned int> Indices;
	};

public:
#if defined(__ANDROID__)
	AAssetManager* assetManager = nullptr;
#endif

	std::vector<MeshEntry> m_Entries;

	struct Dimension 
	{
		glm::vec3 min = glm::vec3(FLT_MAX);
		glm::vec3 max = glm::vec3(-FLT_MAX);
		glm::vec3 size;
	} dim;

	uint32_t numVertices = 0;

	Assimp::Importer Importer;
	const aiScene* pScene;

	/**
	* Default constructor
	*
	* @param vulkanDevice Pointer to a valid VulkanDevice
	*/
	VulkanMeshLoader(vk::VulkanDevice *vulkanDevice)
	{
		assert(vulkanDevice != nullptr);
		this->vulkanDevice = vulkanDevice;
	}

	/**
	* Default destructor
	*
	* @note Does not free any Vulkan resources
	*/
	~VulkanMeshLoader()
	{
		m_Entries.clear();
	}

	/** 
	* Load a scene from a supported 3D file format
	*
	* @param filename Name of the file (or asset) to load
	* @param flags (Optional) Set of ASSIMP processing flags
	*
	* @return Returns true if the scene has been loaded
	*/
	bool LoadMesh(const std::string& filename, int flags = defaultFlags)
	{
#if defined(__ANDROID__)
		// Meshes are stored inside the apk on Android (compressed)
		// So they need to be loaded via the asset manager

		AAsset* asset = AAssetManager_open(assetManager, filename.c_str(), AASSET_MODE_STREAMING);
		assert(asset);
		size_t size = AAsset_getLength(asset);

		assert(size > 0);
		
		void *meshData = malloc(size);
		AAsset_read(asset, meshData, size);
		AAsset_close(asset);

		pScene = Importer.ReadFileFromMemory(meshData, size, flags);

		free(meshData);
#else
		pScene = Importer.ReadFile(filename.c_str(), flags);
#endif

		if (pScene)
		{
			m_Entries.clear();
			m_Entries.resize(pScene->mNumMeshes);
			// Read in all meshes in the scene
			for (auto i = 0; i < m_Entries.size(); i++)
			{
				m_Entries[i].vertexBase = numVertices;
				numVertices += pScene->mMeshes[i]->mNumVertices;
				const aiMesh* paiMesh = pScene->mMeshes[i];
				InitMesh(&m_Entries[i], paiMesh, pScene);
			}
			return true;
		}
		else 
		{
			printf("Error parsing '%s': '%s'\n", filename.c_str(), Importer.GetErrorString());
#if defined(__ANDROID__)
			LOGE("Error parsing '%s': '%s'", filename.c_str(), Importer.GetErrorString());
#endif
			return false;
		}
	}

	/**
	* Read mesh data from ASSIMP mesh to an internal mesh representation that can be used to generate Vulkan buffers
	*
	* @param meshEntry Pointer to the target MeshEntry strucutre for the mesh data
	* @param paiMesh ASSIMP mesh to get the data from
	* @param pScene Scene file of the ASSIMP mesh
	*/
	void InitMesh(MeshEntry *meshEntry, const aiMesh* paiMesh, const aiScene* pScene)
	{
		meshEntry->MaterialIndex = paiMesh->mMaterialIndex;

		aiColor3D pColor(0.f, 0.f, 0.f);
		pScene->mMaterials[paiMesh->mMaterialIndex]->Get(AI_MATKEY_COLOR_DIFFUSE, pColor);

		aiVector3D Zero3D(0.0f, 0.0f, 0.0f);

		for (unsigned int i = 0; i < paiMesh->mNumVertices; i++) 
		{
			aiVector3D* pPos = &(paiMesh->mVertices[i]);
			aiVector3D* pNormal = &(paiMesh->mNormals[i]);
			aiVector3D* pTexCoord = (paiMesh->HasTextureCoords(0)) ? &(paiMesh->mTextureCoords[0][i]) : &Zero3D;
			aiVector3D* pTangent = (paiMesh->HasTangentsAndBitangents()) ? &(paiMesh->mTangents[i]) : &Zero3D;
			aiVector3D* pBiTangent = (paiMesh->HasTangentsAndBitangents()) ? &(paiMesh->mBitangents[i]) : &Zero3D;

			Vertex v(
				glm::vec3(pPos->x, -pPos->y, pPos->z), 
				glm::vec2(pTexCoord->x , pTexCoord->y),
				glm::vec3(pNormal->x, pNormal->y, pNormal->z),
				glm::vec3(pTangent->x, pTangent->y, pTangent->z),
				glm::vec3(pBiTangent->x, pBiTangent->y, pBiTangent->z),
				glm::vec3(pColor.r, pColor.g, pColor.b)
				);
		
			dim.max.x = fmax(pPos->x, dim.max.x);
			dim.max.y = fmax(pPos->y, dim.max.y);
			dim.max.z = fmax(pPos->z, dim.max.z);

			dim.min.x = fmin(pPos->x, dim.min.x);
			dim.min.y = fmin(pPos->y, dim.min.y);
			dim.min.z = fmin(pPos->z, dim.min.z);

			meshEntry->Vertices.push_back(v);
		}

		dim.size = dim.max - dim.min;

		uint32_t indexBase = static_cast<uint32_t>(meshEntry->Indices.size());
		for (unsigned int i = 0; i < paiMesh->mNumFaces; i++)
		{
			const aiFace& Face = paiMesh->mFaces[i];
			if (Face.mNumIndices != 3)
				continue;
			meshEntry->Indices.push_back(indexBase + Face.mIndices[0]);
			meshEntry->Indices.push_back(indexBase + Face.mIndices[1]);
			meshEntry->Indices.push_back(indexBase + Face.mIndices[2]);
		}
	}

	/**
	* Create Vulkan buffers for the index and vertex buffer using a vertex layout
	*
	* @note Only does staging if a valid command buffer and transfer queue are passed
	*
	* @param meshBuffer Pointer to the mesh buffer containing buffer handles and memory
	* @param layout Vertex layout for the vertex buffer
	* @param createInfo Structure containing information for mesh creation time (center, scaling, etc.)
	* @param useStaging If true, buffers are staged to device local memory
	* @param copyCmd (Required for staging) Command buffer to put the copy commands into
	* @param copyQueue (Required for staging) Queue to put copys into
	*/
	void createBuffers(
		vkMeshLoader::MeshBuffer *meshBuffer,
		std::vector<vkMeshLoader::VertexLayout> layout,
		vkMeshLoader::MeshCreateInfo *createInfo,
		bool useStaging,
		VkCommandBuffer copyCmd,
		VkQueue copyQueue)
	{
		glm::vec3 scale;
		glm::vec2 uvscale;
		glm::vec3 center;
		if (createInfo == nullptr)
		{
			scale = glm::vec3(1.0f);
			uvscale = glm::vec2(1.0f);
			center = glm::vec3(0.0f);
		}
		else
		{
			scale = createInfo->scale;
			uvscale = createInfo->uvscale;
			center = createInfo->center;
		}

		std::vector<float> vertexBuffer;
		for (int m = 0; m < m_Entries.size(); m++)
		{
			for (int i = 0; i < m_Entries[m].Vertices.size(); i++)
			{
				// Push vertex data depending on layout
				for (auto& layoutDetail : layout)
				{
					// Position
					if (layoutDetail == vkMeshLoader::VERTEX_LAYOUT_POSITION)
					{
						vertexBuffer.push_back(m_Entries[m].Vertices[i].m_pos.x * scale.x + center.x);
						vertexBuffer.push_back(m_Entries[m].Vertices[i].m_pos.y * scale.y + center.y);
						vertexBuffer.push_back(m_Entries[m].Vertices[i].m_pos.z * scale.z + center.z);
					}
					// Normal
					if (layoutDetail == vkMeshLoader::VERTEX_LAYOUT_NORMAL)
					{
						vertexBuffer.push_back(m_Entries[m].Vertices[i].m_normal.x);
						vertexBuffer.push_back(-m_Entries[m].Vertices[i].m_normal.y);
						vertexBuffer.push_back(m_Entries[m].Vertices[i].m_normal.z);
					}
					// Texture coordinates
					if (layoutDetail == vkMeshLoader::VERTEX_LAYOUT_UV)
					{
						vertexBuffer.push_back(m_Entries[m].Vertices[i].m_tex.s * uvscale.s);
						vertexBuffer.push_back(m_Entries[m].Vertices[i].m_tex.t * uvscale.t);
					}
					// Color
					if (layoutDetail == vkMeshLoader::VERTEX_LAYOUT_COLOR)
					{
						vertexBuffer.push_back(m_Entries[m].Vertices[i].m_color.r);
						vertexBuffer.push_back(m_Entries[m].Vertices[i].m_color.g);
						vertexBuffer.push_back(m_Entries[m].Vertices[i].m_color.b);
					}
					// Tangent
					if (layoutDetail == vkMeshLoader::VERTEX_LAYOUT_TANGENT)
					{
						vertexBuffer.push_back(m_Entries[m].Vertices[i].m_tangent.x);
						vertexBuffer.push_back(m_Entries[m].Vertices[i].m_tangent.y);
						vertexBuffer.push_back(m_Entries[m].Vertices[i].m_tangent.z);
					}
					// Bitangent
					if (layoutDetail == vkMeshLoader::VERTEX_LAYOUT_BITANGENT)
					{
						vertexBuffer.push_back(m_Entries[m].Vertices[i].m_binormal.x);
						vertexBuffer.push_back(m_Entries[m].Vertices[i].m_binormal.y);
						vertexBuffer.push_back(m_Entries[m].Vertices[i].m_binormal.z);
					}
					// Dummy layout components for padding
					if (layoutDetail == vkMeshLoader::VERTEX_LAYOUT_DUMMY_FLOAT)
					{
						vertexBuffer.push_back(0.0f);
					}
					if (layoutDetail == vkMeshLoader::VERTEX_LAYOUT_DUMMY_VEC4)
					{
						vertexBuffer.push_back(0.0f);
						vertexBuffer.push_back(0.0f);
						vertexBuffer.push_back(0.0f);
						vertexBuffer.push_back(0.0f);
					}
				}
			}
		}
		meshBuffer->vertices.size = vertexBuffer.size() * sizeof(float);

		dim.min *= scale;
		dim.max *= scale;
		dim.size *= scale;

		std::vector<uint32_t> indexBuffer;
		for (uint32_t m = 0; m < m_Entries.size(); m++)
		{
			uint32_t indexBase = static_cast<uint32_t>(indexBuffer.size());
			for (uint32_t i = 0; i < m_Entries[m].Indices.size(); i++)
			{
				indexBuffer.push_back(m_Entries[m].Indices[i] + indexBase);
			}
			vkMeshLoader::MeshDescriptor descriptor{};
			descriptor.indexBase = indexBase;
			descriptor.indexCount = static_cast<uint32_t>(m_Entries[m].Indices.size());
			descriptor.vertexCount = static_cast<uint32_t>(m_Entries[m].Vertices.size());
			meshBuffer->meshDescriptors.push_back(descriptor);
		}
		meshBuffer->indices.size = indexBuffer.size() * sizeof(uint32_t);
		meshBuffer->indexCount = static_cast<uint32_t>(indexBuffer.size());

		// Use staging buffer to move vertex and index buffer to device local memory
		if (useStaging && copyQueue != VK_NULL_HANDLE && copyCmd != VK_NULL_HANDLE)
		{
			// Create staging buffers
			struct {
				VkBuffer buffer;
				VkDeviceMemory memory;
			} vertexStaging, indexStaging;

			// Vertex buffer
			vulkanDevice->createBuffer(
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
				meshBuffer->vertices.size,
				&vertexStaging.buffer,
				&vertexStaging.memory,
				vertexBuffer.data());

			// Index buffer
			vulkanDevice->createBuffer(
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
				meshBuffer->indices.size,
				&indexStaging.buffer,
				&indexStaging.memory,
				indexBuffer.data());

			// Create device local target buffers
			// Vertex buffer
			vulkanDevice->createBuffer(
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				meshBuffer->vertices.size,
				&meshBuffer->vertices.buf,
				&meshBuffer->vertices.mem);

			// Index buffer
			vulkanDevice->createBuffer(
				VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				meshBuffer->indices.size,
				&meshBuffer->indices.buf,
				&meshBuffer->indices.mem);

			// Copy from staging buffers
			VkCommandBufferBeginInfo cmdBufInfo = vkTools::initializers::commandBufferBeginInfo();
			VK_CHECK_RESULT(vkBeginCommandBuffer(copyCmd, &cmdBufInfo));

			VkBufferCopy copyRegion = {};

			copyRegion.size = meshBuffer->vertices.size;
			vkCmdCopyBuffer(
				copyCmd,
				vertexStaging.buffer,
				meshBuffer->vertices.buf,
				1,
				&copyRegion);

			copyRegion.size = meshBuffer->indices.size;
			vkCmdCopyBuffer(
				copyCmd,
				indexStaging.buffer,
				meshBuffer->indices.buf,
				1,
				&copyRegion);

			VK_CHECK_RESULT(vkEndCommandBuffer(copyCmd));

			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &copyCmd;

			VK_CHECK_RESULT(vkQueueSubmit(copyQueue, 1, &submitInfo, VK_NULL_HANDLE));
			VK_CHECK_RESULT(vkQueueWaitIdle(copyQueue));

			vkDestroyBuffer(vulkanDevice->logicalDevice, vertexStaging.buffer, nullptr);
			vkFreeMemory(vulkanDevice->logicalDevice, vertexStaging.memory, nullptr);
			vkDestroyBuffer(vulkanDevice->logicalDevice, indexStaging.buffer, nullptr);
			vkFreeMemory(vulkanDevice->logicalDevice, indexStaging.memory, nullptr);
		}
		else
		{
			// Generate vertex buffer
			vulkanDevice->createBuffer(
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
				meshBuffer->vertices.size,
				&meshBuffer->vertices.buf,
				&meshBuffer->vertices.mem,
				vertexBuffer.data());

			// Generate index buffer
			vulkanDevice->createBuffer(
				VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
				meshBuffer->indices.size,
				&meshBuffer->indices.buf,
				&meshBuffer->indices.mem,
				indexBuffer.data());
		}
	}
};