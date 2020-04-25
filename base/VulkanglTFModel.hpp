/*
* Vulkan glTF model and texture loading class based on tinyglTF (https://github.com/syoyo/tinygltf)
*
* Copyright (C) 2018 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#include <stdlib.h>
#include <string>
#include <fstream>
#include <vector>

#include "vulkan/vulkan.h"
#include "VulkanDevice.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf.h"

#if defined(__ANDROID__)
#include <android/asset_manager.h>
#endif

namespace vkglTF
{
	struct Node;

	/*
		glTF texture loading class
	*/
	struct Texture {
		vks::VulkanDevice *device;
		VkImage image;
		VkImageLayout imageLayout;
		VkDeviceMemory deviceMemory;
		VkImageView view;
		uint32_t width, height;
		uint32_t mipLevels;
		uint32_t layerCount;
		VkDescriptorImageInfo descriptor;
		VkSampler sampler;

		void updateDescriptor()
		{
			descriptor.sampler = sampler;
			descriptor.imageView = view;
			descriptor.imageLayout = imageLayout;
		}

		void destroy()
		{
			vkDestroyImageView(device->logicalDevice, view, nullptr);
			vkDestroyImage(device->logicalDevice, image, nullptr);
			vkFreeMemory(device->logicalDevice, deviceMemory, nullptr);
			vkDestroySampler(device->logicalDevice, sampler, nullptr);
		}

		/*
			Load a texture from a glTF image (stored as vector of chars loaded via stb_image)
			Also generates the mip chain as glTF images are stored as jpg or png without any mips
		*/
		void fromglTfImage(tinygltf::Image &gltfimage, vks::VulkanDevice *device, VkQueue copyQueue)
		{
			this->device = device;

			unsigned char* buffer = nullptr;
			VkDeviceSize bufferSize = 0;
			bool deleteBuffer = false;
			if (gltfimage.component == 3) {
				// Most devices don't support RGB only on Vulkan so convert if necessary
				// TODO: Check actual format support and transform only if required
				bufferSize = gltfimage.width * gltfimage.height * 4;
				buffer = new unsigned char[bufferSize];
				unsigned char* rgba = buffer;
				unsigned char* rgb = &gltfimage.image[0];
				for (size_t i = 0; i< gltfimage.width * gltfimage.height; ++i) {
					for (int32_t j = 0; j < 3; ++j) {
						rgba[j] = rgb[j];
					}
					rgba += 4;
					rgb += 3;
				}
				deleteBuffer = true;
			}
			else {
				buffer = &gltfimage.image[0];
				bufferSize = gltfimage.image.size();
			}

			VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

			VkFormatProperties formatProperties;

			width = gltfimage.width;
			height = gltfimage.height;
			mipLevels = static_cast<uint32_t>(floor(log2(std::max(width, height))) + 1.0);

			vkGetPhysicalDeviceFormatProperties(device->physicalDevice, format, &formatProperties);
			assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT);
			assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT);

			VkMemoryAllocateInfo memAllocInfo{};
			memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			VkMemoryRequirements memReqs{};

			VkBuffer stagingBuffer;
			VkDeviceMemory stagingMemory;

			VkBufferCreateInfo bufferCreateInfo{};
			bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferCreateInfo.size = bufferSize;
			bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			VK_CHECK_RESULT(vkCreateBuffer(device->logicalDevice, &bufferCreateInfo, nullptr, &stagingBuffer));
			vkGetBufferMemoryRequirements(device->logicalDevice, stagingBuffer, &memReqs);
			memAllocInfo.allocationSize = memReqs.size;
			memAllocInfo.memoryTypeIndex = device->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			VK_CHECK_RESULT(vkAllocateMemory(device->logicalDevice, &memAllocInfo, nullptr, &stagingMemory));
			VK_CHECK_RESULT(vkBindBufferMemory(device->logicalDevice, stagingBuffer, stagingMemory, 0));

			uint8_t *data;
			VK_CHECK_RESULT(vkMapMemory(device->logicalDevice, stagingMemory, 0, memReqs.size, 0, (void **)&data));
			memcpy(data, buffer, bufferSize);
			vkUnmapMemory(device->logicalDevice, stagingMemory);

			VkImageCreateInfo imageCreateInfo{};
			imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
			imageCreateInfo.format = format;
			imageCreateInfo.mipLevels = mipLevels;
			imageCreateInfo.arrayLayers = 1;
			imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
			imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageCreateInfo.extent = { width, height, 1 };
			imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			VK_CHECK_RESULT(vkCreateImage(device->logicalDevice, &imageCreateInfo, nullptr, &image));
			vkGetImageMemoryRequirements(device->logicalDevice, image, &memReqs);
			memAllocInfo.allocationSize = memReqs.size;
			memAllocInfo.memoryTypeIndex = device->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			VK_CHECK_RESULT(vkAllocateMemory(device->logicalDevice, &memAllocInfo, nullptr, &deviceMemory));
			VK_CHECK_RESULT(vkBindImageMemory(device->logicalDevice, image, deviceMemory, 0));

			VkCommandBuffer copyCmd = device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

			VkImageSubresourceRange subresourceRange = {};
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subresourceRange.levelCount = 1;
			subresourceRange.layerCount = 1;

			{
				VkImageMemoryBarrier imageMemoryBarrier{};
				imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				imageMemoryBarrier.srcAccessMask = 0;
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				imageMemoryBarrier.image = image;
				imageMemoryBarrier.subresourceRange = subresourceRange;
				vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
			}

			VkBufferImageCopy bufferCopyRegion = {};
			bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			bufferCopyRegion.imageSubresource.mipLevel = 0;
			bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
			bufferCopyRegion.imageSubresource.layerCount = 1;
			bufferCopyRegion.imageExtent.width = width;
			bufferCopyRegion.imageExtent.height = height;
			bufferCopyRegion.imageExtent.depth = 1;

			vkCmdCopyBufferToImage(copyCmd, stagingBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferCopyRegion);

			{
				VkImageMemoryBarrier imageMemoryBarrier{};
				imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				imageMemoryBarrier.image = image;
				imageMemoryBarrier.subresourceRange = subresourceRange;
				vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
			}

			device->flushCommandBuffer(copyCmd, copyQueue, true);

			vkFreeMemory(device->logicalDevice, stagingMemory, nullptr);
			vkDestroyBuffer(device->logicalDevice, stagingBuffer, nullptr);

			// Generate the mip chain (glTF uses jpg and png, so we need to create this manually)
			VkCommandBuffer blitCmd = device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
			for (uint32_t i = 1; i < mipLevels; i++) {
				VkImageBlit imageBlit{};

				imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageBlit.srcSubresource.layerCount = 1;
				imageBlit.srcSubresource.mipLevel = i - 1;
				imageBlit.srcOffsets[1].x = int32_t(width >> (i - 1));
				imageBlit.srcOffsets[1].y = int32_t(height >> (i - 1));
				imageBlit.srcOffsets[1].z = 1;

				imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageBlit.dstSubresource.layerCount = 1;
				imageBlit.dstSubresource.mipLevel = i;
				imageBlit.dstOffsets[1].x = int32_t(width >> i);
				imageBlit.dstOffsets[1].y = int32_t(height >> i);
				imageBlit.dstOffsets[1].z = 1;

				VkImageSubresourceRange mipSubRange = {};
				mipSubRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				mipSubRange.baseMipLevel = i;
				mipSubRange.levelCount = 1;
				mipSubRange.layerCount = 1;

				{
					VkImageMemoryBarrier imageMemoryBarrier{};
					imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
					imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
					imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
					imageMemoryBarrier.srcAccessMask = 0;
					imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					imageMemoryBarrier.image = image;
					imageMemoryBarrier.subresourceRange = mipSubRange;
					vkCmdPipelineBarrier(blitCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
				}

				vkCmdBlitImage(blitCmd, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_LINEAR);

				{
					VkImageMemoryBarrier imageMemoryBarrier{};
					imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
					imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
					imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
					imageMemoryBarrier.image = image;
					imageMemoryBarrier.subresourceRange = mipSubRange;
					vkCmdPipelineBarrier(blitCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
				}
			}

			subresourceRange.levelCount = mipLevels;
			imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			{
				VkImageMemoryBarrier imageMemoryBarrier{};
				imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				imageMemoryBarrier.image = image;
				imageMemoryBarrier.subresourceRange = subresourceRange;
				vkCmdPipelineBarrier(blitCmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
			}

			device->flushCommandBuffer(blitCmd, copyQueue, true);

			VkSamplerCreateInfo samplerInfo{};
			samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerInfo.magFilter = VK_FILTER_LINEAR;
			samplerInfo.minFilter = VK_FILTER_LINEAR;
			samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
			samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
			samplerInfo.maxAnisotropy = 1.0;
			samplerInfo.anisotropyEnable = VK_FALSE;
			samplerInfo.maxLod = (float)mipLevels;
			samplerInfo.maxAnisotropy = 8.0f;
			samplerInfo.anisotropyEnable = VK_TRUE;
			VK_CHECK_RESULT(vkCreateSampler(device->logicalDevice, &samplerInfo, nullptr, &sampler));

			VkImageViewCreateInfo viewInfo{};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = image;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = format;
			viewInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewInfo.subresourceRange.layerCount = 1;
			viewInfo.subresourceRange.levelCount = mipLevels;
			VK_CHECK_RESULT(vkCreateImageView(device->logicalDevice, &viewInfo, nullptr, &view));

			descriptor.sampler = sampler;
			descriptor.imageView = view;
			descriptor.imageLayout = imageLayout;
		}
	};

	/*
		glTF material class
	*/
	// TODO: Base class and inheritance
	struct Material {		
		enum AlphaMode{ ALPHAMODE_OPAQUE, ALPHAMODE_MASK, ALPHAMODE_BLEND };
		AlphaMode alphaMode = ALPHAMODE_OPAQUE;
		float alphaCutoff = 1.0f;
		float metallicFactor = 1.0f;
		float roughnessFactor = 1.0f;
		glm::vec4 baseColorFactor = glm::vec4(1.0f);
		vkglTF::Texture *baseColorTexture;
		vkglTF::Texture *metallicRoughnessTexture;
		vkglTF::Texture *normalTexture;
		vkglTF::Texture *occlusionTexture;
		vkglTF::Texture *emissiveTexture;

		vkglTF::Texture *specularGlossinessTexture;
		vkglTF::Texture *diffuseTexture;

		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
	};

	/*
		glTF primitive
	*/
	struct Primitive {
		uint32_t firstIndex;
		uint32_t indexCount;
		uint32_t firstVertex;
		uint32_t vertexCount;
		Material &material;

		struct Dimensions {
			glm::vec3 min = glm::vec3(FLT_MAX);
			glm::vec3 max = glm::vec3(-FLT_MAX);
			glm::vec3 size;
			glm::vec3 center;
			float radius;
		} dimensions;

		void setDimensions(glm::vec3 min, glm::vec3 max) {
			dimensions.min = min;
			dimensions.max = max;
			dimensions.size = max - min;
			dimensions.center = (min + max) / 2.0f;
			dimensions.radius = glm::distance(min, max) / 2.0f;
		}

		Primitive(uint32_t firstIndex, uint32_t indexCount, Material &material) : firstIndex(firstIndex), indexCount(indexCount), material(material) {};
	};

	/*
		glTF mesh
	*/
	struct Mesh {
		vks::VulkanDevice *device;

		std::vector<Primitive*> primitives;
		std::string name;

		struct UniformBuffer {
			VkBuffer buffer;
			VkDeviceMemory memory;
			VkDescriptorBufferInfo descriptor;
			VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
			void *mapped;
		} uniformBuffer;

		struct UniformBlock {
			glm::mat4 matrix;
			glm::mat4 jointMatrix[64]{};
			float jointcount{ 0 };
		} uniformBlock;

		Mesh(vks::VulkanDevice *device, glm::mat4 matrix) {
			this->device = device;
			this->uniformBlock.matrix = matrix;
			VK_CHECK_RESULT(device->createBuffer(
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				sizeof(uniformBlock),
				&uniformBuffer.buffer,
				&uniformBuffer.memory,
				&uniformBlock));
			VK_CHECK_RESULT(vkMapMemory(device->logicalDevice, uniformBuffer.memory, 0, sizeof(uniformBlock), 0, &uniformBuffer.mapped));
			uniformBuffer.descriptor = { uniformBuffer.buffer, 0, sizeof(uniformBlock) };
		};

		~Mesh() {
			vkDestroyBuffer(device->logicalDevice, uniformBuffer.buffer, nullptr);
			vkFreeMemory(device->logicalDevice, uniformBuffer.memory, nullptr);
		}

	};

	/*
		glTF skin
	*/
	struct Skin {
		std::string name;
		Node *skeletonRoot = nullptr;
		std::vector<glm::mat4> inverseBindMatrices;
		std::vector<Node*> joints;
	};

	/*
		glTF node
	*/
	struct Node {
		Node *parent;
		uint32_t index;
		std::vector<Node*> children;
		glm::mat4 matrix;
		std::string name;
		Mesh *mesh;
		Skin *skin;
		int32_t skinIndex = -1;
		glm::vec3 translation{};
		glm::vec3 scale{ 1.0f };
		glm::quat rotation{};

		glm::mat4 localMatrix() {
			return glm::translate(glm::mat4(1.0f), translation) * glm::mat4(rotation) * glm::scale(glm::mat4(1.0f), scale) * matrix;
		}

		glm::mat4 getMatrix() {
			glm::mat4 m = localMatrix();
			vkglTF::Node *p = parent;
			while (p) {
				m = p->localMatrix() * m;
				p = p->parent;
			}
			return m;
		}

		void update() {
			if (mesh) {
				glm::mat4 m = getMatrix();
				if (skin) {
					mesh->uniformBlock.matrix = m;
					// Update join matrices
					glm::mat4 inverseTransform = glm::inverse(m);
					for (size_t i = 0; i < skin->joints.size(); i++) {
						vkglTF::Node *jointNode = skin->joints[i];
						glm::mat4 jointMat = jointNode->getMatrix() * skin->inverseBindMatrices[i];
						jointMat = inverseTransform * jointMat;
						mesh->uniformBlock.jointMatrix[i] = jointMat;
					}
					mesh->uniformBlock.jointcount = (float)skin->joints.size();
					memcpy(mesh->uniformBuffer.mapped, &mesh->uniformBlock, sizeof(mesh->uniformBlock));
				} else {
					memcpy(mesh->uniformBuffer.mapped, &m, sizeof(glm::mat4));
				}
			}

			for (auto& child : children) {
				child->update();
			}
		}

		~Node() {
			if (mesh) {
				delete mesh;
			}
			for (auto& child : children) {
				delete child;
			}
		}
	};

	/*
		glTF animation channel
	*/
	struct AnimationChannel {
		enum PathType { TRANSLATION, ROTATION, SCALE };
		PathType path;
		Node *node;
		uint32_t samplerIndex;
	};

	/*
		glTF animation sampler
	*/
	struct AnimationSampler {
		enum InterpolationType { LINEAR, STEP, CUBICSPLINE };
		InterpolationType interpolation;
		std::vector<float> inputs;
		std::vector<glm::vec4> outputsVec4;
	};

	/*
		glTF animation
	*/
	struct Animation {
		std::string name;
		std::vector<AnimationSampler> samplers;
		std::vector<AnimationChannel> channels;
		float start = std::numeric_limits<float>::max();
		float end = std::numeric_limits<float>::min();
	};

	/*
		glTF default vertex layout with easy Vulkan mapping functions
	*/
	enum class VertexComponent { Position, Normal, UV, Color, Joint0, Weight0 };

	struct Vertex {
		glm::vec3 pos;
		glm::vec3 normal;
		glm::vec2 uv;
		glm::vec4 color;
		glm::vec4 joint0;
		glm::vec4 weight0;
		static VkVertexInputBindingDescription inputBindingDescription(uint32_t binding) {
			return VkVertexInputBindingDescription({ binding, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX });
		}
		static VkVertexInputAttributeDescription inputAttributeDescription(uint32_t binding, uint32_t location, VertexComponent component) {
			switch (component) {
				case VertexComponent::Position: 
					return VkVertexInputAttributeDescription({ location, binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos) });
				case VertexComponent::Normal:
					return VkVertexInputAttributeDescription({ location, binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal) });
				case VertexComponent::UV:
					return VkVertexInputAttributeDescription({ location, binding, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv) });
				case VertexComponent::Color:
					return VkVertexInputAttributeDescription({ location, binding, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, color) });
				case VertexComponent::Joint0:
					return VkVertexInputAttributeDescription({ location, binding, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, joint0) });
				case VertexComponent::Weight0:
					return VkVertexInputAttributeDescription({ location, binding, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, weight0) });
			}
		}
	};

	typedef enum FileLoadingFlags { 
		None = 0, 
		PreTransformVertices = 1, 
	};

	/*
		glTF model loading and rendering class
	*/
	struct Model {

		vks::VulkanDevice *device;
		VkDescriptorPool descriptorPool;
		VkDescriptorSetLayout descriptorSetLayout;

		struct Vertices {
			VkBuffer buffer;
			VkDeviceMemory memory;
		} vertices;
		struct Indices {
			int count;
			VkBuffer buffer;
			VkDeviceMemory memory;
		} indices;

		std::vector<Node*> nodes;
		std::vector<Node*> linearNodes;

		std::vector<Skin*> skins;

		std::vector<Texture> textures;
		std::vector<Material> materials;
		std::vector<Animation> animations;

		struct Dimensions {
			glm::vec3 min = glm::vec3(FLT_MAX);
			glm::vec3 max = glm::vec3(-FLT_MAX);
			glm::vec3 size;
			glm::vec3 center;
			float radius;
		} dimensions;

		bool metallicRoughnessWorkflow = true;

		Model() {};

		~Model() 
		{
			vkDestroyBuffer(device->logicalDevice, vertices.buffer, nullptr);
			vkFreeMemory(device->logicalDevice, vertices.memory, nullptr);
			vkDestroyBuffer(device->logicalDevice, indices.buffer, nullptr);
			vkFreeMemory(device->logicalDevice, indices.memory, nullptr);
			for (auto texture : textures) {
				texture.destroy();
			}
			for (auto node : nodes) {
				delete node;
			}
			vkDestroyDescriptorSetLayout(device->logicalDevice, descriptorSetLayout, nullptr);
			vkDestroyDescriptorPool(device->logicalDevice, descriptorPool, nullptr);
		}

		void loadNode(vkglTF::Node *parent, const tinygltf::Node &node, uint32_t nodeIndex, const tinygltf::Model &model, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer, float globalscale)
		{
			vkglTF::Node *newNode = new Node{};
			newNode->index = nodeIndex;
			newNode->parent = parent;
			newNode->name = node.name;
			newNode->skinIndex = node.skin;
			newNode->matrix = glm::mat4(1.0f);

			// Generate local node matrix
			glm::vec3 translation = glm::vec3(0.0f);
			if (node.translation.size() == 3) {
				translation = glm::make_vec3(node.translation.data());
				newNode->translation = translation;
			}
			glm::mat4 rotation = glm::mat4(1.0f);
			if (node.rotation.size() == 4) {
				glm::quat q = glm::make_quat(node.rotation.data());
				newNode->rotation = glm::mat4(q);
			}
			glm::vec3 scale = glm::vec3(1.0f);
			if (node.scale.size() == 3) {
				scale = glm::make_vec3(node.scale.data());
				newNode->scale = scale;
			}
			if (node.matrix.size() == 16) {
				newNode->matrix = glm::make_mat4x4(node.matrix.data());
				if (globalscale != 1.0f) {
					//newNode->matrix = glm::scale(newNode->matrix, glm::vec3(globalscale));
				}
			};

			// Node with children
			if (node.children.size() > 0) {
				for (auto i = 0; i < node.children.size(); i++) {
					loadNode(newNode, model.nodes[node.children[i]], node.children[i], model, indexBuffer, vertexBuffer, globalscale);
				}
			}

			// Node contains mesh data
			if (node.mesh > -1) {
				const tinygltf::Mesh mesh = model.meshes[node.mesh];
				Mesh *newMesh = new Mesh(device, newNode->matrix);
				newMesh->name = mesh.name;
				for (size_t j = 0; j < mesh.primitives.size(); j++) {
					const tinygltf::Primitive &primitive = mesh.primitives[j];
					if (primitive.indices < 0) {
						continue;
					}
					uint32_t indexStart = static_cast<uint32_t>(indexBuffer.size());
					uint32_t vertexStart = static_cast<uint32_t>(vertexBuffer.size());
					uint32_t indexCount = 0;
					uint32_t vertexCount = 0;
					glm::vec3 posMin{};
					glm::vec3 posMax{};
					bool hasSkin = false;
					// Vertices
					{
						const float *bufferPos = nullptr;
						const float *bufferNormals = nullptr;
						const float *bufferTexCoords = nullptr;
						const float* bufferColors = nullptr;
						uint32_t numColorComponents;
						const uint16_t *bufferJoints = nullptr;
						const float *bufferWeights = nullptr;

						// Position attribute is required
						assert(primitive.attributes.find("POSITION") != primitive.attributes.end());

						const tinygltf::Accessor &posAccessor = model.accessors[primitive.attributes.find("POSITION")->second];
						const tinygltf::BufferView &posView = model.bufferViews[posAccessor.bufferView];
						bufferPos = reinterpret_cast<const float *>(&(model.buffers[posView.buffer].data[posAccessor.byteOffset + posView.byteOffset]));
						posMin = glm::vec3(posAccessor.minValues[0], posAccessor.minValues[1], posAccessor.minValues[2]);
						posMax = glm::vec3(posAccessor.maxValues[0], posAccessor.maxValues[1], posAccessor.maxValues[2]);

						if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
							const tinygltf::Accessor &normAccessor = model.accessors[primitive.attributes.find("NORMAL")->second];
							const tinygltf::BufferView &normView = model.bufferViews[normAccessor.bufferView];
							bufferNormals = reinterpret_cast<const float *>(&(model.buffers[normView.buffer].data[normAccessor.byteOffset + normView.byteOffset]));
						}

						if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
							const tinygltf::Accessor &uvAccessor = model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
							const tinygltf::BufferView &uvView = model.bufferViews[uvAccessor.bufferView];
							bufferTexCoords = reinterpret_cast<const float *>(&(model.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
						}
						if (primitive.attributes.find("COLOR_0") != primitive.attributes.end()) {
							const tinygltf::Accessor& colorAccessor = model.accessors[primitive.attributes.find("COLOR_0")->second];
							const tinygltf::BufferView& colorView = model.bufferViews[colorAccessor.bufferView];
							// Color buffer are either of type vec3 or vec4
							numColorComponents = colorAccessor.type == TINYGLTF_PARAMETER_TYPE_FLOAT_VEC3 ? 3 : 4;
							bufferColors = reinterpret_cast<const float*>(&(model.buffers[colorView.buffer].data[colorAccessor.byteOffset + colorView.byteOffset]));
						}

						// Skinning
						// Joints
						if (primitive.attributes.find("JOINTS_0") != primitive.attributes.end()) {
							const tinygltf::Accessor &jointAccessor = model.accessors[primitive.attributes.find("JOINTS_0")->second];
							const tinygltf::BufferView &jointView = model.bufferViews[jointAccessor.bufferView];
							bufferJoints = reinterpret_cast<const uint16_t *>(&(model.buffers[jointView.buffer].data[jointAccessor.byteOffset + jointView.byteOffset]));
						}

						if (primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end()) {
							const tinygltf::Accessor &uvAccessor = model.accessors[primitive.attributes.find("WEIGHTS_0")->second];
							const tinygltf::BufferView &uvView = model.bufferViews[uvAccessor.bufferView];
							bufferWeights = reinterpret_cast<const float *>(&(model.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
						}

						hasSkin = (bufferJoints && bufferWeights);

						vertexCount = static_cast<uint32_t>(posAccessor.count);

						for (size_t v = 0; v < posAccessor.count; v++) {
							Vertex vert{};
							vert.pos = glm::vec4(glm::make_vec3(&bufferPos[v * 3]), 1.0f);
							vert.normal = glm::normalize(glm::vec3(bufferNormals ? glm::make_vec3(&bufferNormals[v * 3]) : glm::vec3(0.0f)));
							vert.uv = bufferTexCoords ? glm::make_vec2(&bufferTexCoords[v * 2]) : glm::vec3(0.0f);
							if (bufferColors) {
								switch (numColorComponents) {
									case 3: 
										vert.color = glm::vec4(glm::make_vec3(&bufferColors[v * 3]), 1.0f);
									case 4:
										vert.color = glm::make_vec4(&bufferColors[v * 4]);
								}
							}
							else {
								vert.color = glm::vec4(1.0f);
							}
							vert.joint0 = hasSkin ? glm::vec4(glm::make_vec4(&bufferJoints[v * 4])) : glm::vec4(0.0f);
							vert.weight0 = hasSkin ? glm::make_vec4(&bufferWeights[v * 4]) : glm::vec4(0.0f);
							vertexBuffer.push_back(vert);
						}
					}
					// Indices
					{
						const tinygltf::Accessor &accessor = model.accessors[primitive.indices];
						const tinygltf::BufferView &bufferView = model.bufferViews[accessor.bufferView];
						const tinygltf::Buffer &buffer = model.buffers[bufferView.buffer];

						indexCount = static_cast<uint32_t>(accessor.count);

						switch (accessor.componentType) {
						case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
							uint32_t *buf = new uint32_t[accessor.count];
							memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint32_t));
							for (size_t index = 0; index < accessor.count; index++) {
								indexBuffer.push_back(buf[index] + vertexStart);
							}
							break;
						}
						case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
							uint16_t *buf = new uint16_t[accessor.count];
							memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint16_t));
							for (size_t index = 0; index < accessor.count; index++) {
								indexBuffer.push_back(buf[index] + vertexStart);
							}
							break;
						}
						case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
							uint8_t *buf = new uint8_t[accessor.count];
							memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint8_t));
							for (size_t index = 0; index < accessor.count; index++) {
								indexBuffer.push_back(buf[index] + vertexStart);
							}
							break;
						}
						default:
							std::cerr << "Index component type " << accessor.componentType << " not supported!" << std::endl;
							return;
						}
					}
					Primitive *newPrimitive = new Primitive(indexStart, indexCount, materials[primitive.material]);
					newPrimitive->firstVertex = vertexStart;
					newPrimitive->vertexCount = vertexCount;
					newPrimitive->setDimensions(posMin, posMax);
					newMesh->primitives.push_back(newPrimitive);
				}
				newNode->mesh = newMesh;
			}
			if (parent) {
				parent->children.push_back(newNode);
			} else {
				nodes.push_back(newNode);
			}
			linearNodes.push_back(newNode);
		}

		void loadSkins(tinygltf::Model &gltfModel)
		{
			for (tinygltf::Skin &source : gltfModel.skins) {
				Skin *newSkin = new Skin{};
				newSkin->name = source.name;
				
				// Find skeleton root node
				if (source.skeleton > -1) {
					newSkin->skeletonRoot = nodeFromIndex(source.skeleton);
				}

				// Find joint nodes
				for (int jointIndex : source.joints) {
					Node* node = nodeFromIndex(jointIndex);
					if (node) {
						newSkin->joints.push_back(nodeFromIndex(jointIndex));
					}
				}

				// Get inverse bind matrices from buffer
				if (source.inverseBindMatrices > -1) {
					const tinygltf::Accessor &accessor = gltfModel.accessors[source.inverseBindMatrices];
					const tinygltf::BufferView &bufferView = gltfModel.bufferViews[accessor.bufferView];
					const tinygltf::Buffer &buffer = gltfModel.buffers[bufferView.buffer];
					newSkin->inverseBindMatrices.resize(accessor.count);
					memcpy(newSkin->inverseBindMatrices.data(), &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(glm::mat4));
				}

				skins.push_back(newSkin);
			}
		}

		void loadImages(tinygltf::Model &gltfModel, vks::VulkanDevice *device, VkQueue transferQueue)
		{
			for (tinygltf::Image &image : gltfModel.images) {
				vkglTF::Texture texture;
				texture.fromglTfImage(image, device, transferQueue);
				textures.push_back(texture);
			}
		}

		void loadMaterials(tinygltf::Model &gltfModel)
		{
			for (tinygltf::Material &mat : gltfModel.materials) {
				vkglTF::Material material{};
				if (mat.values.find("baseColorTexture") != mat.values.end()) {
					material.baseColorTexture = &textures[gltfModel.textures[mat.values["baseColorTexture"].TextureIndex()].source];
				}
				// Metallic roughness workflow
				if (mat.values.find("metallicRoughnessTexture") != mat.values.end()) {
					material.metallicRoughnessTexture = &textures[gltfModel.textures[mat.values["metallicRoughnessTexture"].TextureIndex()].source];
				}
				if (mat.values.find("roughnessFactor") != mat.values.end()) {
					material.roughnessFactor = static_cast<float>(mat.values["roughnessFactor"].Factor());
				}
				if (mat.values.find("metallicFactor") != mat.values.end()) {
					material.metallicFactor = static_cast<float>(mat.values["metallicFactor"].Factor());
				}
				if (mat.values.find("baseColorFactor") != mat.values.end()) {
					material.baseColorFactor = glm::make_vec4(mat.values["baseColorFactor"].ColorFactor().data());
				}				
				if (mat.additionalValues.find("normalTexture") != mat.additionalValues.end()) {
					material.normalTexture = &textures[gltfModel.textures[mat.additionalValues["normalTexture"].TextureIndex()].source];
				}
				if (mat.additionalValues.find("emissiveTexture") != mat.additionalValues.end()) {
					material.emissiveTexture = &textures[gltfModel.textures[mat.additionalValues["emissiveTexture"].TextureIndex()].source];
				}
				if (mat.additionalValues.find("occlusionTexture") != mat.additionalValues.end()) {
					material.occlusionTexture = &textures[gltfModel.textures[mat.additionalValues["occlusionTexture"].TextureIndex()].source];
				}
				if (mat.additionalValues.find("alphaMode") != mat.additionalValues.end()) {
					tinygltf::Parameter param = mat.additionalValues["alphaMode"];
					if (param.string_value == "BLEND") {
						material.alphaMode = Material::ALPHAMODE_BLEND;
					}
					if (param.string_value == "MASK") {
						material.alphaMode = Material::ALPHAMODE_MASK;
					}
				}
				if (mat.additionalValues.find("alphaCutoff") != mat.additionalValues.end()) {
					material.alphaCutoff = static_cast<float>(mat.additionalValues["alphaCutoff"].Factor());
				}

				materials.push_back(material);
			}
		}

		void loadAnimations(tinygltf::Model &gltfModel)
		{
			for (tinygltf::Animation &anim : gltfModel.animations) {
				vkglTF::Animation animation{};
				animation.name = anim.name;
				if (anim.name.empty()) {
					animation.name = std::to_string(animations.size());
				}

				// Samplers
				for (auto &samp : anim.samplers) {
					vkglTF::AnimationSampler sampler{};

					if (samp.interpolation == "LINEAR") {
						sampler.interpolation = AnimationSampler::InterpolationType::LINEAR;
					}
					if (samp.interpolation == "STEP") {
						sampler.interpolation = AnimationSampler::InterpolationType::STEP;
					}
					if (samp.interpolation == "CUBICSPLINE") {
						sampler.interpolation = AnimationSampler::InterpolationType::CUBICSPLINE;
					}

					// Read sampler input time values
					{
						const tinygltf::Accessor &accessor = gltfModel.accessors[samp.input];
						const tinygltf::BufferView &bufferView = gltfModel.bufferViews[accessor.bufferView];
						const tinygltf::Buffer &buffer = gltfModel.buffers[bufferView.buffer];

						assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

						float *buf = new float[accessor.count];
						memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(float));
						for (size_t index = 0; index < accessor.count; index++) {
							sampler.inputs.push_back(buf[index]);
						}

						for (auto input : sampler.inputs) {
							if (input < animation.start) {
								animation.start = input;
							};
							if (input > animation.end) {
								animation.end = input;
							}
						}
					}

					// Read sampler output T/R/S values 
					{
						const tinygltf::Accessor &accessor = gltfModel.accessors[samp.output];
						const tinygltf::BufferView &bufferView = gltfModel.bufferViews[accessor.bufferView];
						const tinygltf::Buffer &buffer = gltfModel.buffers[bufferView.buffer];

						assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

						switch (accessor.type) {
						case TINYGLTF_TYPE_VEC3: {
							glm::vec3 *buf = new glm::vec3[accessor.count];
							memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(glm::vec3));
							for (size_t index = 0; index < accessor.count; index++) {
								sampler.outputsVec4.push_back(glm::vec4(buf[index], 0.0f));
							}
							break;
						}
						case TINYGLTF_TYPE_VEC4: {
							glm::vec4 *buf = new glm::vec4[accessor.count];
							memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(glm::vec4));
							for (size_t index = 0; index < accessor.count; index++) {
								sampler.outputsVec4.push_back(buf[index]);
							}
							break;
						}
						default: {
							std::cout << "unknown type" << std::endl;
							break;
						}
						}
					}

					animation.samplers.push_back(sampler);
				}

				// Channels
				for (auto &source: anim.channels) {
					vkglTF::AnimationChannel channel{};

					if (source.target_path == "rotation") {
						channel.path = AnimationChannel::PathType::ROTATION;
					}
					if (source.target_path == "translation") {
						channel.path = AnimationChannel::PathType::TRANSLATION;
					}
					if (source.target_path == "scale") {
						channel.path = AnimationChannel::PathType::SCALE;
					}
					if (source.target_path == "weights") {
						std::cout << "weights not yet supported, skipping channel" << std::endl;
						continue;
					}
					channel.samplerIndex = source.sampler;
					channel.node = nodeFromIndex(source.target_node);
					if (!channel.node) {
						continue;
					}

					animation.channels.push_back(channel);
				}

				animations.push_back(animation);
			}
		}

		void loadFromFile(std::string filename, vks::VulkanDevice *device, VkQueue transferQueue, uint32_t fileLoadingFlags = vkglTF::FileLoadingFlags::None, float scale = 1.0f)
		{
			tinygltf::Model gltfModel;
			tinygltf::TinyGLTF gltfContext;
			std::string error, warning;

			this->device = device;

#if defined(__ANDROID__)
			AAsset* asset = AAssetManager_open(androidApp->activity->assetManager, filename.c_str(), AASSET_MODE_STREAMING);
			assert(asset);
			size_t size = AAsset_getLength(asset);
			assert(size > 0);
			char* fileData = new char[size];
			AAsset_read(asset, fileData, size);
			AAsset_close(asset);
			std::string baseDir;
			bool fileLoaded = gltfContext.LoadASCIIFromString(&gltfModel, &error, &warning, fileData, size, baseDir);
			free(fileData);
#else
			bool fileLoaded = gltfContext.LoadASCIIFromFile(&gltfModel, &error, &warning, filename);
#endif
			std::vector<uint32_t> indexBuffer;
			std::vector<Vertex> vertexBuffer;

			if (fileLoaded) {
				loadImages(gltfModel, device, transferQueue);
				loadMaterials(gltfModel);
				const tinygltf::Scene &scene = gltfModel.scenes[gltfModel.defaultScene > -1 ? gltfModel.defaultScene : 0];
				for (size_t i = 0; i < scene.nodes.size(); i++) {
					const tinygltf::Node node = gltfModel.nodes[scene.nodes[i]];
					loadNode(nullptr, node, scene.nodes[i], gltfModel, indexBuffer, vertexBuffer, scale);
				}
				if (gltfModel.animations.size() > 0) {
					loadAnimations(gltfModel);
				}
				loadSkins(gltfModel);

				for (auto node : linearNodes) {
					// Assign skins
					if (node->skinIndex > -1) {
						node->skin = skins[node->skinIndex];
					}
					// Initial pose
					if (node->mesh) {
						node->update();
					}
				}
			}
			else {
				// TODO: throw
				std::cerr << "Could not load gltf file: " << error << std::endl;
				return;
			}

			// Pre-transform all vertices by the node matrix hierarchy if requested
			if (fileLoadingFlags & FileLoadingFlags::PreTransformVertices) {
				for (Node* node : linearNodes) {
					const glm::mat4 localMatrix = node->getMatrix();
					if (node->mesh) {
						for (Primitive* primitive : node->mesh->primitives) {
							for (uint32_t i = 0; i < primitive->vertexCount; i++) {
								vertexBuffer[primitive->firstVertex + i].pos = glm::vec3(localMatrix * glm::vec4(vertexBuffer[primitive->firstVertex + i].pos, 1.0f));
							}
						}
					}
				}
			}

			for (auto extension : gltfModel.extensionsUsed) {
				if (extension == "KHR_materials_pbrSpecularGlossiness") {
					std::cout << "Required extension: " << extension;
					metallicRoughnessWorkflow = false;
				}
			}

			size_t vertexBufferSize = vertexBuffer.size() * sizeof(Vertex);
			size_t indexBufferSize = indexBuffer.size() * sizeof(uint32_t);
			indices.count = static_cast<uint32_t>(indexBuffer.size());

			assert((vertexBufferSize > 0) && (indexBufferSize > 0));

			struct StagingBuffer {
				VkBuffer buffer;
				VkDeviceMemory memory;
			} vertexStaging, indexStaging;

			// Create staging buffers
			// Vertex data
			VK_CHECK_RESULT(device->createBuffer(
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				vertexBufferSize,
				&vertexStaging.buffer,
				&vertexStaging.memory,
				vertexBuffer.data()));
			// Index data
			VK_CHECK_RESULT(device->createBuffer(
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				indexBufferSize,
				&indexStaging.buffer,
				&indexStaging.memory,
				indexBuffer.data()));

			// Create device local buffers
			// Vertex buffer
			VK_CHECK_RESULT(device->createBuffer(
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				vertexBufferSize,
				&vertices.buffer,
				&vertices.memory));
			// Index buffer
			VK_CHECK_RESULT(device->createBuffer(
				VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				indexBufferSize,
				&indices.buffer,
				&indices.memory));

			// Copy from staging buffers
			VkCommandBuffer copyCmd = device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

			VkBufferCopy copyRegion = {};

			copyRegion.size = vertexBufferSize;
			vkCmdCopyBuffer(copyCmd, vertexStaging.buffer, vertices.buffer, 1, &copyRegion);

			copyRegion.size = indexBufferSize;
			vkCmdCopyBuffer(copyCmd, indexStaging.buffer, indices.buffer, 1, &copyRegion);

			device->flushCommandBuffer(copyCmd, transferQueue, true);

			vkDestroyBuffer(device->logicalDevice, vertexStaging.buffer, nullptr);
			vkFreeMemory(device->logicalDevice, vertexStaging.memory, nullptr);
			vkDestroyBuffer(device->logicalDevice, indexStaging.buffer, nullptr);
			vkFreeMemory(device->logicalDevice, indexStaging.memory, nullptr);

			getSceneDimensions();

			// Setup descriptors
			uint32_t uboCount{ 0 };
			for (auto node : linearNodes) {
				if (node->mesh) {
					uboCount++;
				}
			}
			std::vector<VkDescriptorPoolSize> poolSizes = {
				vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, uboCount),
			};
			VkDescriptorPoolCreateInfo descriptorPoolCI = vks::initializers::descriptorPoolCreateInfo(static_cast<uint32_t>(poolSizes.size()), poolSizes.data(), uboCount);
			VK_CHECK_RESULT(vkCreateDescriptorPool(device->logicalDevice, &descriptorPoolCI, nullptr, &descriptorPool));

			std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
				vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
			};
			VkDescriptorSetLayoutCreateInfo descriptorLayoutCI{};
			descriptorLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			descriptorLayoutCI.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
			descriptorLayoutCI.pBindings = setLayoutBindings.data();
			VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device->logicalDevice, &descriptorLayoutCI, nullptr, &descriptorSetLayout));
			for (auto node : nodes) {
				prepareNodeDescriptor(node, descriptorSetLayout);
			}
		}

		void drawNode(Node *node, VkCommandBuffer commandBuffer)
		{
			if (node->mesh) {
				for (Primitive *primitive : node->mesh->primitives) {
					vkCmdDrawIndexed(commandBuffer, primitive->indexCount, 1, primitive->firstIndex, 0, 0);
				}
			}
			for (auto& child : node->children) {
				drawNode(child, commandBuffer);
			}
		}

		void draw(VkCommandBuffer commandBuffer)
		{
			const VkDeviceSize offsets[1] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices.buffer, offsets);
			vkCmdBindIndexBuffer(commandBuffer, indices.buffer, 0, VK_INDEX_TYPE_UINT32);
			for (auto& node : nodes) {
				drawNode(node, commandBuffer);
			}
		}

		void getNodeDimensions(Node *node, glm::vec3 &min, glm::vec3 &max)
		{
			if (node->mesh) {
				for (Primitive *primitive : node->mesh->primitives) {
					glm::vec4 locMin = glm::vec4(primitive->dimensions.min, 1.0f) * node->getMatrix();
					glm::vec4 locMax = glm::vec4(primitive->dimensions.max, 1.0f) * node->getMatrix();
					if (locMin.x < min.x) { min.x = locMin.x; }
					if (locMin.y < min.y) { min.y = locMin.y; }
					if (locMin.z < min.z) { min.z = locMin.z; }
					if (locMax.x > max.x) { max.x = locMax.x; }
					if (locMax.y > max.y) { max.y = locMax.y; }
					if (locMax.z > max.z) { max.z = locMax.z; }
				}
			}
			for (auto child : node->children) {
				getNodeDimensions(child, min, max);
			}
		}

		void getSceneDimensions()
		{
			dimensions.min = glm::vec3(FLT_MAX);
			dimensions.max = glm::vec3(-FLT_MAX);
			for (auto node : nodes) {
				getNodeDimensions(node, dimensions.min, dimensions.max);
			}
			dimensions.size = dimensions.max - dimensions.min;
			dimensions.center = (dimensions.min + dimensions.max) / 2.0f;
			dimensions.radius = glm::distance(dimensions.min, dimensions.max) / 2.0f;
		}

		void updateAnimation(uint32_t index, float time) 
		{
			if (index > static_cast<uint32_t>(animations.size()) - 1) {
				std::cout << "No animation with index " << index << std::endl;
				return;
			}
			Animation &animation = animations[index];

			bool updated = false;
			for (auto& channel : animation.channels) {
				vkglTF::AnimationSampler &sampler = animation.samplers[channel.samplerIndex];
				if (sampler.inputs.size() > sampler.outputsVec4.size()) {
					continue;
				}

				for (auto i = 0; i < sampler.inputs.size() - 1; i++) {
					if ((time >= sampler.inputs[i]) && (time <= sampler.inputs[i + 1])) {
						float u = std::max(0.0f, time - sampler.inputs[i]) / (sampler.inputs[i + 1] - sampler.inputs[i]);
						if (u <= 1.0f) {
							switch (channel.path) {
							case vkglTF::AnimationChannel::PathType::TRANSLATION: {
								glm::vec4 trans = glm::mix(sampler.outputsVec4[i], sampler.outputsVec4[i + 1], u);
								channel.node->translation = glm::vec3(trans);
								break;
							}
							case vkglTF::AnimationChannel::PathType::SCALE: {
								glm::vec4 trans = glm::mix(sampler.outputsVec4[i], sampler.outputsVec4[i + 1], u);
								channel.node->scale = glm::vec3(trans);
								break;
							}
							case vkglTF::AnimationChannel::PathType::ROTATION: {
								glm::quat q1;
								q1.x = sampler.outputsVec4[i].x;
								q1.y = sampler.outputsVec4[i].y;
								q1.z = sampler.outputsVec4[i].z;
								q1.w = sampler.outputsVec4[i].w;
								glm::quat q2;
								q2.x = sampler.outputsVec4[i + 1].x;
								q2.y = sampler.outputsVec4[i + 1].y;
								q2.z = sampler.outputsVec4[i + 1].z;
								q2.w = sampler.outputsVec4[i + 1].w;
								channel.node->rotation = glm::normalize(glm::slerp(q1, q2, u));
								break;
							}
							}
							updated = true;
						}
					}
				}
			}
			if (updated) {
				for (auto &node : nodes) {
					node->update();
				}
			}
		}

		/*
			Helper functions
		*/
		Node* findNode(Node *parent, uint32_t index) {
			Node* nodeFound = nullptr;
			if (parent->index == index) {
				return parent;
			}
			for (auto& child : parent->children) {
				nodeFound = findNode(child, index);
				if (nodeFound) {
					break;
				}
			}
			return nodeFound;
		}

		Node* nodeFromIndex(uint32_t index) {
			Node* nodeFound = nullptr;
			for (auto &node : nodes) {
				nodeFound = findNode(node, index);
				if (nodeFound) {
					break;
				}
			}
			return nodeFound;
		}

		void prepareNodeDescriptor(vkglTF::Node *node, VkDescriptorSetLayout descriptorSetLayout) {
			if (node->mesh) {
				VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
				descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
				descriptorSetAllocInfo.descriptorPool = descriptorPool;
				descriptorSetAllocInfo.pSetLayouts = &descriptorSetLayout;
				descriptorSetAllocInfo.descriptorSetCount = 1;
				VK_CHECK_RESULT(vkAllocateDescriptorSets(device->logicalDevice, &descriptorSetAllocInfo, &node->mesh->uniformBuffer.descriptorSet));

				VkWriteDescriptorSet writeDescriptorSet{};
				writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				writeDescriptorSet.descriptorCount = 1;
				writeDescriptorSet.dstSet = node->mesh->uniformBuffer.descriptorSet;
				writeDescriptorSet.dstBinding = 0;
				writeDescriptorSet.pBufferInfo = &node->mesh->uniformBuffer.descriptor;

				vkUpdateDescriptorSets(device->logicalDevice, 1, &writeDescriptorSet, 0, nullptr);
			}
			for (auto& child : node->children) {
				prepareNodeDescriptor(child, descriptorSetLayout);
			}
		}
	};
}