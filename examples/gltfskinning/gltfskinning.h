/*
* Vulkan Example - glTF skinned animation
*
* Copyright (C) 2020 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

/*
 * This is based on the glTF scene example and only the parts that show added functionality are commented

 * @todo: Rework comments
 * Shows how to load and display a simple scene from a glTF file
 * Note that this isn't a complete glTF loader and only basic functions are shown here
 * This means no complex materials, no animations, no skins, etc.
 * For details on how glTF 2.0 works, see the official spec at https://github.com/KhronosGroup/glTF/tree/master/specification/2.0
 *
 * Other samples will load models using a dedicated model loader with more features (see base/VulkanglTFModel.hpp)
 * 
 * If you are looking for a complete glTF implementation, check out https://github.com/SaschaWillems/Vulkan-glTF-PBR/
 */

 // @todo: add link to https://github.com/KhronosGroup/glTF-Tutorials/blob/master/gltfTutorial/gltfTutorial_020_Skins.md

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

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#ifdef VK_USE_PLATFORM_ANDROID_KHR
#define TINYGLTF_ANDROID_LOAD_FROM_ASSETS
#endif
#include "tiny_gltf.h"

#include <vulkan/vulkan.h>
#include "vulkanexamplebase.h"
#include "VulkanTexture.hpp"

#define ENABLE_VALIDATION false

// Contains everything required to render a glTF model in Vulkan
// This class is heavily simplified (compared to glTF's feature set) but retains the basic glTF structure
class VulkanglTFModel 
{
public:
	vks::VulkanDevice* vulkanDevice;
	VkQueue copyQueue;	

	struct Vertex {
		glm::vec3 pos;
		glm::vec3 normal;
		glm::vec2 uv;
		glm::vec3 color;
		// Contains indices of the joints that effect this vertex
		glm::vec4 jointIndices;
		// Contains the weights that define how strongly this vertex is affected by above joints
		glm::vec4 jointWeights;
	};

	struct {
		VkBuffer buffer;
		VkDeviceMemory memory;
	} vertices;
	
	struct {
		int count;
		VkBuffer buffer;
		VkDeviceMemory memory;
	} indices;

	struct Node;

	struct Material {
		glm::vec4 baseColorFactor = glm::vec4(1.0f);
		uint32_t baseColorTextureIndex;
	};

	struct Image {
		vks::Texture2D texture;
		VkDescriptorSet descriptorSet;
	};

	struct Texture {
		int32_t imageIndex;
	};

	struct Primitive {
		uint32_t firstIndex;
		uint32_t indexCount;
		int32_t materialIndex;
	};

	struct Mesh {
		std::vector<Primitive> primitives;
	};

	struct Node {
		Node* parent;
		uint32_t index;
		std::vector<Node*> children;
		Mesh mesh;
		// Matrix components are stored separately as they are affected by animations
		glm::vec3 translation{};
		glm::vec3 scale{ 1.0f };
		glm::quat rotation{};
		// Index of the skin for this node
		int32_t skin = -1;
		glm::mat4 matrix;
		// Gets the current local matrix based on translation, rotation and scale, which can all be affected by animations
		glm::mat4 getLocalMatrix();
	};

	// A skin contains the joints and matrices applied during vertex skinning 
	// @todo: Add link to spec
	struct Skin {
		std::string name;
		Node* skeletonRoot = nullptr;
		std::vector<glm::mat4> inverseBindMatrices;
		std::vector<Node*> joints;
		// POI: Store joint matrices in an SSBO
		// @todo: proper comment
		std::vector<glm::mat4> jointMatrices;
		vks::Buffer ssbo;
		VkDescriptorSet descriptorSet;
	};


	/*
		glTF animation channel
		// @todo: Comment
	*/
	struct AnimationChannel {
		std::string path;
		Node* node;
		uint32_t samplerIndex;
	};

	/*
		glTF animation sampler
		// @todo: Comment
	*/
	struct AnimationSampler {
		std::string interpolation;
		std::vector<float> inputs;
		std::vector<glm::vec4> outputsVec4;
	};

	/*
		glTF animation
		// @todo: Comment
	*/
	struct Animation {
		std::string name;
		std::vector<AnimationSampler> samplers;
		std::vector<AnimationChannel> channels;
		float start = std::numeric_limits<float>::max();
		float end = std::numeric_limits<float>::min();
		float currentTime = 0.0f;
	};

	std::vector<Image> images;
	std::vector<Texture> textures;
	std::vector<Material> materials;
	std::vector<Node*> nodes;
	
	// Store skins and animations
	std::vector<Skin> skins;
	std::vector<Animation> animations;

	// POI: @todo: document	
	struct MeshData {
		glm::mat4 jointMatrix[32]{};
	};
	struct ShaderData {
		vks::Buffer buffer;
	} shaderData;
	VkDescriptorSet descriptorSet;
	std::vector<MeshData> meshdata;

	uint32_t activeAnimation = 0;

	~VulkanglTFModel();
	void loadImages(tinygltf::Model& input);
	void loadTextures(tinygltf::Model& input);
	void loadMaterials(tinygltf::Model& input);
	Node* findNode(Node* parent, uint32_t index);
	Node* nodeFromIndex(uint32_t index);
	void loadSkins(tinygltf::Model& input);
	void loadAnimations(tinygltf::Model& input);
	void loadNode(const tinygltf::Node& inputNode, const tinygltf::Model& input, VulkanglTFModel::Node* parent, uint32_t nodeIndex, std::vector<uint32_t>& indexBuffer, std::vector<VulkanglTFModel::Vertex>& vertexBuffer);
	glm::mat4 getNodeMatrix(VulkanglTFModel::Node* node);
	glm::mat4 getNodeMatrix2(VulkanglTFModel::Node* node);
	void updateJoints(VulkanglTFModel::Node* node);
	void updateAnimation(float deltaTime);
	void drawNode(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, VulkanglTFModel::Node node);
	void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);
};

class VulkanExample : public VulkanExampleBase
{
public:
	bool wireframe = false;

	struct ShaderData {
		vks::Buffer buffer;
		struct Values {
			glm::mat4 projection;
			glm::mat4 model;
			glm::vec4 lightPos = glm::vec4(5.0f, 5.0f, 5.0f, 1.0f);
		} values;
	} shaderData;

	VkPipelineLayout pipelineLayout;
	struct Pipelines {
		VkPipeline solid;
		VkPipeline wireframe = VK_NULL_HANDLE;
	} pipelines;

	struct DescriptorSetLayouts {
		VkDescriptorSetLayout matrices;
		VkDescriptorSetLayout textures;
		VkDescriptorSetLayout jointMatrices;
	} descriptorSetLayouts;
	VkDescriptorSet descriptorSet;

	VulkanglTFModel glTFModel;

	VulkanExample();
	~VulkanExample();
	void loadglTFFile(std::string filename);
	virtual void getEnabledFeatures();
	void buildCommandBuffers();
	void loadAssets();
	void setupDescriptors();
	void preparePipelines();
	void prepareUniformBuffers();
	void updateUniformBuffers();
	void prepare();
	virtual void render();
	virtual void OnUpdateUIOverlay(vks::UIOverlay* overlay);
};
