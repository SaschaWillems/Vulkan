/*
* Vulkan Model loader using ASSIMP
*
* Copyright(C) 2016-2017 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license(MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#include <stdlib.h>
#include <string>
#include <fstream>
#include <vector>

#include "vulkan/vulkan.h"

#include <assimp/Importer.hpp> 
#include <assimp/scene.h>     
#include <assimp/postprocess.h>
#include <assimp/cimport.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "VulkanDevice.hpp"
#include "VulkanBuffer.hpp"

#if defined(__ANDROID__)
#include <android/asset_manager.h>
#endif

namespace vks
{
	/** @brief Vertex layout components */
	typedef enum Component {
		VERTEX_COMPONENT_POSITION = 0x0,
		VERTEX_COMPONENT_NORMAL = 0x1,
		VERTEX_COMPONENT_COLOR = 0x2,
		VERTEX_COMPONENT_UV = 0x3,
		VERTEX_COMPONENT_TANGENT = 0x4,
		VERTEX_COMPONENT_BITANGENT = 0x5,
		VERTEX_COMPONENT_DUMMY_FLOAT = 0x6,
		VERTEX_COMPONENT_DUMMY_VEC4 = 0x7
	} Component;

	/** @brief Stores vertex layout components for model loading and Vulkan vertex input and atribute bindings  */
	struct VertexLayout {
	public:
		/** @brief Components used to generate vertices from */
		std::vector<Component> components;

		VertexLayout(std::vector<Component> components)
		{
			this->components = std::move(components);
		}

		uint32_t stride()
		{
			uint32_t res = 0;
			for (auto& component : components)
			{
				switch (component)
				{
				case VERTEX_COMPONENT_UV:
					res += 2 * sizeof(float);
					break;
				case VERTEX_COMPONENT_DUMMY_FLOAT:
					res += sizeof(float);
					break;
				case VERTEX_COMPONENT_DUMMY_VEC4:
					res += 4 * sizeof(float);
					break;
				default:
					// All components except the ones listed above are made up of 3 floats
					res += 3 * sizeof(float);
				}
			}
			return res;
		}
	};

	/** @brief Used to parametrize model loading */
	struct ModelCreateInfo {
		glm::vec3 center;
		glm::vec3 scale;
		glm::vec2 uvscale;

		ModelCreateInfo() {};

		ModelCreateInfo(glm::vec3 scale, glm::vec2 uvscale, glm::vec3 center)
		{
			this->center = center;
			this->scale = scale;
			this->uvscale = uvscale;
		}

		ModelCreateInfo(float scale, float uvscale, float center)
		{
			this->center = glm::vec3(center);
			this->scale = glm::vec3(scale);
			this->uvscale = glm::vec2(uvscale);
		}

	};

	struct Model {
		VkDevice device = nullptr;
		vks::Buffer vertices;
		vks::Buffer indices;
		uint32_t indexCount = 0;
		uint32_t vertexCount = 0;

		/** @brief Stores vertex and index base and counts for each part of a model */
		struct ModelPart {
			uint32_t vertexBase;
			uint32_t vertexCount;
			uint32_t indexBase;
			uint32_t indexCount;
		};
		std::vector<ModelPart> parts;

		static const int defaultFlags = aiProcess_FlipWindingOrder | aiProcess_Triangulate | aiProcess_PreTransformVertices | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals;

		struct Dimension
		{
			glm::vec3 min = glm::vec3(FLT_MAX);
			glm::vec3 max = glm::vec3(-FLT_MAX);
			glm::vec3 size;
		} dim;

		/** @brief Release all Vulkan resources of this model */
		void destroy()
		{		
			assert(device);
			vkDestroyBuffer(device, vertices.buffer, nullptr);
			vkFreeMemory(device, vertices.memory, nullptr);
			if (indices.buffer != VK_NULL_HANDLE)
			{
				vkDestroyBuffer(device, indices.buffer, nullptr);
				vkFreeMemory(device, indices.memory, nullptr);
			}
		}

		/**
		* Loads a 3D model from a file into Vulkan buffers
		*
		* @param device Pointer to the Vulkan device used to generated the vertex and index buffers on
		* @param filename File to load (must be a model format supported by ASSIMP)
		* @param layout Vertex layout components (position, normals, tangents, etc.)
		* @param createInfo MeshCreateInfo structure for load time settings like scale, center, etc.
		* @param copyQueue Queue used for the memory staging copy commands (must support transfer)
		* @param (Optional) flags ASSIMP model loading flags
		*/
		bool loadFromFile(const std::string& filename, vks::VertexLayout layout, vks::ModelCreateInfo *createInfo, vks::VulkanDevice *device, VkQueue copyQueue, const int flags = defaultFlags)
		{
			this->device = device->logicalDevice;

			Assimp::Importer Importer;
			const aiScene* pScene;

			// Load file
#if defined(__ANDROID__)
			// Meshes are stored inside the apk on Android (compressed)
			// So they need to be loaded via the asset manager

			AAsset* asset = AAssetManager_open(androidApp->activity->assetManager, filename.c_str(), AASSET_MODE_STREAMING);
			if (!asset) {
				LOGE("Could not load mesh from \"%s\"!", filename.c_str());
				return false;
			}
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
				parts.clear();
				parts.resize(pScene->mNumMeshes);

				glm::vec3 scale(1.0f);
				glm::vec2 uvscale(1.0f);
				glm::vec3 center(0.0f);
				if (createInfo)
				{
					scale = createInfo->scale;
					uvscale = createInfo->uvscale;
					center = createInfo->center;
				}

				std::vector<float> vertexBuffer;
				std::vector<uint32_t> indexBuffer;

				vertexCount = 0;
				indexCount = 0;

				// Load meshes
				for (unsigned int i = 0; i < pScene->mNumMeshes; i++)
				{
					const aiMesh* paiMesh = pScene->mMeshes[i];

					parts[i] = {};
					parts[i].vertexBase = vertexCount;
					parts[i].indexBase = indexCount;

					vertexCount += pScene->mMeshes[i]->mNumVertices;

					aiColor3D pColor(0.f, 0.f, 0.f);
					pScene->mMaterials[paiMesh->mMaterialIndex]->Get(AI_MATKEY_COLOR_DIFFUSE, pColor);

					const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);

					for (unsigned int j = 0; j < paiMesh->mNumVertices; j++)
					{
						const aiVector3D* pPos = &(paiMesh->mVertices[j]);
						const aiVector3D* pNormal = &(paiMesh->mNormals[j]);
						const aiVector3D* pTexCoord = (paiMesh->HasTextureCoords(0)) ? &(paiMesh->mTextureCoords[0][j]) : &Zero3D;
						const aiVector3D* pTangent = (paiMesh->HasTangentsAndBitangents()) ? &(paiMesh->mTangents[j]) : &Zero3D;
						const aiVector3D* pBiTangent = (paiMesh->HasTangentsAndBitangents()) ? &(paiMesh->mBitangents[j]) : &Zero3D;

						for (auto& component : layout.components)
						{
							switch (component) {
							case VERTEX_COMPONENT_POSITION:
								vertexBuffer.push_back(pPos->x * scale.x + center.x);
								vertexBuffer.push_back(-pPos->y * scale.y + center.y);
								vertexBuffer.push_back(pPos->z * scale.z + center.z);
								break;
							case VERTEX_COMPONENT_NORMAL:
								vertexBuffer.push_back(pNormal->x);
								vertexBuffer.push_back(-pNormal->y);
								vertexBuffer.push_back(pNormal->z);
								break;
							case VERTEX_COMPONENT_UV:
								vertexBuffer.push_back(pTexCoord->x * uvscale.s);
								vertexBuffer.push_back(pTexCoord->y * uvscale.t);
								break;
							case VERTEX_COMPONENT_COLOR:
								vertexBuffer.push_back(pColor.r);
								vertexBuffer.push_back(pColor.g);
								vertexBuffer.push_back(pColor.b);
								break;
							case VERTEX_COMPONENT_TANGENT:
								vertexBuffer.push_back(pTangent->x);
								vertexBuffer.push_back(pTangent->y);
								vertexBuffer.push_back(pTangent->z);
								break;
							case VERTEX_COMPONENT_BITANGENT:
								vertexBuffer.push_back(pBiTangent->x);
								vertexBuffer.push_back(pBiTangent->y);
								vertexBuffer.push_back(pBiTangent->z);
								break;
							// Dummy components for padding
							case VERTEX_COMPONENT_DUMMY_FLOAT:
								vertexBuffer.push_back(0.0f);
								break;
							case VERTEX_COMPONENT_DUMMY_VEC4:
								vertexBuffer.push_back(0.0f);
								vertexBuffer.push_back(0.0f);
								vertexBuffer.push_back(0.0f);
								vertexBuffer.push_back(0.0f);
								break;
							};
						}

						dim.max.x = fmax(pPos->x, dim.max.x);
						dim.max.y = fmax(pPos->y, dim.max.y);
						dim.max.z = fmax(pPos->z, dim.max.z);

						dim.min.x = fmin(pPos->x, dim.min.x);
						dim.min.y = fmin(pPos->y, dim.min.y);
						dim.min.z = fmin(pPos->z, dim.min.z);
					}

					dim.size = dim.max - dim.min;

					parts[i].vertexCount = paiMesh->mNumVertices;

					uint32_t indexBase = static_cast<uint32_t>(indexBuffer.size());
					for (unsigned int j = 0; j < paiMesh->mNumFaces; j++)
					{
						const aiFace& Face = paiMesh->mFaces[j];
						if (Face.mNumIndices != 3)
							continue;
						indexBuffer.push_back(indexBase + Face.mIndices[0]);
						indexBuffer.push_back(indexBase + Face.mIndices[1]);
						indexBuffer.push_back(indexBase + Face.mIndices[2]);
						parts[i].indexCount += 3;
						indexCount += 3;
					}
				}


				uint32_t vBufferSize = static_cast<uint32_t>(vertexBuffer.size()) * sizeof(float);
				uint32_t iBufferSize = static_cast<uint32_t>(indexBuffer.size()) * sizeof(uint32_t);

				// Use staging buffer to move vertex and index buffer to device local memory
				// Create staging buffers
				vks::Buffer vertexStaging, indexStaging;

				// Vertex buffer
				VK_CHECK_RESULT(device->createBuffer(
					VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
					&vertexStaging,
					vBufferSize,
					vertexBuffer.data()));

				// Index buffer
				VK_CHECK_RESULT(device->createBuffer(
					VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
					&indexStaging,
					iBufferSize,
					indexBuffer.data()));

				// Create device local target buffers
				// Vertex buffer
				VK_CHECK_RESULT(device->createBuffer(
					VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
					&vertices,
					vBufferSize));

				// Index buffer
				VK_CHECK_RESULT(device->createBuffer(
					VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
					&indices,
					iBufferSize));

				// Copy from staging buffers
				VkCommandBuffer copyCmd = device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

				VkBufferCopy copyRegion{};

				copyRegion.size = vertices.size;
				vkCmdCopyBuffer(copyCmd, vertexStaging.buffer, vertices.buffer, 1, &copyRegion);

				copyRegion.size = indices.size;
				vkCmdCopyBuffer(copyCmd, indexStaging.buffer, indices.buffer, 1, &copyRegion);

				device->flushCommandBuffer(copyCmd, copyQueue);

				// Destroy staging resources
				vkDestroyBuffer(device->logicalDevice, vertexStaging.buffer, nullptr);
				vkFreeMemory(device->logicalDevice, vertexStaging.memory, nullptr);
				vkDestroyBuffer(device->logicalDevice, indexStaging.buffer, nullptr);
				vkFreeMemory(device->logicalDevice, indexStaging.memory, nullptr);

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
		};

		/**
		* Loads a 3D model from a file into Vulkan buffers
		*
		* @param device Pointer to the Vulkan device used to generated the vertex and index buffers on
		* @param filename File to load (must be a model format supported by ASSIMP)
		* @param layout Vertex layout components (position, normals, tangents, etc.)
		* @param scale Load time scene scale
		* @param copyQueue Queue used for the memory staging copy commands (must support transfer)
		* @param (Optional) flags ASSIMP model loading flags
		*/
		bool loadFromFile(const std::string& filename, vks::VertexLayout layout, float scale, vks::VulkanDevice *device, VkQueue copyQueue, const int flags = defaultFlags)
		{
			vks::ModelCreateInfo modelCreateInfo(scale, 1.0f, 0.0f);
			return loadFromFile(filename, layout, &modelCreateInfo, device, copyQueue, flags);
		}
	};
};