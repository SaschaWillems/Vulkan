/*
* Vulkan Example - Skeletal animation
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <vector>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vulkan/vulkan.h>
#include "vulkanexamplebase.h"

#define VERTEX_BUFFER_BIND_ID 0
#define ENABLE_VALIDATION false

// Vertex layout used in this example
struct Vertex {
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec2 uv;
	glm::vec3 color;
	// Max. four bones per vertex
	float boneWeights[4];
	uint32_t boneIDs[4];
};

std::vector<vkMeshLoader::VertexLayout> vertexLayout =
{
	vkMeshLoader::VERTEX_LAYOUT_POSITION,
	vkMeshLoader::VERTEX_LAYOUT_NORMAL,
	vkMeshLoader::VERTEX_LAYOUT_UV,
	vkMeshLoader::VERTEX_LAYOUT_COLOR,
	vkMeshLoader::VERTEX_LAYOUT_DUMMY_VEC4,
	vkMeshLoader::VERTEX_LAYOUT_DUMMY_VEC4
};

// Maximum number of bones per mesh
// Must not be higher than same const in skinning shader
#define MAX_BONES 64
// Maximum number of bones per vertex
#define MAX_BONES_PER_VERTEX 4

// Skinned mesh class

// Per-vertex bone IDs and weights
struct VertexBoneData
{
	std::array<uint32_t, MAX_BONES_PER_VERTEX> IDs;
	std::array<float, MAX_BONES_PER_VERTEX> weights;

	// Ad bone weighting to vertex info
	void add(uint32_t boneID, float weight)
	{
		for (uint32_t i = 0; i < MAX_BONES_PER_VERTEX; i++)
		{
			if (weights[i] == 0.0f)
			{
				IDs[i] = boneID;
				weights[i] = weight;
				return;
			}
		}
	}
};

// Stores information on a single bone
struct BoneInfo
{
	aiMatrix4x4 offset;
	aiMatrix4x4 finalTransformation;

	BoneInfo()
	{
		offset = aiMatrix4x4();
		finalTransformation = aiMatrix4x4();
	};
};

class SkinnedMesh 
{
public:
	// Bone related stuff
	// Maps bone name with index
	std::map<std::string, uint32_t> boneMapping;
	// Bone details
	std::vector<BoneInfo> boneInfo;
	// Number of bones present
	uint32_t numBones = 0;
	// Root inverese transform matrix
	aiMatrix4x4 globalInverseTransform;
	// Per-vertex bone info
	std::vector<VertexBoneData> bones;
	// Bone transformations
	std::vector<aiMatrix4x4> boneTransforms;

	// Modifier for the animation 
	float animationSpeed = 0.75f;
	// Currently active animation
	aiAnimation* pAnimation;

	// Vulkan buffers
	vkMeshLoader::MeshBuffer meshBuffer;
	// Reference to assimp mesh
	// Required for animation
	VulkanMeshLoader *meshLoader;

	// Set active animation by index
	void setAnimation(uint32_t animationIndex)
	{
		assert(animationIndex < meshLoader->pScene->mNumAnimations);
		pAnimation = meshLoader->pScene->mAnimations[animationIndex];
	}

	// Load bone information from ASSIMP mesh
	void loadBones(uint32_t meshIndex, const aiMesh* pMesh, std::vector<VertexBoneData>& Bones)
	{
		for (uint32_t i = 0; i < pMesh->mNumBones; i++)
		{
			uint32_t index = 0;

			assert(pMesh->mNumBones <= MAX_BONES);

			std::string name(pMesh->mBones[i]->mName.data);

			if (boneMapping.find(name) == boneMapping.end())
			{
				// Bone not present, add new one
				index = numBones;
				numBones++;
				BoneInfo bone;
				boneInfo.push_back(bone);
				boneInfo[index].offset = pMesh->mBones[i]->mOffsetMatrix;
				boneMapping[name] = index;
			}
			else
			{
				index = boneMapping[name];
			}

			for (uint32_t j = 0; j < pMesh->mBones[i]->mNumWeights; j++)
			{
				uint32_t vertexID = meshLoader->m_Entries[meshIndex].vertexBase + pMesh->mBones[i]->mWeights[j].mVertexId;
				Bones[vertexID].add(index, pMesh->mBones[i]->mWeights[j].mWeight);
			}
		}
		boneTransforms.resize(numBones);
	}

	// Recursive bone transformation for given animation time
	void update(float time)
	{
		float TicksPerSecond = (float)(meshLoader->pScene->mAnimations[0]->mTicksPerSecond != 0 ? meshLoader->pScene->mAnimations[0]->mTicksPerSecond : 25.0f);
		float TimeInTicks = time * TicksPerSecond;
		float AnimationTime = fmod(TimeInTicks, (float)meshLoader->pScene->mAnimations[0]->mDuration);

		aiMatrix4x4 identity = aiMatrix4x4();
		readNodeHierarchy(AnimationTime, meshLoader->pScene->mRootNode, identity);

		for (uint32_t i = 0; i < boneTransforms.size(); i++)
		{
			boneTransforms[i] = boneInfo[i].finalTransformation;
		}
	}

private:
	// Find animation for a given node
	const aiNodeAnim* findNodeAnim(const aiAnimation* animation, const std::string nodeName)
	{
		for (uint32_t i = 0; i < animation->mNumChannels; i++)
		{
			const aiNodeAnim* nodeAnim = animation->mChannels[i];
			if (std::string(nodeAnim->mNodeName.data) == nodeName)
			{
				return nodeAnim;
			}
		}
		return nullptr;
	}

	// Returns a 4x4 matrix with interpolated translation between current and next frame
	aiMatrix4x4 interpolateTranslation(float time, const aiNodeAnim* pNodeAnim)
	{
		aiVector3D translation;

		if (pNodeAnim->mNumPositionKeys == 1)
		{
			translation = pNodeAnim->mPositionKeys[0].mValue;
		}
		else
		{
			uint32_t frameIndex = 0;
			for (uint32_t i = 0; i < pNodeAnim->mNumPositionKeys - 1; i++)
			{
				if (time < (float)pNodeAnim->mPositionKeys[i + 1].mTime)
				{
					frameIndex = i;
					break;
				}
			}

			aiVectorKey currentFrame = pNodeAnim->mPositionKeys[frameIndex];
			aiVectorKey nextFrame = pNodeAnim->mPositionKeys[(frameIndex + 1) % pNodeAnim->mNumPositionKeys];

			float delta = (time - (float)currentFrame.mTime) / (float)(nextFrame.mTime - currentFrame.mTime);

			const aiVector3D& start = currentFrame.mValue;
			const aiVector3D& end = nextFrame.mValue;

			translation = (start + delta * (end - start));
		}

		aiMatrix4x4 mat;
		aiMatrix4x4::Translation(translation, mat);
		return mat;
	}

	// Returns a 4x4 matrix with interpolated rotation between current and next frame
	aiMatrix4x4 interpolateRotation(float time, const aiNodeAnim* pNodeAnim)
	{
		aiQuaternion rotation;

		if (pNodeAnim->mNumRotationKeys == 1)
		{
			rotation = pNodeAnim->mRotationKeys[0].mValue;
		}
		else
		{
			uint32_t frameIndex = 0;
			for (uint32_t i = 0; i < pNodeAnim->mNumRotationKeys - 1; i++)
			{
				if (time < (float)pNodeAnim->mRotationKeys[i + 1].mTime)
				{
					frameIndex = i;
					break;
				}
			}

			aiQuatKey currentFrame = pNodeAnim->mRotationKeys[frameIndex];
			aiQuatKey nextFrame = pNodeAnim->mRotationKeys[(frameIndex + 1) % pNodeAnim->mNumRotationKeys];

			float delta = (time - (float)currentFrame.mTime) / (float)(nextFrame.mTime - currentFrame.mTime);

			const aiQuaternion& start = currentFrame.mValue;
			const aiQuaternion& end = nextFrame.mValue;

			aiQuaternion::Interpolate(rotation, start, end, delta);
			rotation.Normalize();
		}

		aiMatrix4x4 mat(rotation.GetMatrix());
		return mat;
	}


	// Returns a 4x4 matrix with interpolated scaling between current and next frame
	aiMatrix4x4 interpolateScale(float time, const aiNodeAnim* pNodeAnim)
	{
		aiVector3D scale;

		if (pNodeAnim->mNumScalingKeys == 1)
		{
			scale = pNodeAnim->mScalingKeys[0].mValue;
		}
		else
		{
			uint32_t frameIndex = 0;
			for (uint32_t i = 0; i < pNodeAnim->mNumScalingKeys - 1; i++)
			{
				if (time < (float)pNodeAnim->mScalingKeys[i + 1].mTime)
				{
					frameIndex = i;
					break;
				}
			}

			aiVectorKey currentFrame = pNodeAnim->mScalingKeys[frameIndex];
			aiVectorKey nextFrame = pNodeAnim->mScalingKeys[(frameIndex + 1) % pNodeAnim->mNumScalingKeys];

			float delta = (time - (float)currentFrame.mTime) / (float)(nextFrame.mTime - currentFrame.mTime);

			const aiVector3D& start = currentFrame.mValue;
			const aiVector3D& end = nextFrame.mValue;

			scale = (start + delta * (end - start));
		}

		aiMatrix4x4 mat;
		aiMatrix4x4::Scaling(scale, mat);
		return mat;
	}

	// Get node hierarchy for current animation time
	void readNodeHierarchy(float AnimationTime, const aiNode* pNode, const aiMatrix4x4& ParentTransform)
	{
		std::string NodeName(pNode->mName.data);

		aiMatrix4x4 NodeTransformation(pNode->mTransformation);

		const aiNodeAnim* pNodeAnim = findNodeAnim(pAnimation, NodeName);

		if (pNodeAnim)
		{
			// Get interpolated matrices between current and next frame
			aiMatrix4x4 matScale = interpolateScale(AnimationTime, pNodeAnim);
			aiMatrix4x4 matRotation = interpolateRotation(AnimationTime, pNodeAnim);
			aiMatrix4x4 matTranslation = interpolateTranslation(AnimationTime, pNodeAnim);

			NodeTransformation = matTranslation * matRotation * matScale;
		}

		aiMatrix4x4 GlobalTransformation = ParentTransform * NodeTransformation;

		if (boneMapping.find(NodeName) != boneMapping.end())
		{
			uint32_t BoneIndex = boneMapping[NodeName];
			boneInfo[BoneIndex].finalTransformation = globalInverseTransform * GlobalTransformation * boneInfo[BoneIndex].offset;
		}

		for (uint32_t i = 0; i < pNode->mNumChildren; i++)
		{
			readNodeHierarchy(AnimationTime, pNode->mChildren[i], GlobalTransformation);
		}
	}
};

class VulkanExample : public VulkanExampleBase
{
public:
	struct {
		vkTools::VulkanTexture colorMap;
		vkTools::VulkanTexture floor;
	} textures;

	struct {
		VkPipelineVertexInputStateCreateInfo inputState;
		std::vector<VkVertexInputBindingDescription> bindingDescriptions;
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
	} vertices;

	SkinnedMesh *skinnedMesh = nullptr;

	struct {
		vkTools::UniformData vsScene;
		vkTools::UniformData floor;
	} uniformData;

	struct {
		glm::mat4 projection;
		glm::mat4 model;
		glm::mat4 bones[MAX_BONES];
		glm::vec4 lightPos = glm::vec4(0.0f, -250.0f, 250.0f, 1.0);
		glm::vec4 viewPos;
	} uboVS;

	struct {
		glm::mat4 projection;
		glm::mat4 model;
		glm::vec4 lightPos = glm::vec4(0.0, 0.0f, -25.0f, 1.0);
		glm::vec4 viewPos;
		glm::vec2 uvOffset;
	} uboFloor;

	struct {
		VkPipeline skinning;
		VkPipeline texture;
	} pipelines;

	struct {
		vkMeshLoader::MeshBuffer floor;
	} meshes;

	VkPipelineLayout pipelineLayout;
	VkDescriptorSet descriptorSet;
	VkDescriptorSetLayout descriptorSetLayout;

	struct {
		VkDescriptorSet skinning;
		VkDescriptorSet floor;
	} descriptorSets;

	float runningTime = 0.0f;

	VulkanExample() : VulkanExampleBase(ENABLE_VALIDATION)
	{
		zoom = -150.0f;
		zoomSpeed = 2.5f;
		rotationSpeed = 0.5f;
		rotation = { -182.5f, -38.5f, 180.0f };
		enableTextOverlay = true;
		title = "Vulkan Example - Skeletal animation";
		cameraPos = { 0.0f, 0.0f, 12.0f };
	}

	~VulkanExample()
	{
		// Clean up used Vulkan resources 
		// Note : Inherited destructor cleans up resources stored in base class
		vkDestroyPipeline(device, pipelines.skinning, nullptr);

		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);


		textureLoader->destroyTexture(textures.colorMap);

		vkTools::destroyUniformData(device, &uniformData.vsScene);

		// Destroy and free mesh resources 
		vkMeshLoader::freeMeshBufferResources(device, &skinnedMesh->meshBuffer);
		delete(skinnedMesh->meshLoader);
		delete(skinnedMesh);
	}

	void buildCommandBuffers()
	{
		VkCommandBufferBeginInfo cmdBufInfo = vkTools::initializers::commandBufferBeginInfo();

		VkClearValue clearValues[2];
		clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f} };
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo renderPassBeginInfo = vkTools::initializers::renderPassBeginInfo();
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

			vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport viewport = vkTools::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
			vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

			VkRect2D scissor = vkTools::initializers::rect2D(width, height, 0, 0);
			vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

			VkDeviceSize offsets[1] = { 0 };

			// Skinned mesh
			vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);
			vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.skinning);

			vkCmdBindVertexBuffers(drawCmdBuffers[i], VERTEX_BUFFER_BIND_ID, 1, &skinnedMesh->meshBuffer.vertices.buf, offsets);
			vkCmdBindIndexBuffer(drawCmdBuffers[i], skinnedMesh->meshBuffer.indices.buf, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(drawCmdBuffers[i], skinnedMesh->meshBuffer.indexCount, 1, 0, 0, 0);

			// Floor
			vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets.floor, 0, NULL);
			vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.texture);

			vkCmdBindVertexBuffers(drawCmdBuffers[i], VERTEX_BUFFER_BIND_ID, 1, &meshes.floor.vertices.buf, offsets);
			vkCmdBindIndexBuffer(drawCmdBuffers[i], meshes.floor.indices.buf, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(drawCmdBuffers[i], meshes.floor.indexCount, 1, 0, 0, 0);

			vkCmdEndRenderPass(drawCmdBuffers[i]);

			VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
		}
	}

	// Load a mesh based on data read via assimp 
	// The other example will use the VulkanMesh loader which has some additional functionality for loading meshes
	void loadMesh()
	{
		skinnedMesh = new SkinnedMesh();
		skinnedMesh->meshLoader = new VulkanMeshLoader();
#if defined(__ANDROID__)
		skinnedMesh->meshLoader->assetManager = androidApp->activity->assetManager;
#endif
		skinnedMesh->meshLoader->LoadMesh(getAssetPath() + "models/goblin.dae", 0);
		skinnedMesh->setAnimation(0);

		// Setup bones
		// One vertex bone info structure per vertex
		skinnedMesh->bones.resize(skinnedMesh->meshLoader->numVertices);
		// Store global inverse transform matrix of root node 
		skinnedMesh->globalInverseTransform = skinnedMesh->meshLoader->pScene->mRootNode->mTransformation;
		skinnedMesh->globalInverseTransform.Inverse();
		// Load bones (weights and IDs)
		for (uint32_t m = 0; m < skinnedMesh->meshLoader->m_Entries.size(); m++)
		{
			aiMesh *paiMesh = skinnedMesh->meshLoader->pScene->mMeshes[m];
			if (paiMesh->mNumBones > 0)
			{
				skinnedMesh->loadBones(m, paiMesh, skinnedMesh->bones);
			}
		}

		// Generate vertex buffer
		std::vector<Vertex> vertexBuffer;
		// Iterate through all meshes in the file
		// and extract the vertex information used in this demo
		for (uint32_t m = 0; m < skinnedMesh->meshLoader->m_Entries.size(); m++)
		{
			for (uint32_t i = 0; i < skinnedMesh->meshLoader->m_Entries[m].Vertices.size(); i++)
			{
				Vertex vertex;

				vertex.pos = skinnedMesh->meshLoader->m_Entries[m].Vertices[i].m_pos;
				vertex.pos.y = -vertex.pos.y;
				vertex.normal = skinnedMesh->meshLoader->m_Entries[m].Vertices[i].m_normal;
				vertex.uv = skinnedMesh->meshLoader->m_Entries[m].Vertices[i].m_tex;
				vertex.color = skinnedMesh->meshLoader->m_Entries[m].Vertices[i].m_color;

				// Fetch bone weights and IDs
				for (uint32_t j = 0; j < MAX_BONES_PER_VERTEX; j++)
				{
					vertex.boneWeights[j] = skinnedMesh->bones[skinnedMesh->meshLoader->m_Entries[m].vertexBase + i].weights[j];
					vertex.boneIDs[j] = skinnedMesh->bones[skinnedMesh->meshLoader->m_Entries[m].vertexBase + i].IDs[j];
				}

				vertexBuffer.push_back(vertex);
			}
		}
		uint32_t vertexBufferSize = vertexBuffer.size() * sizeof(Vertex);

		// Generate index buffer from loaded mesh file
		std::vector<uint32_t> indexBuffer;
		for (uint32_t m = 0; m < skinnedMesh->meshLoader->m_Entries.size(); m++)
		{
			uint32_t indexBase = indexBuffer.size();
			for (uint32_t i = 0; i < skinnedMesh->meshLoader->m_Entries[m].Indices.size(); i++)
			{
				indexBuffer.push_back(skinnedMesh->meshLoader->m_Entries[m].Indices[i] + indexBase);
			}
		}
		uint32_t indexBufferSize = indexBuffer.size() * sizeof(uint32_t);
		skinnedMesh->meshBuffer.indexCount = indexBuffer.size();

		bool useStaging = true;

		if (useStaging)
		{
			struct {
				VkBuffer buffer;
				VkDeviceMemory memory;
			} vertexStaging, indexStaging;

			// Create staging buffers
			// Vertex data
			createBuffer(
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
				vertexBufferSize,
				vertexBuffer.data(),
				&vertexStaging.buffer,
				&vertexStaging.memory);
			// Index data
			createBuffer(
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
				indexBufferSize,
				indexBuffer.data(),
				&indexStaging.buffer,
				&indexStaging.memory);

			// Create device local buffers
			// Vertex buffer
			createBuffer(
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				vertexBufferSize,
				nullptr,
				&skinnedMesh->meshBuffer.vertices.buf,
				&skinnedMesh->meshBuffer.vertices.mem);
			// Index buffer
			createBuffer(
				VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				indexBufferSize,
				nullptr,
				&skinnedMesh->meshBuffer.indices.buf,
				&skinnedMesh->meshBuffer.indices.mem);

			// Copy from staging buffers
			VkCommandBuffer copyCmd = VulkanExampleBase::createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

			VkBufferCopy copyRegion = {};

			copyRegion.size = vertexBufferSize;
			vkCmdCopyBuffer(
				copyCmd,
				vertexStaging.buffer,
				skinnedMesh->meshBuffer.vertices.buf,
				1,
				&copyRegion);

			copyRegion.size = indexBufferSize;
			vkCmdCopyBuffer(
				copyCmd,
				indexStaging.buffer,
				skinnedMesh->meshBuffer.indices.buf,
				1,
				&copyRegion);

			VulkanExampleBase::flushCommandBuffer(copyCmd, queue, true);

			vkDestroyBuffer(device, vertexStaging.buffer, nullptr);
			vkFreeMemory(device, vertexStaging.memory, nullptr);
			vkDestroyBuffer(device, indexStaging.buffer, nullptr);
			vkFreeMemory(device, indexStaging.memory, nullptr);
		} 
		else
		{
			// Vertex buffer
			createBuffer(
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
				vertexBufferSize,
				vertexBuffer.data(),
				&skinnedMesh->meshBuffer.vertices.buf,
				&skinnedMesh->meshBuffer.vertices.mem);
			// Index buffer
			createBuffer(
				VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
				indexBufferSize,
				indexBuffer.data(),
				&skinnedMesh->meshBuffer.indices.buf,
				&skinnedMesh->meshBuffer.indices.mem);
		}
	}

	void loadTextures()
	{
		textureLoader->loadTexture(
			getAssetPath() + "textures/goblin_bc3.ktx",
			VK_FORMAT_BC3_UNORM_BLOCK,
			&textures.colorMap);

		textureLoader->loadTexture(
			getAssetPath() + "textures/pattern_35_bc3.ktx",
			VK_FORMAT_BC3_UNORM_BLOCK,
			&textures.floor);
	}

	void loadMeshes()
	{
		VulkanExampleBase::loadMesh(getAssetPath() + "models/plane_z.obj", &meshes.floor, vertexLayout, 512.0f);
	}

	void setupVertexDescriptions()
	{
		// Binding description
		vertices.bindingDescriptions.resize(1);
		vertices.bindingDescriptions[0] =
			vkTools::initializers::vertexInputBindingDescription(
				VERTEX_BUFFER_BIND_ID,
				sizeof(Vertex),
				VK_VERTEX_INPUT_RATE_VERTEX);

		// Attribute descriptions
		// Describes memory layout and shader positions
		vertices.attributeDescriptions.resize(6);
		// Location 0 : Position
		vertices.attributeDescriptions[0] =
			vkTools::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				0,
				VK_FORMAT_R32G32B32_SFLOAT,
				0);
		// Location 1 : Normal
		vertices.attributeDescriptions[1] =
			vkTools::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				1,
				VK_FORMAT_R32G32B32_SFLOAT,
				sizeof(float) * 3);
		// Location 2 : Texture coordinates
		vertices.attributeDescriptions[2] =
			vkTools::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				2,
				VK_FORMAT_R32G32_SFLOAT,
				sizeof(float) * 6);
		// Location 3 : Color
		vertices.attributeDescriptions[3] =
			vkTools::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				3,
				VK_FORMAT_R32G32B32_SFLOAT,
				sizeof(float) * 8);
		// Location 4 : Bone weights
		vertices.attributeDescriptions[4] =
			vkTools::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				4,
				VK_FORMAT_R32G32B32A32_SFLOAT,
				sizeof(float) * 11);
		// Location 5 : Bone IDs
		vertices.attributeDescriptions[5] =
			vkTools::initializers::vertexInputAttributeDescription(
				VERTEX_BUFFER_BIND_ID,
				5,
				VK_FORMAT_R32G32B32A32_SINT,
				sizeof(float) * 15);

		vertices.inputState = vkTools::initializers::pipelineVertexInputStateCreateInfo();
		vertices.inputState.vertexBindingDescriptionCount = vertices.bindingDescriptions.size();
		vertices.inputState.pVertexBindingDescriptions = vertices.bindingDescriptions.data();
		vertices.inputState.vertexAttributeDescriptionCount = vertices.attributeDescriptions.size();
		vertices.inputState.pVertexAttributeDescriptions = vertices.attributeDescriptions.data();
	}

	void setupDescriptorPool()
	{
		// Example uses one ubo and one combined image sampler
		std::vector<VkDescriptorPoolSize> poolSizes =
		{
			vkTools::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2),
			vkTools::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2),
		};

		VkDescriptorPoolCreateInfo descriptorPoolInfo =
			vkTools::initializers::descriptorPoolCreateInfo(
				poolSizes.size(),
				poolSizes.data(),
				2);

		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));
	}

	void setupDescriptorSetLayout()
	{
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings =
		{
			// Binding 0 : Vertex shader uniform buffer
			vkTools::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				VK_SHADER_STAGE_VERTEX_BIT,
				0),
			// Binding 1 : Fragment shader combined sampler
			vkTools::initializers::descriptorSetLayoutBinding(
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_SHADER_STAGE_FRAGMENT_BIT,
				1),
		};

		VkDescriptorSetLayoutCreateInfo descriptorLayout =
			vkTools::initializers::descriptorSetLayoutCreateInfo(
				setLayoutBindings.data(),
				setLayoutBindings.size());

		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayout));

		VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo =
			vkTools::initializers::pipelineLayoutCreateInfo(
				&descriptorSetLayout,
				1);

		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &pipelineLayout));
	}

	void setupDescriptorSet()
	{
		VkDescriptorSetAllocateInfo allocInfo =
			vkTools::initializers::descriptorSetAllocateInfo(
				descriptorPool,
				&descriptorSetLayout,
				1);

		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));
		
		VkDescriptorImageInfo texDescriptor =
			vkTools::initializers::descriptorImageInfo(
				textures.colorMap.sampler,
				textures.colorMap.view,
				VK_IMAGE_LAYOUT_GENERAL);

		std::vector<VkWriteDescriptorSet> writeDescriptorSets =
		{
			// Binding 0 : Vertex shader uniform buffer
			vkTools::initializers::writeDescriptorSet(
				descriptorSet,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				0,
				&uniformData.vsScene.descriptor),
			// Binding 1 : Color map 
			vkTools::initializers::writeDescriptorSet(
				descriptorSet,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				1,
				&texDescriptor)
		};

		vkUpdateDescriptorSets(device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);

		// Floor
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.floor));

		texDescriptor.imageView = textures.floor.view;
		texDescriptor.sampler = textures.floor.sampler;

		writeDescriptorSets.clear();

		// Binding 0 : Vertex shader uniform buffer
		writeDescriptorSets.push_back(
			vkTools::initializers::writeDescriptorSet(
				descriptorSets.floor,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				0,
				&uniformData.floor.descriptor));
		// Binding 1 : Color map 
		writeDescriptorSets.push_back(
			vkTools::initializers::writeDescriptorSet(
				descriptorSets.floor,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				1,
				&texDescriptor));

		vkUpdateDescriptorSets(device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);
	}

	void preparePipelines()
	{
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
			vkTools::initializers::pipelineInputAssemblyStateCreateInfo(
				VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
				0,
				VK_FALSE);

		VkPipelineRasterizationStateCreateInfo rasterizationState =
			vkTools::initializers::pipelineRasterizationStateCreateInfo(
				VK_POLYGON_MODE_FILL,
				VK_CULL_MODE_BACK_BIT,
				VK_FRONT_FACE_CLOCKWISE,
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

		std::vector<VkDynamicState> dynamicStateEnables = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};
		VkPipelineDynamicStateCreateInfo dynamicState =
			vkTools::initializers::pipelineDynamicStateCreateInfo(
				dynamicStateEnables.data(),
				dynamicStateEnables.size(),
				0);

		// Skinned rendering pipeline
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

		shaderStages[0] = loadShader(getAssetPath() + "shaders/skeletalanimation/mesh.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getAssetPath() + "shaders/skeletalanimation/mesh.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

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

		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.skinning));

		shaderStages[0] = loadShader(getAssetPath() + "shaders/skeletalanimation/texture.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getAssetPath() + "shaders/skeletalanimation/texture.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipelines.texture));
	}

	// Prepare and initialize uniform buffer containing shader uniforms
	void prepareUniformBuffers()
	{
		// Vertex shader uniform buffer block
		createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			sizeof(uboVS),
			nullptr,
			&uniformData.vsScene.buffer,
			&uniformData.vsScene.memory,
			&uniformData.vsScene.descriptor);

		// Map for host access
		VK_CHECK_RESULT(vkMapMemory(device, uniformData.vsScene.memory, 0, sizeof(uboVS), 0, (void **)&uniformData.vsScene.mapped));

		// Floor
		createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			sizeof(uboFloor),
			nullptr,
			&uniformData.floor.buffer,
			&uniformData.floor.memory,
			&uniformData.floor.descriptor);

		// Map for host access
		VK_CHECK_RESULT(vkMapMemory(device, uniformData.floor.memory, 0, sizeof(uboFloor), 0, (void **)&uniformData.floor.mapped));

		updateUniformBuffers(true);
	}

	void updateUniformBuffers(bool viewChanged)
	{
		if (viewChanged)
		{
			uboVS.projection = glm::perspective(glm::radians(60.0f), (float)width / (float)height, 0.1f, 512.0f);

			glm::mat4 viewMatrix = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, zoom));
			viewMatrix = glm::rotate(viewMatrix, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
			viewMatrix = glm::scale(viewMatrix, glm::vec3(0.025f));

			uboVS.model = viewMatrix * glm::translate(glm::mat4(), glm::vec3(cameraPos.x, -cameraPos.z, cameraPos.y) * 100.0f);
			uboVS.model = glm::rotate(uboVS.model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
			uboVS.model = glm::rotate(uboVS.model, glm::radians(rotation.z), glm::vec3(0.0f, 1.0f, 0.0f));
			uboVS.model = glm::rotate(uboVS.model, glm::radians(-rotation.y), glm::vec3(0.0f, 0.0f, 1.0f));

			uboVS.viewPos = glm::vec4(0.0f, 0.0f, -zoom, 0.0f);

			uboFloor.projection = uboVS.projection;
			uboFloor.model = viewMatrix * glm::translate(glm::mat4(), glm::vec3(cameraPos.x, -cameraPos.z, cameraPos.y) * 100.0f);
			uboFloor.model = glm::rotate(uboFloor.model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
			uboFloor.model = glm::rotate(uboFloor.model, glm::radians(rotation.z), glm::vec3(0.0f, 1.0f, 0.0f));
			uboFloor.model = glm::rotate(uboFloor.model, glm::radians(-rotation.y), glm::vec3(0.0f, 0.0f, 1.0f));
			uboFloor.model = glm::translate(uboFloor.model, glm::vec3(0.0f, 0.0f, -1800.0f));
			uboFloor.viewPos = glm::vec4(0.0f, 0.0f, -zoom, 0.0f);
		}

		// Update bones
		skinnedMesh->update(runningTime);
		for (uint32_t i = 0; i < skinnedMesh->boneTransforms.size(); i++)
		{
			uboVS.bones[i] = glm::transpose(glm::make_mat4(&skinnedMesh->boneTransforms[i].a1));
		}

		memcpy(uniformData.vsScene.mapped, &uboVS, sizeof(uboVS));

		// Update floor animation
		uboFloor.uvOffset.t -= 0.5f * skinnedMesh->animationSpeed * frameTimer;
		memcpy(uniformData.floor.mapped, &uboFloor, sizeof(uboFloor));
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
		loadTextures();
		loadMesh();
		loadMeshes();
		setupVertexDescriptions();
		prepareUniformBuffers();
		setupDescriptorSetLayout();
		preparePipelines();
		setupDescriptorPool();
		setupDescriptorSet();
		buildCommandBuffers();
		prepared = true;
	}

	virtual void render()
	{
		if (!prepared)
			return;
		draw();
		if (!paused)
		{
			runningTime += frameTimer * skinnedMesh->animationSpeed;
			vkDeviceWaitIdle(device);
			updateUniformBuffers(false);
		}
	}

	virtual void viewChanged()
	{
		vkDeviceWaitIdle(device);
		updateUniformBuffers(true);
	}

	void changeAnimationSpeed(float delta)
	{
		skinnedMesh->animationSpeed += delta;
	}

	virtual void keyPressed(uint32_t keyCode)
	{
		switch (keyCode)
		{
		case 0x6B:
		case GAMEPAD_BUTTON_R1:
			changeAnimationSpeed(0.1f);
			break;
		case 0x6D:
		case GAMEPAD_BUTTON_L1:
			changeAnimationSpeed(-0.1f);
			break;
		}
	}

	virtual void getOverlayText(VulkanTextOverlay *textOverlay)
	{
		if (skinnedMesh != nullptr)
		{
			std::stringstream ss;
			ss << std::setprecision(2) << std::fixed << skinnedMesh->animationSpeed;
#if defined(__ANDROID__)
			textOverlay->addText("Animation speed: " + ss.str() + " (Buttons L1/R1 to change)", 5.0f, 85.0f, VulkanTextOverlay::alignLeft);
#else
			textOverlay->addText("Animation speed: " + ss.str() + " (numpad +/- to change)", 5.0f, 85.0f, VulkanTextOverlay::alignLeft);
#endif
		}
	}
};

VulkanExample *vulkanExample;

#if defined(_WIN32)
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (vulkanExample != NULL)
	{
		vulkanExample->handleMessages(hWnd, uMsg, wParam, lParam);
	}
	return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}
#elif defined(__linux__) && !defined(__ANDROID__)
static void handleEvent(const xcb_generic_event_t *event)
{
	if (vulkanExample != NULL)
	{
		vulkanExample->handleEvent(event);
	}
}
#endif

// Main entry point
#if defined(_WIN32)
// Windows entry point
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
#elif defined(__ANDROID__)
// Android entry point
void android_main(android_app* state)
#elif defined(__linux__)
// Linux entry point
int main(const int argc, const char *argv[])
#endif
{
#if defined(__ANDROID__)
	// Removing this may cause the compiler to omit the main entry point 
	// which would make the application crash at start
	app_dummy();
#endif
	vulkanExample = new VulkanExample();
#if defined(_WIN32)
	vulkanExample->setupWindow(hInstance, WndProc);
#elif defined(__ANDROID__)
	// Attach vulkan example to global android application state
	state->userData = vulkanExample;
	state->onAppCmd = VulkanExample::handleAppCommand;
	state->onInputEvent = VulkanExample::handleAppInput;
	vulkanExample->androidApp = state;
#elif defined(__linux__)
	vulkanExample->setupWindow();
#endif
#if !defined(__ANDROID__)
	vulkanExample->initSwapchain();
	vulkanExample->prepare();
#endif
	vulkanExample->renderLoop();
	delete(vulkanExample);
#if !defined(__ANDROID__)
	return 0;
#endif
}