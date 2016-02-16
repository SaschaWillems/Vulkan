/*
* Simple wrapper for getting an index buffer and vertices out of an assimp mesh
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

namespace vkMeshLoader 
{
	typedef enum VertexLayout {
		VERTEX_LAYOUT_POSITION = 0x0,
		VERTEX_LAYOUT_NORMAL = 0x1,
		VERTEX_LAYOUT_COLOR = 0x2,
		VERTEX_LAYOUT_UV = 0x3,
		VERTEX_LAYOUT_TANGENT = 0x4,
		VERTEX_LAYOUT_BITANGENT = 0x5
	} VertexLayout;

	struct MeshBufferInfo 
	{
		VkBuffer buf = VK_NULL_HANDLE;
		VkDeviceMemory mem = VK_NULL_HANDLE;
	};

	struct MeshBuffer 
	{
		MeshBufferInfo vertices;
		MeshBufferInfo indices;
		uint32_t indexCount;
	};

	// Get vertex size from vertex layout
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
class VulkanMeshLoader {
private:

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


	VkBool32 getMemoryType(VkPhysicalDeviceMemoryProperties deviceMemoryProperties, uint32_t typeBits, VkFlags properties, uint32_t * typeIndex)
	{
		for (int i = 0; i < 32; i++)
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

public:
	std::vector<MeshEntry> m_Entries;

	struct Dimension 
	{
		glm::vec3 min = glm::vec3(FLT_MAX);
		glm::vec3 max = glm::vec3(-FLT_MAX);
		glm::vec3 size;
	} dim;

	uint32_t numVertices = 0;

	// Optional
	struct
	{
		VkBuffer buf;
		VkDeviceMemory mem;
	} vertexBuffer;

	struct {
		VkBuffer buf;
		VkDeviceMemory mem;
		uint32_t count;
	} indexBuffer;

	VkPipelineVertexInputStateCreateInfo vi;
	std::vector<VkVertexInputBindingDescription> bindingDescriptions;
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
	VkPipeline pipeline;

	Assimp::Importer Importer;
	const aiScene* pScene;

	~VulkanMeshLoader()
	{
		m_Entries.clear();
	}

	// Loads the mesh with some default flags
	bool LoadMesh(const std::string& Filename) 
	{
		int flags = aiProcess_FlipWindingOrder | aiProcess_Triangulate | aiProcess_PreTransformVertices | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals;

		return LoadMesh(Filename, flags);
	}

	// Load the mesh with custom flags
	bool LoadMesh(const std::string& Filename, int flags)
	{
		pScene = Importer.ReadFile(Filename.c_str(), flags);

		if (pScene)
		{
			return InitFromScene(pScene, Filename);
		}
		else 
		{
			printf("Error parsing '%s': '%s'\n", Filename.c_str(), Importer.GetErrorString());
			return false;
		}
	}

	bool InitFromScene(const aiScene* pScene, const std::string& Filename)
	{
		m_Entries.resize(pScene->mNumMeshes);

		// Counters
		for (unsigned int i = 0; i < m_Entries.size(); i++)
		{
			m_Entries[i].vertexBase = numVertices;
			numVertices += pScene->mMeshes[i]->mNumVertices;;
		}

		// Initialize the meshes in the scene one by one
		for (unsigned int i = 0; i < m_Entries.size(); i++) 
		{
			const aiMesh* paiMesh = pScene->mMeshes[i];
			InitMesh(i, paiMesh, pScene);
		}

		return true;
	}

	void InitMesh(unsigned int index, const aiMesh* paiMesh, const aiScene* pScene)
	{
		m_Entries[index].MaterialIndex = paiMesh->mMaterialIndex;

		aiColor3D pColor(0.f, 0.f, 0.f);
		pScene->mMaterials[paiMesh->mMaterialIndex]->Get(AI_MATKEY_COLOR_DIFFUSE, pColor);

		aiVector3D Zero3D(0.0f, 0.0f, 0.0f);

		for (unsigned int i = 0; i < paiMesh->mNumVertices; i++) {
			aiVector3D* pPos = &(paiMesh->mVertices[i]);
			aiVector3D* pNormal = &(paiMesh->mNormals[i]);
			aiVector3D *pTexCoord;
			if (paiMesh->HasTextureCoords(0))
			{
				pTexCoord = &(paiMesh->mTextureCoords[0][i]);
			}
			else {
				pTexCoord = &Zero3D;
			}
			aiVector3D* pTangent = (paiMesh->HasTangentsAndBitangents()) ? &(paiMesh->mTangents[i]) : &Zero3D;
			aiVector3D* pBiTangent = (paiMesh->HasTangentsAndBitangents()) ? &(paiMesh->mBitangents[i]) : &Zero3D;

			Vertex v(glm::vec3(pPos->x, -pPos->y, pPos->z), 
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

			m_Entries[index].Vertices.push_back(v);
		}

		dim.size = dim.max - dim.min;

		for (unsigned int i = 0; i < paiMesh->mNumFaces; i++) 
		{
			const aiFace& Face = paiMesh->mFaces[i];
			assert(Face.mNumIndices == 3);
			m_Entries[index].Indices.push_back(Face.mIndices[0]);
			m_Entries[index].Indices.push_back(Face.mIndices[1]);
			m_Entries[index].Indices.push_back(Face.mIndices[2]);
		}
	}

	// Clean up vulkan resources used by a mesh
	static void freeVulkanResources(VkDevice device, VulkanMeshLoader *mesh)
	{
		vkDestroyBuffer(device, mesh->vertexBuffer.buf, nullptr);
		vkFreeMemory(device, mesh->vertexBuffer.mem, nullptr);
		vkDestroyBuffer(device, mesh->indexBuffer.buf, nullptr);
		vkFreeMemory(device, mesh->indexBuffer.mem, nullptr);
	}

	// Create vertex and index buffer with given layout
	void createVulkanBuffers(
		VkDevice device, 
		VkPhysicalDeviceMemoryProperties deviceMemoryProperties,
		vkMeshLoader::MeshBuffer *meshBuffer, 
		std::vector<vkMeshLoader::VertexLayout> layout, 
		float scale)
	{

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
						vertexBuffer.push_back(m_Entries[m].Vertices[i].m_pos.x * scale);
						vertexBuffer.push_back(m_Entries[m].Vertices[i].m_pos.y * scale);
						vertexBuffer.push_back(m_Entries[m].Vertices[i].m_pos.z * scale);
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
						vertexBuffer.push_back(m_Entries[m].Vertices[i].m_tex.s);
						vertexBuffer.push_back(m_Entries[m].Vertices[i].m_tex.t);
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
					// todo : add checks if vertex component exists
				}
			}
		}
		size_t vertexBufferSize = vertexBuffer.size() * sizeof(float);

		std::vector<uint32_t> indexBuffer;
		for (uint32_t m = 0; m < m_Entries.size(); m++)
		{
			uint32_t indexBase = (uint32_t)indexBuffer.size();
			for (uint32_t i = 0; i < m_Entries[m].Indices.size(); i++) 
			{
				indexBuffer.push_back(m_Entries[m].Indices[i] + indexBase);
			}
		}
		size_t indexBufferSize = indexBuffer.size() * sizeof(uint32_t);

		VkMemoryAllocateInfo memAlloc = vkTools::initializers::memoryAllocateInfo();
		VkMemoryRequirements memReqs;

		VkResult err;
		void *data;

		// Generate vertex buffer
		VkBufferCreateInfo vBufferInfo = vkTools::initializers::bufferCreateInfo(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertexBufferSize);
		err = vkCreateBuffer(device, &vBufferInfo, nullptr, &meshBuffer->vertices.buf);
		assert(!err);
		vkGetBufferMemoryRequirements(device, meshBuffer->vertices.buf, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		getMemoryType(deviceMemoryProperties, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memAlloc.memoryTypeIndex);
		err = vkAllocateMemory(device, &memAlloc, nullptr, &meshBuffer->vertices.mem);
		assert(!err);
		err = vkMapMemory(device, meshBuffer->vertices.mem, 0, vertexBufferSize, 0, &data);
		assert(!err);
		memcpy(data, vertexBuffer.data(), vertexBufferSize);
		vkUnmapMemory(device, meshBuffer->vertices.mem);
		err = vkBindBufferMemory(device, meshBuffer->vertices.buf, meshBuffer->vertices.mem, 0);
		assert(!err);

		// Generate index buffer
		VkBufferCreateInfo iBufferInfo = vkTools::initializers::bufferCreateInfo(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, indexBufferSize);
		err = vkCreateBuffer(device, &iBufferInfo, nullptr, &meshBuffer->indices.buf);
		assert(!err);
		vkGetBufferMemoryRequirements(device, meshBuffer->indices.buf, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		getMemoryType(deviceMemoryProperties, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memAlloc.memoryTypeIndex);
		err = vkAllocateMemory(device, &memAlloc, nullptr, &meshBuffer->indices.mem);
		assert(!err);
		err = vkMapMemory(device, meshBuffer->indices.mem, 0, indexBufferSize, 0, &data);
		assert(!err);
		memcpy(data, indexBuffer.data(), indexBufferSize);
		vkUnmapMemory(device, meshBuffer->indices.mem);
		err = vkBindBufferMemory(device, meshBuffer->indices.buf, meshBuffer->indices.mem, 0);
		assert(!err);
		meshBuffer->indexCount = (uint32_t)indexBuffer.size();
	}
};