/*
* Vulkan Example - glTF skinned animation
*
* Copyright (C) 2020 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

/*
 * Shows how to load and display an animated scene from a glTF file using vertex skinning
 * See the accompanying README.md for a short tutorial on the data structures and functions required for vertex skinning
 *
 * For details on how glTF 2.0 works, see the official spec at https://github.com/KhronosGroup/glTF/tree/master/specification/2.0
 *
 * If you are looking for a complete glTF implementation, check out https://github.com/SaschaWillems/Vulkan-glTF-PBR/
 */

#include "gltfskinning.h"

/*

	 glTF model class

	 Contains everything required to render a skinned glTF model in Vulkan
	 This class is simplified compared to glTF's feature set but retains the basic glTF structure required for this sample

 */

/*
	 Get a node's local matrix from the current translation, rotation and scale values
	 These are calculated from the current animation an need to be calculated dynamically
 */
glm::mat4 VulkanglTFModel::Node::getLocalMatrix()
{
	return glm::translate(glm::mat4(1.0f), translation) * glm::mat4(rotation) * glm::scale(glm::mat4(1.0f), scale) * matrix;
}

/*
	Release all Vulkan resources acquired for the model
*/
VulkanglTFModel::~VulkanglTFModel()
{
	vkDestroyBuffer(vulkanDevice->logicalDevice, vertices.buffer, nullptr);
	vkFreeMemory(vulkanDevice->logicalDevice, vertices.memory, nullptr);
	vkDestroyBuffer(vulkanDevice->logicalDevice, indices.buffer, nullptr);
	vkFreeMemory(vulkanDevice->logicalDevice, indices.memory, nullptr);
	for (Image image : images)
	{
		vkDestroyImageView(vulkanDevice->logicalDevice, image.texture.view, nullptr);
		vkDestroyImage(vulkanDevice->logicalDevice, image.texture.image, nullptr);
		vkDestroySampler(vulkanDevice->logicalDevice, image.texture.sampler, nullptr);
		vkFreeMemory(vulkanDevice->logicalDevice, image.texture.deviceMemory, nullptr);
	}
	for (Skin skin : skins)
	{
		skin.ssbo.destroy();
	}
}

/*
	glTF loading functions

	The following functions take a glTF input model loaded via tinyglTF and converts all required data into our own structures
*/

void VulkanglTFModel::loadImages(tinygltf::Model &input)
{
	// Images can be stored inside the glTF (which is the case for the sample model), so instead of directly
	// loading them from disk, we fetch them from the glTF loader and upload the buffers
	images.resize(input.images.size());
	for (size_t i = 0; i < input.images.size(); i++)
	{
		tinygltf::Image &glTFImage = input.images[i];
		// Get the image data from the glTF loader
		unsigned char *buffer       = nullptr;
		VkDeviceSize   bufferSize   = 0;
		bool           deleteBuffer = false;
		// We convert RGB-only images to RGBA, as most devices don't support RGB-formats in Vulkan
		if (glTFImage.component == 3)
		{
			bufferSize          = glTFImage.width * glTFImage.height * 4;
			buffer              = new unsigned char[bufferSize];
			unsigned char *rgba = buffer;
			unsigned char *rgb  = &glTFImage.image[0];
			for (size_t i = 0; i < glTFImage.width * glTFImage.height; ++i)
			{
				memcpy(rgba, rgb, sizeof(unsigned char) * 3);
				rgba += 4;
				rgb += 3;
			}
			deleteBuffer = true;
		}
		else
		{
			buffer     = &glTFImage.image[0];
			bufferSize = glTFImage.image.size();
		}
		// Load texture from image buffer
		images[i].texture.fromBuffer(buffer, bufferSize, VK_FORMAT_R8G8B8A8_UNORM, glTFImage.width, glTFImage.height, vulkanDevice, copyQueue);
		if (deleteBuffer)
		{
			delete[] buffer;
		}
	}
}

void VulkanglTFModel::loadTextures(tinygltf::Model &input)
{
	textures.resize(input.textures.size());
	for (size_t i = 0; i < input.textures.size(); i++)
	{
		textures[i].imageIndex = input.textures[i].source;
	}
}

void VulkanglTFModel::loadMaterials(tinygltf::Model &input)
{
	materials.resize(input.materials.size());
	for (size_t i = 0; i < input.materials.size(); i++)
	{
		// We only read the most basic properties required for our sample
		tinygltf::Material glTFMaterial = input.materials[i];
		// Get the base color factor
		if (glTFMaterial.values.find("baseColorFactor") != glTFMaterial.values.end())
		{
			materials[i].baseColorFactor = glm::make_vec4(glTFMaterial.values["baseColorFactor"].ColorFactor().data());
		}
		// Get base color texture index
		if (glTFMaterial.values.find("baseColorTexture") != glTFMaterial.values.end())
		{
			materials[i].baseColorTextureIndex = glTFMaterial.values["baseColorTexture"].TextureIndex();
		}
	}
}

// Helper functions for locating glTF nodes

VulkanglTFModel::Node *VulkanglTFModel::findNode(Node *parent, uint32_t index)
{
	Node *nodeFound = nullptr;
	if (parent->index == index)
	{
		return parent;
	}
	for (auto &child : parent->children)
	{
		nodeFound = findNode(child, index);
		if (nodeFound)
		{
			break;
		}
	}
	return nodeFound;
}

VulkanglTFModel::Node *VulkanglTFModel::nodeFromIndex(uint32_t index)
{
	Node *nodeFound = nullptr;
	for (auto &node : nodes)
	{
		nodeFound = findNode(node, index);
		if (nodeFound)
		{
			break;
		}
	}
	return nodeFound;
}

// POI: Load the skins from the glTF model
void VulkanglTFModel::loadSkins(tinygltf::Model &input)
{
	skins.resize(input.skins.size());

	for (size_t i = 0; i < input.skins.size(); i++)
	{
		tinygltf::Skin glTFSkin = input.skins[i];

		skins[i].name = glTFSkin.name;
		// Find the root node of the skeleton
		skins[i].skeletonRoot = nodeFromIndex(glTFSkin.skeleton);

		// Find joint nodes
		for (int jointIndex : glTFSkin.joints)
		{
			Node *node = nodeFromIndex(jointIndex);
			if (node)
			{
				skins[i].joints.push_back(node);
			}
		}

		// Get the inverse bind matrices from the buffer associated to this skin
		if (glTFSkin.inverseBindMatrices > -1)
		{
			const tinygltf::Accessor &  accessor   = input.accessors[glTFSkin.inverseBindMatrices];
			const tinygltf::BufferView &bufferView = input.bufferViews[accessor.bufferView];
			const tinygltf::Buffer &    buffer     = input.buffers[bufferView.buffer];
			skins[i].inverseBindMatrices.resize(accessor.count);
			memcpy(skins[i].inverseBindMatrices.data(), &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(glm::mat4));

			// Store inverse bind matrices for this skin in a shader storage buffer object
			// To keep this sample simple, we create a host visible shader storage buffer
			VK_CHECK_RESULT(vulkanDevice->createBuffer(
			    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			    &skins[i].ssbo,
			    sizeof(glm::mat4) * skins[i].inverseBindMatrices.size(),
			    skins[i].inverseBindMatrices.data()));
			VK_CHECK_RESULT(skins[i].ssbo.map());
		}
	}
}

// POI: Load the animations from the glTF model
void VulkanglTFModel::loadAnimations(tinygltf::Model &input)
{
	animations.resize(input.animations.size());

	for (size_t i = 0; i < input.animations.size(); i++)
	{
		tinygltf::Animation glTFAnimation = input.animations[i];
		animations[i].name                = glTFAnimation.name;

		// Samplers
		animations[i].samplers.resize(glTFAnimation.samplers.size());
		for (size_t j = 0; j < glTFAnimation.samplers.size(); j++)
		{
			tinygltf::AnimationSampler glTFSampler = glTFAnimation.samplers[j];
			AnimationSampler &         dstSampler  = animations[i].samplers[j];
			dstSampler.interpolation               = glTFSampler.interpolation;

			// Read sampler keyframe input time values
			{
				const tinygltf::Accessor &  accessor   = input.accessors[glTFSampler.input];
				const tinygltf::BufferView &bufferView = input.bufferViews[accessor.bufferView];
				const tinygltf::Buffer &    buffer     = input.buffers[bufferView.buffer];
				const void *                dataPtr    = &buffer.data[accessor.byteOffset + bufferView.byteOffset];
				const float *               buf        = static_cast<const float *>(dataPtr);
				for (size_t index = 0; index < accessor.count; index++)
				{
					dstSampler.inputs.push_back(buf[index]);
				}
				// Adjust animation's start and end times
				for (auto input : animations[i].samplers[j].inputs)
				{
					if (input < animations[i].start)
					{
						animations[i].start = input;
					};
					if (input > animations[i].end)
					{
						animations[i].end = input;
					}
				}
			}

			// Read sampler keyframe output translate/rotate/scale values
			{
				const tinygltf::Accessor &  accessor   = input.accessors[glTFSampler.output];
				const tinygltf::BufferView &bufferView = input.bufferViews[accessor.bufferView];
				const tinygltf::Buffer &    buffer     = input.buffers[bufferView.buffer];
				const void *                dataPtr    = &buffer.data[accessor.byteOffset + bufferView.byteOffset];
				switch (accessor.type)
				{
					case TINYGLTF_TYPE_VEC3: {
						const glm::vec3 *buf = static_cast<const glm::vec3 *>(dataPtr);
						for (size_t index = 0; index < accessor.count; index++)
						{
							dstSampler.outputsVec4.push_back(glm::vec4(buf[index], 0.0f));
						}
						break;
					}
					case TINYGLTF_TYPE_VEC4: {
						const glm::vec4 *buf = static_cast<const glm::vec4 *>(dataPtr);
						for (size_t index = 0; index < accessor.count; index++)
						{
							dstSampler.outputsVec4.push_back(buf[index]);
						}
						break;
					}
					default: {
						std::cout << "unknown type" << std::endl;
						break;
					}
				}
			}
		}

		// Channels
		animations[i].channels.resize(glTFAnimation.channels.size());
		for (size_t j = 0; j < glTFAnimation.channels.size(); j++)
		{
			tinygltf::AnimationChannel glTFChannel = glTFAnimation.channels[j];
			AnimationChannel &         dstChannel  = animations[i].channels[j];
			dstChannel.path                        = glTFChannel.target_path;
			dstChannel.samplerIndex                = glTFChannel.sampler;
			dstChannel.node                        = nodeFromIndex(glTFChannel.target_node);
		}
	}
}

void VulkanglTFModel::loadNode(const tinygltf::Node &inputNode, const tinygltf::Model &input, VulkanglTFModel::Node *parent, uint32_t nodeIndex, std::vector<uint32_t> &indexBuffer, std::vector<VulkanglTFModel::Vertex> &vertexBuffer)
{
	VulkanglTFModel::Node *node = new VulkanglTFModel::Node{};
	node->parent                = parent;
	node->matrix                = glm::mat4(1.0f);
	node->index                 = nodeIndex;
	node->skin                  = inputNode.skin;

	// Get the local node matrix
	// It's either made up from translation, rotation, scale or a 4x4 matrix
	if (inputNode.translation.size() == 3)
	{
		node->translation = glm::make_vec3(inputNode.translation.data());
	}
	if (inputNode.rotation.size() == 4)
	{
		glm::quat q    = glm::make_quat(inputNode.rotation.data());
		node->rotation = glm::mat4(q);
	}
	if (inputNode.scale.size() == 3)
	{
		node->scale = glm::make_vec3(inputNode.scale.data());
	}
	if (inputNode.matrix.size() == 16)
	{
		node->matrix = glm::make_mat4x4(inputNode.matrix.data());
	};

	// Load node's children
	if (inputNode.children.size() > 0)
	{
		for (size_t i = 0; i < inputNode.children.size(); i++)
		{
			loadNode(input.nodes[inputNode.children[i]], input, node, inputNode.children[i], indexBuffer, vertexBuffer);
		}
	}

	// If the node contains mesh data, we load vertices and indices from the buffers
	// In glTF this is done via accessors and buffer views
	if (inputNode.mesh > -1)
	{
		const tinygltf::Mesh mesh = input.meshes[inputNode.mesh];
		// Iterate through all primitives of this node's mesh
		for (size_t i = 0; i < mesh.primitives.size(); i++)
		{
			const tinygltf::Primitive &glTFPrimitive = mesh.primitives[i];
			uint32_t                   firstIndex    = static_cast<uint32_t>(indexBuffer.size());
			uint32_t                   vertexStart   = static_cast<uint32_t>(vertexBuffer.size());
			uint32_t                   indexCount    = 0;
			bool                       hasSkin       = false;
			// Vertices
			{
				const float *   positionBuffer     = nullptr;
				const float *   normalsBuffer      = nullptr;
				const float *   texCoordsBuffer    = nullptr;
				const uint16_t *jointIndicesBuffer = nullptr;
				const float *   jointWeightsBuffer = nullptr;
				size_t          vertexCount        = 0;

				// Get buffer data for vertex normals
				if (glTFPrimitive.attributes.find("POSITION") != glTFPrimitive.attributes.end())
				{
					const tinygltf::Accessor &  accessor = input.accessors[glTFPrimitive.attributes.find("POSITION")->second];
					const tinygltf::BufferView &view     = input.bufferViews[accessor.bufferView];
					positionBuffer                       = reinterpret_cast<const float *>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
					vertexCount                          = accessor.count;
				}
				// Get buffer data for vertex normals
				if (glTFPrimitive.attributes.find("NORMAL") != glTFPrimitive.attributes.end())
				{
					const tinygltf::Accessor &  accessor = input.accessors[glTFPrimitive.attributes.find("NORMAL")->second];
					const tinygltf::BufferView &view     = input.bufferViews[accessor.bufferView];
					normalsBuffer                        = reinterpret_cast<const float *>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
				}
				// Get buffer data for vertex texture coordinates
				// glTF supports multiple sets, we only load the first one
				if (glTFPrimitive.attributes.find("TEXCOORD_0") != glTFPrimitive.attributes.end())
				{
					const tinygltf::Accessor &  accessor = input.accessors[glTFPrimitive.attributes.find("TEXCOORD_0")->second];
					const tinygltf::BufferView &view     = input.bufferViews[accessor.bufferView];
					texCoordsBuffer                      = reinterpret_cast<const float *>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
				}

				// POI: Get buffer data required for vertex skinning
				// Get vertex joint indices
				if (glTFPrimitive.attributes.find("JOINTS_0") != glTFPrimitive.attributes.end())
				{
					const tinygltf::Accessor &  accessor = input.accessors[glTFPrimitive.attributes.find("JOINTS_0")->second];
					const tinygltf::BufferView &view     = input.bufferViews[accessor.bufferView];
					jointIndicesBuffer                   = reinterpret_cast<const uint16_t *>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
				}
				// Get vertex joint weights
				if (glTFPrimitive.attributes.find("WEIGHTS_0") != glTFPrimitive.attributes.end())
				{
					const tinygltf::Accessor &  accessor = input.accessors[glTFPrimitive.attributes.find("WEIGHTS_0")->second];
					const tinygltf::BufferView &view     = input.bufferViews[accessor.bufferView];
					jointWeightsBuffer                   = reinterpret_cast<const float *>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
				}

				hasSkin = (jointIndicesBuffer && jointWeightsBuffer);

				// Append data to model's vertex buffer
				for (size_t v = 0; v < vertexCount; v++)
				{
					Vertex vert{};
					vert.pos          = glm::vec4(glm::make_vec3(&positionBuffer[v * 3]), 1.0f);
					vert.normal       = glm::normalize(glm::vec3(normalsBuffer ? glm::make_vec3(&normalsBuffer[v * 3]) : glm::vec3(0.0f)));
					vert.uv           = texCoordsBuffer ? glm::make_vec2(&texCoordsBuffer[v * 2]) : glm::vec3(0.0f);
					vert.color        = glm::vec3(1.0f);
					vert.jointIndices = hasSkin ? glm::vec4(glm::make_vec4(&jointIndicesBuffer[v * 4])) : glm::vec4(0.0f);
					vert.jointWeights = hasSkin ? glm::make_vec4(&jointWeightsBuffer[v * 4]) : glm::vec4(0.0f);
					vertexBuffer.push_back(vert);
				}
			}
			// Indices
			{
				const tinygltf::Accessor &  accessor   = input.accessors[glTFPrimitive.indices];
				const tinygltf::BufferView &bufferView = input.bufferViews[accessor.bufferView];
				const tinygltf::Buffer &    buffer     = input.buffers[bufferView.buffer];

				indexCount += static_cast<uint32_t>(accessor.count);

				// glTF supports different component types of indices
				switch (accessor.componentType)
				{
					case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
						uint32_t *buf = new uint32_t[accessor.count];
						memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint32_t));
						for (size_t index = 0; index < accessor.count; index++)
						{
							indexBuffer.push_back(buf[index] + vertexStart);
						}
						break;
					}
					case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
						uint16_t *buf = new uint16_t[accessor.count];
						memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint16_t));
						for (size_t index = 0; index < accessor.count; index++)
						{
							indexBuffer.push_back(buf[index] + vertexStart);
						}
						break;
					}
					case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
						uint8_t *buf = new uint8_t[accessor.count];
						memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint8_t));
						for (size_t index = 0; index < accessor.count; index++)
						{
							indexBuffer.push_back(buf[index] + vertexStart);
						}
						break;
					}
					default:
						std::cerr << "Index component type " << accessor.componentType << " not supported!" << std::endl;
						return;
				}
			}
			Primitive primitive{};
			primitive.firstIndex    = firstIndex;
			primitive.indexCount    = indexCount;
			primitive.materialIndex = glTFPrimitive.material;
			node->mesh.primitives.push_back(primitive);
		}
	}

	if (parent)
	{
		parent->children.push_back(node);
	}
	else
	{
		nodes.push_back(node);
	}
}

/*
	glTF vertex skinning functions
*/

// POI: Traverse the node hierarchy to the top-most parent to get the local matrix of the given node
glm::mat4 VulkanglTFModel::getNodeMatrix(VulkanglTFModel::Node *node)
{
	glm::mat4              nodeMatrix    = node->getLocalMatrix();
	VulkanglTFModel::Node *currentParent = node->parent;
	while (currentParent)
	{
		nodeMatrix    = currentParent->getLocalMatrix() * nodeMatrix;
		currentParent = currentParent->parent;
	}
	return nodeMatrix;
}

// POI: Update the joint matrices from the current animation frame and pass them to the GPU
void VulkanglTFModel::updateJoints(VulkanglTFModel::Node *node)
{
	if (node->skin > -1)
	{
		// Update the joint matrices
		glm::mat4              inverseTransform = glm::inverse(getNodeMatrix(node));
		Skin                   skin             = skins[node->skin];
		size_t                 numJoints        = (uint32_t) skin.joints.size();
		std::vector<glm::mat4> jointMatrices(numJoints);
		for (size_t i = 0; i < numJoints; i++)
		{
			jointMatrices[i] = getNodeMatrix(skin.joints[i]) * skin.inverseBindMatrices[i];
			jointMatrices[i] = inverseTransform * jointMatrices[i];
		}
		// Update ssbo
		skin.ssbo.copyTo(jointMatrices.data(), jointMatrices.size() * sizeof(glm::mat4));
	}

	for (auto &child : node->children)
	{
		updateJoints(child);
	}
}

// POI: Update the current animation
void VulkanglTFModel::updateAnimation(float deltaTime)
{
	if (activeAnimation > static_cast<uint32_t>(animations.size()) - 1)
	{
		std::cout << "No animation with index " << activeAnimation << std::endl;
		return;
	}
	Animation &animation = animations[activeAnimation];
	animation.currentTime += deltaTime;
	if (animation.currentTime > animation.end)
	{
		animation.currentTime -= animation.end;
	}

	for (auto &channel : animation.channels)
	{
		AnimationSampler &sampler = animation.samplers[channel.samplerIndex];
		for (size_t i = 0; i < sampler.inputs.size() - 1; i++)
		{
			if (sampler.interpolation != "LINEAR")
			{
				std::cout << "This sample only supports linear interpolations\n";
				continue;
			}

			// Get the input keyframe values for the current time stamp
			if ((animation.currentTime >= sampler.inputs[i]) && (animation.currentTime <= sampler.inputs[i + 1]))
			{
				float a = (animation.currentTime - sampler.inputs[i]) / (sampler.inputs[i + 1] - sampler.inputs[i]);
				if (channel.path == "translation")
				{
					channel.node->translation = glm::mix(sampler.outputsVec4[i], sampler.outputsVec4[i + 1], a);
				}
				if (channel.path == "rotation")
				{
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

					channel.node->rotation = glm::normalize(glm::slerp(q1, q2, a));
				}
				if (channel.path == "scale")
				{
					channel.node->scale = glm::mix(sampler.outputsVec4[i], sampler.outputsVec4[i + 1], a);
				}
			}
		}
	}
	for (auto &node : nodes)
	{
		updateJoints(node);
	}
}

/*
	glTF rendering functions
*/

// Draw a single node including child nodes (if present)
void VulkanglTFModel::drawNode(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, VulkanglTFModel::Node node)
{
	if (node.mesh.primitives.size() > 0)
	{
		// Pass the node's matrix via push constants
		// Traverse the node hierarchy to the top-most parent to get the final matrix of the current node
		glm::mat4              nodeMatrix    = node.matrix;
		VulkanglTFModel::Node *currentParent = node.parent;
		while (currentParent)
		{
			nodeMatrix    = currentParent->matrix * nodeMatrix;
			currentParent = currentParent->parent;
		}
		// Pass the final matrix to the vertex shader using push constants
		vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &nodeMatrix);
		// Bind SSBO with skin data for this node to set 1
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &skins[node.skin].descriptorSet, 0, nullptr);
		for (VulkanglTFModel::Primitive &primitive : node.mesh.primitives)
		{
			if (primitive.indexCount > 0)
			{
				// Get the texture index for this primitive
				VulkanglTFModel::Texture texture = textures[materials[primitive.materialIndex].baseColorTextureIndex];
				// Bind the descriptor for the current primitive's texture to set 2
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 2, 1, &images[texture.imageIndex].descriptorSet, 0, nullptr);
				vkCmdDrawIndexed(commandBuffer, primitive.indexCount, 1, primitive.firstIndex, 0, 0);
			}
		}
	}
	for (auto &child : node.children)
	{
		drawNode(commandBuffer, pipelineLayout, *child);
	}
}

// Draw the glTF scene starting at the top-level-nodes
void VulkanglTFModel::draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout)
{
	// All vertices and indices are stored in single buffers, so we only need to bind once
	VkDeviceSize offsets[1] = {0};
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices.buffer, offsets);
	vkCmdBindIndexBuffer(commandBuffer, indices.buffer, 0, VK_INDEX_TYPE_UINT32);
	// Render all nodes at top-level
	for (auto &node : nodes)
	{
		drawNode(commandBuffer, pipelineLayout, *node);
	}
}

/*

	Vulkan Example class

*/

VulkanExample::VulkanExample() :
    VulkanExampleBase(ENABLE_VALIDATION)
{
	title        = "glTF vertex skinning";
	camera.type  = Camera::CameraType::lookat;
	camera.flipY = true;
	camera.setPosition(glm::vec3(0.0f, 0.75f, -2.0f));
	camera.setRotation(glm::vec3(0.0f, 0.0f, 0.0f));
	camera.setPerspective(60.0f, (float) width / (float) height, 0.1f, 256.0f);
}

VulkanExample::~VulkanExample()
{
	vkDestroyPipeline(device, pipelines.solid, nullptr);
	if (pipelines.wireframe != VK_NULL_HANDLE)
	{
		vkDestroyPipeline(device, pipelines.wireframe, nullptr);
	}

	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	vkDestroyDescriptorSetLayout(device, descriptorSetLayouts.matrices, nullptr);
	vkDestroyDescriptorSetLayout(device, descriptorSetLayouts.textures, nullptr);
	vkDestroyDescriptorSetLayout(device, descriptorSetLayouts.jointMatrices, nullptr);

	shaderData.buffer.destroy();
}

void VulkanExample::getEnabledFeatures()
{
	// Fill mode non solid is required for wireframe display
	if (deviceFeatures.fillModeNonSolid)
	{
		enabledFeatures.fillModeNonSolid = VK_TRUE;
	};
}

void VulkanExample::buildCommandBuffers()
{
	VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

	VkClearValue clearValues[2];
	clearValues[0].color = {{0.25f, 0.25f, 0.25f, 1.0f}};
	;
	clearValues[1].depthStencil = {1.0f, 0};

	VkRenderPassBeginInfo renderPassBeginInfo    = vks::initializers::renderPassBeginInfo();
	renderPassBeginInfo.renderPass               = renderPass;
	renderPassBeginInfo.renderArea.offset.x      = 0;
	renderPassBeginInfo.renderArea.offset.y      = 0;
	renderPassBeginInfo.renderArea.extent.width  = width;
	renderPassBeginInfo.renderArea.extent.height = height;
	renderPassBeginInfo.clearValueCount          = 2;
	renderPassBeginInfo.pClearValues             = clearValues;

	const VkViewport viewport = vks::initializers::viewport((float) width, (float) height, 0.0f, 1.0f);
	const VkRect2D   scissor  = vks::initializers::rect2D(width, height, 0, 0);

	for (int32_t i = 0; i < drawCmdBuffers.size(); ++i)
	{
		renderPassBeginInfo.framebuffer = frameBuffers[i];
		VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo));
		vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);
		vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);
		// Bind scene matrices descriptor to set 0
		vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
		vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, wireframe ? pipelines.wireframe : pipelines.solid);
		glTFModel.draw(drawCmdBuffers[i], pipelineLayout);
		drawUI(drawCmdBuffers[i]);
		vkCmdEndRenderPass(drawCmdBuffers[i]);
		VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
	}
}

void VulkanExample::loadglTFFile(std::string filename)
{
	tinygltf::Model    glTFInput;
	tinygltf::TinyGLTF gltfContext;
	std::string        error, warning;

	this->device = device;

#if defined(__ANDROID__)
	// On Android all assets are packed with the apk in a compressed form, so we need to open them using the asset manager
	// We let tinygltf handle this, by passing the asset manager of our app
	tinygltf::asset_manager = androidApp->activity->assetManager;
#endif
	bool fileLoaded = gltfContext.LoadASCIIFromFile(&glTFInput, &error, &warning, filename);

	// Pass some Vulkan resources required for setup and rendering to the glTF model loading class
	glTFModel.vulkanDevice = vulkanDevice;
	glTFModel.copyQueue    = queue;

	std::vector<uint32_t>                indexBuffer;
	std::vector<VulkanglTFModel::Vertex> vertexBuffer;

	if (fileLoaded)
	{
		glTFModel.loadImages(glTFInput);
		glTFModel.loadMaterials(glTFInput);
		glTFModel.loadTextures(glTFInput);
		const tinygltf::Scene &scene = glTFInput.scenes[0];
		for (size_t i = 0; i < scene.nodes.size(); i++)
		{
			const tinygltf::Node node = glTFInput.nodes[scene.nodes[i]];
			glTFModel.loadNode(node, glTFInput, nullptr, scene.nodes[i], indexBuffer, vertexBuffer);
		}
		glTFModel.loadSkins(glTFInput);
		glTFModel.loadAnimations(glTFInput);
		// Calculate initial pose
		for (auto node : glTFModel.nodes)
		{
			glTFModel.updateJoints(node);
		}
	}
	else
	{
		vks::tools::exitFatal("Could not open the glTF file.\n\nThe file is part of the additional asset pack.\n\nRun \"download_assets.py\" in the repository root to download the latest version.", -1);
		return;
	}

	// Create and upload vertex and index buffer
	size_t vertexBufferSize = vertexBuffer.size() * sizeof(VulkanglTFModel::Vertex);
	size_t indexBufferSize  = indexBuffer.size() * sizeof(uint32_t);
	glTFModel.indices.count = static_cast<uint32_t>(indexBuffer.size());

	struct StagingBuffer
	{
		VkBuffer       buffer;
		VkDeviceMemory memory;
	} vertexStaging, indexStaging;

	// Create host visible staging buffers (source)
	VK_CHECK_RESULT(vulkanDevice->createBuffer(
	    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
	    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	    vertexBufferSize,
	    &vertexStaging.buffer,
	    &vertexStaging.memory,
	    vertexBuffer.data()));
	// Index data
	VK_CHECK_RESULT(vulkanDevice->createBuffer(
	    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
	    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	    indexBufferSize,
	    &indexStaging.buffer,
	    &indexStaging.memory,
	    indexBuffer.data()));

	// Create device local buffers (target)
	VK_CHECK_RESULT(vulkanDevice->createBuffer(
	    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	    vertexBufferSize,
	    &glTFModel.vertices.buffer,
	    &glTFModel.vertices.memory));
	VK_CHECK_RESULT(vulkanDevice->createBuffer(
	    VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	    indexBufferSize,
	    &glTFModel.indices.buffer,
	    &glTFModel.indices.memory));

	// Copy data from staging buffers (host) do device local buffer (gpu)
	VkCommandBuffer copyCmd    = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
	VkBufferCopy    copyRegion = {};
	copyRegion.size            = vertexBufferSize;
	vkCmdCopyBuffer(copyCmd, vertexStaging.buffer, glTFModel.vertices.buffer, 1, &copyRegion);
	copyRegion.size = indexBufferSize;
	vkCmdCopyBuffer(copyCmd, indexStaging.buffer, glTFModel.indices.buffer, 1, &copyRegion);
	vulkanDevice->flushCommandBuffer(copyCmd, queue, true);

	// Free staging resources
	vkDestroyBuffer(device, vertexStaging.buffer, nullptr);
	vkFreeMemory(device, vertexStaging.memory, nullptr);
	vkDestroyBuffer(device, indexStaging.buffer, nullptr);
	vkFreeMemory(device, indexStaging.memory, nullptr);
}

void VulkanExample::setupDescriptors()
{
	/*
		This sample uses separate descriptor sets (and layouts) for the matrices and materials (textures)
	*/

	std::vector<VkDescriptorPoolSize> poolSizes = {
	    vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1),
	    // One combined image sampler per material image/texture
	    vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(glTFModel.images.size())),
	    // One ssbo per skin
	    vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, static_cast<uint32_t>(glTFModel.skins.size())),
	};
	// Number of descriptor sets = One for the scene ubo + one per image + one per skin
	const uint32_t             maxSetCount        = static_cast<uint32_t>(glTFModel.images.size()) + static_cast<uint32_t>(glTFModel.skins.size()) + 1;
	VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, maxSetCount);
	VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));

	// Descriptor set layouts
	VkDescriptorSetLayoutBinding    setLayoutBinding{};
	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI = vks::initializers::descriptorSetLayoutCreateInfo(&setLayoutBinding, 1);

	// Descriptor set layout for passing matrices
	setLayoutBinding = vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0);
	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCI, nullptr, &descriptorSetLayouts.matrices));

	// Descriptor set layout for passing material textures
	setLayoutBinding = vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0);
	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCI, nullptr, &descriptorSetLayouts.textures));

	// Descriptor set layout for passing skin joint matrices
	setLayoutBinding = vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0);
	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCI, nullptr, &descriptorSetLayouts.jointMatrices));

	// The pipeline layout uses three sets:
	// Set 0 = Scene matrices (VS)
	// Set 1 = Joint matrices (VS)
	// Set 2 = Material texture (FS)
	std::array<VkDescriptorSetLayout, 3> setLayouts = {
	    descriptorSetLayouts.matrices,
	    descriptorSetLayouts.jointMatrices,
	    descriptorSetLayouts.textures};
	VkPipelineLayoutCreateInfo pipelineLayoutCI = vks::initializers::pipelineLayoutCreateInfo(setLayouts.data(), static_cast<uint32_t>(setLayouts.size()));

	// We will use push constants to push the local matrices of a primitive to the vertex shader
	VkPushConstantRange pushConstantRange = vks::initializers::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), 0);
	// Push constant ranges are part of the pipeline layout
	pipelineLayoutCI.pushConstantRangeCount = 1;
	pipelineLayoutCI.pPushConstantRanges    = &pushConstantRange;
	VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &pipelineLayout));

	// Descriptor set for scene matrices
	VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayouts.matrices, 1);
	VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));
	VkWriteDescriptorSet writeDescriptorSet = vks::initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &shaderData.buffer.descriptor);
	vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);

	// Descriptor set for glTF model skin joint matrices
	for (auto &skin : glTFModel.skins)
	{
		const VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayouts.jointMatrices, 1);
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &skin.descriptorSet));
		VkWriteDescriptorSet writeDescriptorSet = vks::initializers::writeDescriptorSet(skin.descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, &skin.ssbo.descriptor);
		vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
	}

	// Descriptor sets for glTF model materials
	for (auto &image : glTFModel.images)
	{
		const VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayouts.textures, 1);
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &image.descriptorSet));
		VkWriteDescriptorSet writeDescriptorSet = vks::initializers::writeDescriptorSet(image.descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &image.texture.descriptor);
		vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
	}
}

void VulkanExample::preparePipelines()
{
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI   = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
	VkPipelineRasterizationStateCreateInfo rasterizationStateCI   = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
	VkPipelineColorBlendAttachmentState    blendAttachmentStateCI = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
	VkPipelineColorBlendStateCreateInfo    colorBlendStateCI      = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentStateCI);
	VkPipelineDepthStencilStateCreateInfo  depthStencilStateCI    = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
	VkPipelineViewportStateCreateInfo      viewportStateCI        = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
	VkPipelineMultisampleStateCreateInfo   multisampleStateCI     = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
	const std::vector<VkDynamicState>      dynamicStateEnables    = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
	VkPipelineDynamicStateCreateInfo       dynamicStateCI         = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables.data(), static_cast<uint32_t>(dynamicStateEnables.size()), 0);
	// Vertex input bindings and attributes
	const std::vector<VkVertexInputBindingDescription> vertexInputBindings = {
	    vks::initializers::vertexInputBindingDescription(0, sizeof(VulkanglTFModel::Vertex), VK_VERTEX_INPUT_RATE_VERTEX),
	};
	const std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = {
	    {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VulkanglTFModel::Vertex, pos)},
	    {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VulkanglTFModel::Vertex, normal)},
	    {2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VulkanglTFModel::Vertex, uv)},
	    {3, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VulkanglTFModel::Vertex, color)},
	    // POI: Per-Vertex Joint indices and weights are passed to the vertex shader
	    {4, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(VulkanglTFModel::Vertex, jointIndices)},
	    {5, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(VulkanglTFModel::Vertex, jointWeights)},
	};

	VkPipelineVertexInputStateCreateInfo vertexInputStateCI = vks::initializers::pipelineVertexInputStateCreateInfo();
	vertexInputStateCI.vertexBindingDescriptionCount        = static_cast<uint32_t>(vertexInputBindings.size());
	vertexInputStateCI.pVertexBindingDescriptions           = vertexInputBindings.data();
	vertexInputStateCI.vertexAttributeDescriptionCount      = static_cast<uint32_t>(vertexInputAttributes.size());
	vertexInputStateCI.pVertexAttributeDescriptions         = vertexInputAttributes.data();

	const std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = {
	    loadShader(getShadersPath() + "gltfskinning/skinnedmodel.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
	    loadShader(getShadersPath() + "gltfskinning/skinnedmodel.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT)};

	VkGraphicsPipelineCreateInfo pipelineCI = vks::initializers::pipelineCreateInfo(pipelineLayout, renderPass, 0);
	pipelineCI.pVertexInputState            = &vertexInputStateCI;
	pipelineCI.pInputAssemblyState          = &inputAssemblyStateCI;
	pipelineCI.pRasterizationState          = &rasterizationStateCI;
	pipelineCI.pColorBlendState             = &colorBlendStateCI;
	pipelineCI.pMultisampleState            = &multisampleStateCI;
	pipelineCI.pViewportState               = &viewportStateCI;
	pipelineCI.pDepthStencilState           = &depthStencilStateCI;
	pipelineCI.pDynamicState                = &dynamicStateCI;
	pipelineCI.stageCount                   = static_cast<uint32_t>(shaderStages.size());
	pipelineCI.pStages                      = shaderStages.data();

	// Solid rendering pipeline
	VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.solid));

	// Wire frame rendering pipeline
	if (deviceFeatures.fillModeNonSolid)
	{
		rasterizationStateCI.polygonMode = VK_POLYGON_MODE_LINE;
		rasterizationStateCI.lineWidth   = 1.0f;
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.wireframe));
	}
}

void VulkanExample::prepareUniformBuffers()
{
	VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &shaderData.buffer, sizeof(shaderData.values)));
	VK_CHECK_RESULT(shaderData.buffer.map());
	updateUniformBuffers();
}

void VulkanExample::updateUniformBuffers()
{
	shaderData.values.projection = camera.matrices.perspective;
	shaderData.values.model      = camera.matrices.view;
	memcpy(shaderData.buffer.mapped, &shaderData.values, sizeof(shaderData.values));
}

void VulkanExample::loadAssets()
{
	loadglTFFile(getAssetPath() + "models/CesiumMan/glTF/CesiumMan.gltf");
}

void VulkanExample::prepare()
{
	VulkanExampleBase::prepare();
	loadAssets();
	prepareUniformBuffers();
	setupDescriptors();
	preparePipelines();
	buildCommandBuffers();
	prepared = true;
}

void VulkanExample::render()
{
	renderFrame();
	if (camera.updated)
	{
		updateUniformBuffers();
	}
	// POI: Advance animation
	if (!paused)
	{
		glTFModel.updateAnimation(frameTimer);
	}
}

void VulkanExample::OnUpdateUIOverlay(vks::UIOverlay *overlay)
{
	if (overlay->header("Settings"))
	{
		if (overlay->checkBox("Wireframe", &wireframe))
		{
			buildCommandBuffers();
		}
	}
}

VULKAN_EXAMPLE_MAIN()