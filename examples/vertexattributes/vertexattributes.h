/*
 * Vulkan Example - Passing vertex attributes using interleaved and separate buffers
 *
 * Copyright (C) 2022 by Sascha Willems - www.saschawillems.de
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#ifdef VK_USE_PLATFORM_ANDROID_KHR
#define TINYGLTF_ANDROID_LOAD_FROM_ASSETS
#endif
#include "tiny_gltf.h"

#include "vulkanexamplebase.h"

#define ENABLE_VALIDATION false

struct PushConstBlock {
	glm::mat4 nodeMatrix;
	uint32_t alphaMask;
	float alphaMaskCutoff;
};

struct Material {
	glm::vec4 baseColorFactor = glm::vec4(1.0f);
	uint32_t baseColorTextureIndex;
	uint32_t normalTextureIndex;
	std::string alphaMode = "OPAQUE";
	float alphaCutOff;
	VkDescriptorSet descriptorSet;
};

struct Image {
	vks::Texture2D texture;
};

struct Texture {
	int32_t imageIndex;
};

// Layout for the interleaved vertex attributes
struct Vertex {
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec2 uv;
	glm::vec4 tangent;
};

struct Primitive {
	uint32_t firstIndex;
	uint32_t indexCount;
	int32_t materialIndex;
};
struct Mesh {
	std::vector<Primitive> primitives;
};
struct Node;
struct Node {
	Node* parent;
	std::vector<Node> children;
	Mesh mesh;
	glm::mat4 matrix;
};

std::vector<Node> nodes;

class VulkanExample : public VulkanExampleBase
{
public:
	enum VertexAttributeSettings { interleaved, separate };
	VertexAttributeSettings vertexAttributeSettings = separate;

	// Used to store indices and vertices from glTF to be uploaded to the GPU
	std::vector<uint32_t> indexBuffer;
	std::vector<Vertex> vertexBuffer;
	struct VertexAttributes {
		std::vector<glm::vec2> uv;
		std::vector<glm::vec3> pos, normal;
		std::vector<glm::vec4> tangent;
	} vertexAttributeBuffers;

	// Buffers for the separate vertex attributes
	// @todo: rename
	struct SeparateVertexBuffers {
		vks::Buffer pos, normal, uv, tangent;
	} separateVertexBuffers;

	// Single vertex buffer for all primitives
	vks::Buffer interleavedVertexBuffer;

	// Index buffer for all primitives of the scene
	vks::Buffer indices;

	struct ShaderData {
		vks::Buffer buffer;
		struct Values {
			glm::mat4 projection;
			glm::mat4 view;
			glm::vec4 lightPos = glm::vec4(0.0f, 2.5f, 0.0f, 1.0f);
			glm::vec4 viewPos;
		} values;
	} shaderData;

	struct Pipelines {
		VkPipeline vertexAttributesInterleaved;
		VkPipeline vertexAttributesSeparate;
	} pipelines;
	VkPipelineLayout pipelineLayout;

	struct DescriptorSetLayouts {
		VkDescriptorSetLayout matrices;
		VkDescriptorSetLayout textures;
	} descriptorSetLayouts;
	VkDescriptorSet descriptorSet;

	struct Scene {
		std::vector<Image> images;
		std::vector<Texture> textures;
		std::vector<Material> materials;
	} scene;

	VulkanExample();
	~VulkanExample();
	virtual void getEnabledFeatures();
	void buildCommandBuffers();
	void uploadVertexData();
	void loadglTFFile(std::string filename);
	void loadAssets();
	void setupDescriptors();
	void preparePipelines();
	void prepareUniformBuffers();
	void updateUniformBuffers();
	void prepare();
	void loadSceneNode(const tinygltf::Node& inputNode, const tinygltf::Model& input, Node* parent);
	void drawSceneNode(VkCommandBuffer commandBuffer, Node node);
	virtual void render();
	virtual void OnUpdateUIOverlay(vks::UIOverlay* overlay);
};