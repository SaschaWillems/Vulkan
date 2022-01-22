/*
 * Vulkan Example - Passing vertex attributes using interleaved and separate buffers
 *
 * Copyright (C) 2022 by Sascha Willems - www.saschawillems.de
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */

#include "vertexattributes.h"

void VulkanExample::loadSceneNode(const tinygltf::Node& inputNode, const tinygltf::Model& input, Node* parent)
{
	Node node{};

	// Get the local node matrix
	// It's either made up from translation, rotation, scale or a 4x4 matrix
	node.matrix = glm::mat4(1.0f);
	if (inputNode.translation.size() == 3) {
		node.matrix = glm::translate(node.matrix, glm::vec3(glm::make_vec3(inputNode.translation.data())));
	}
	if (inputNode.rotation.size() == 4) {
		glm::quat q = glm::make_quat(inputNode.rotation.data());
		node.matrix *= glm::mat4(q);
	}
	if (inputNode.scale.size() == 3) {
		node.matrix = glm::scale(node.matrix, glm::vec3(glm::make_vec3(inputNode.scale.data())));
	}
	if (inputNode.matrix.size() == 16) {
		node.matrix = glm::make_mat4x4(inputNode.matrix.data());
	};

	// Load node's children
	if (inputNode.children.size() > 0) {
		for (size_t i = 0; i < inputNode.children.size(); i++) {
			loadSceneNode(input.nodes[inputNode.children[i]], input, &node);
		}
	}

	// If the node contains mesh data, we load vertices and indices from the buffers
	// In glTF this is done via accessors and buffer views
	if (inputNode.mesh > -1) {
		const tinygltf::Mesh mesh = input.meshes[inputNode.mesh];
		// Iterate through all primitives of this node's mesh
		for (size_t i = 0; i < mesh.primitives.size(); i++) {
			const tinygltf::Primitive& glTFPrimitive = mesh.primitives[i];
			uint32_t firstIndex = static_cast<uint32_t>(indexBuffer.size());
			uint32_t vertexStart = static_cast<uint32_t>(vertexBuffer.size());
			uint32_t indexCount = 0;

			// Vertex attributes
			const float* positionBuffer = nullptr;
			const float* normalsBuffer = nullptr;
			const float* texCoordsBuffer = nullptr;
			const float* tangentsBuffer = nullptr;
			size_t vertexCount = 0;

			// Anonymous functions to simplify buffer view access
			auto getBuffer = [glTFPrimitive, input, &vertexCount](const std::string attributeName, const float* &bufferTarget) {
				if (glTFPrimitive.attributes.find(attributeName) != glTFPrimitive.attributes.end()) {
					const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.attributes.find(attributeName)->second];
					const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
					bufferTarget = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
					if (attributeName == "POSITION") {
						vertexCount = accessor.count;
					}
				}
			};

			// Get buffer pointers to the vertex attributes used in this sample
			getBuffer("POSITION", positionBuffer);
			getBuffer("NORMAL", normalsBuffer);
			getBuffer("TEXCOORD_0", texCoordsBuffer);
			getBuffer("TANGENT", tangentsBuffer);

			// Append attributes to the vertex buffers
			for (size_t v = 0; v < vertexCount; v++) {

				// Append interleaved attributes
				Vertex vert{};
				vert.pos = glm::vec4(glm::make_vec3(&positionBuffer[v * 3]), 1.0f);
				vert.normal = glm::normalize(glm::vec3(normalsBuffer ? glm::make_vec3(&normalsBuffer[v * 3]) : glm::vec3(0.0f)));
				vert.uv = texCoordsBuffer ? glm::make_vec2(&texCoordsBuffer[v * 2]) : glm::vec3(0.0f);
				vert.tangent = tangentsBuffer ? glm::make_vec4(&tangentsBuffer[v * 4]) : glm::vec4(0.0f);
				vertexBuffer.push_back(vert);

				// Append separate attributes
				vertexAttributeBuffers.pos.push_back(glm::make_vec3(&positionBuffer[v * 3]));
				vertexAttributeBuffers.normal.push_back(glm::normalize(glm::vec3(normalsBuffer ? glm::make_vec3(&normalsBuffer[v * 3]) : glm::vec3(0.0f))));
				vertexAttributeBuffers.tangent.push_back(tangentsBuffer ? glm::make_vec4(&tangentsBuffer[v * 4]) : glm::vec4(0.0f));
				vertexAttributeBuffers.uv.push_back(texCoordsBuffer ? glm::make_vec2(&texCoordsBuffer[v * 2]) : glm::vec3(0.0f));
			}

			// Indices
			const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.indices];
			const tinygltf::BufferView& bufferView = input.bufferViews[accessor.bufferView];
			const tinygltf::Buffer& buffer = input.buffers[bufferView.buffer];

			indexCount += static_cast<uint32_t>(accessor.count);

			// glTF supports different component types of indices
			switch (accessor.componentType) {
			case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
				const uint32_t* buf = reinterpret_cast<const uint32_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
				for (size_t index = 0; index < accessor.count; index++) {
					indexBuffer.push_back(buf[index] + vertexStart);
				}
				break;
			}
			case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
				const uint16_t* buf = reinterpret_cast<const uint16_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
				for (size_t index = 0; index < accessor.count; index++) {
					indexBuffer.push_back(buf[index] + vertexStart);
				}
				break;
			}
			case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
				const uint8_t* buf = reinterpret_cast<const uint8_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
				for (size_t index = 0; index < accessor.count; index++) {
					indexBuffer.push_back(buf[index] + vertexStart);
				}
				break;
			}
			default:
				std::cerr << "Index component type " << accessor.componentType << " not supported!" << std::endl;
				return;
			}

			Primitive primitive{};
			primitive.firstIndex = firstIndex;
			primitive.indexCount = indexCount;
			primitive.materialIndex = glTFPrimitive.material;
			node.mesh.primitives.push_back(primitive);
		}
	}

	if (parent) {
		parent->children.push_back(node);
	}
	else {
		nodes.push_back(node);
	}
}

VulkanExample::VulkanExample() : VulkanExampleBase(ENABLE_VALIDATION)
{
	title = "Separate/interleaved vertex attribute buffers";
	camera.type = Camera::CameraType::firstperson;
	camera.flipY = true;
	camera.setPosition(glm::vec3(0.0f, 1.0f, 0.0f));
	camera.setRotation(glm::vec3(0.0f, -90.0f, 0.0f));
	camera.setPerspective(60.0f, (float)width / (float)height, 0.1f, 256.0f);
}

VulkanExample::~VulkanExample()
{
	vkDestroyPipeline(device, pipelines.vertexAttributesInterleaved, nullptr);
	vkDestroyPipeline(device, pipelines.vertexAttributesSeparate, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	vkDestroyDescriptorSetLayout(device, descriptorSetLayouts.matrices, nullptr);
	vkDestroyDescriptorSetLayout(device, descriptorSetLayouts.textures, nullptr);
	indices.destroy();
	shaderData.buffer.destroy();
	separateVertexBuffers.normal.destroy();
	separateVertexBuffers.pos.destroy();
	separateVertexBuffers.tangent.destroy();
	separateVertexBuffers.uv.destroy();
	interleavedVertexBuffer.destroy();
	for (Image image : scene.images) {
		vkDestroyImageView(vulkanDevice->logicalDevice, image.texture.view, nullptr);
		vkDestroyImage(vulkanDevice->logicalDevice, image.texture.image, nullptr);
		vkDestroySampler(vulkanDevice->logicalDevice, image.texture.sampler, nullptr);
		vkFreeMemory(vulkanDevice->logicalDevice, image.texture.deviceMemory, nullptr);
	}
}

void VulkanExample::getEnabledFeatures()
{
	enabledFeatures.samplerAnisotropy = deviceFeatures.samplerAnisotropy;
}

void VulkanExample::buildCommandBuffers()
{
	VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

	VkClearValue clearValues[2];
	clearValues[0].color = defaultClearColor;
	clearValues[0].color = { { 0.25f, 0.25f, 0.25f, 1.0f } };;
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
	renderPassBeginInfo.renderPass = renderPass;
	renderPassBeginInfo.renderArea.offset.x = 0;
	renderPassBeginInfo.renderArea.offset.y = 0;
	renderPassBeginInfo.renderArea.extent.width = width;
	renderPassBeginInfo.renderArea.extent.height = height;
	renderPassBeginInfo.clearValueCount = 2;
	renderPassBeginInfo.pClearValues = clearValues;

	const VkViewport viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
	const VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);

	for (int32_t i = 0; i < drawCmdBuffers.size(); ++i)
	{
		renderPassBeginInfo.framebuffer = frameBuffers[i];
		VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo));
		vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);
		vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

		// Select the separate or interleaved vertex binding pipeline
		vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vertexAttributeSettings == VertexAttributeSettings::separate ? pipelines.vertexAttributesSeparate : pipelines.vertexAttributesInterleaved);

		// Bind scene matrices descriptor to set 0
		vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

		// Use the same index buffer, no matter how vertex attributes are passed
		vkCmdBindIndexBuffer(drawCmdBuffers[i], indices.buffer, 0, VK_INDEX_TYPE_UINT32);

		if (vertexAttributeSettings == VertexAttributeSettings::separate) {
			// Using separate vertex attribute bindings requires binding multiple attribute buffers
			VkDeviceSize offsets[4] = { 0, 0, 0, 0 };
			std::array<VkBuffer, 4> buffers = { separateVertexBuffers.pos.buffer, separateVertexBuffers.normal.buffer, separateVertexBuffers.uv.buffer, separateVertexBuffers.tangent.buffer };
			vkCmdBindVertexBuffers(drawCmdBuffers[i], 0, static_cast<uint32_t>(buffers.size()), buffers.data(), offsets);
		}
		else {
			// Using interleaved attribute bindings only requires one buffer to be bound
			VkDeviceSize offsets[1] = { 0 };
			vkCmdBindVertexBuffers(drawCmdBuffers[i], 0, 1, &interleavedVertexBuffer.buffer, offsets);
		}
		// Render all nodes starting at top-level
		for (auto& node : nodes) {
			drawSceneNode(drawCmdBuffers[i], node);
		}

		drawUI(drawCmdBuffers[i]);
		vkCmdEndRenderPass(drawCmdBuffers[i]);
		VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
	}
}

void VulkanExample::loadglTFFile(std::string filename)
{
	tinygltf::Model glTFInput;
	tinygltf::TinyGLTF gltfContext;
	std::string error, warning;

	this->device = device;

#if defined(__ANDROID__)
	// On Android all assets are packed with the apk in a compressed form, so we need to open them using the asset manager
	// We let tinygltf handle this, by passing the asset manager of our app
	tinygltf::asset_manager = androidApp->activity->assetManager;
#endif
	bool fileLoaded = gltfContext.LoadASCIIFromFile(&glTFInput, &error, &warning, filename);

	size_t pos = filename.find_last_of('/');
	std::string path = filename.substr(0, pos);

	if (!fileLoaded) {
		vks::tools::exitFatal("Could not open the glTF file.\n\nThe file is part of the additional asset pack.\n\nRun \"download_assets.py\" in the repository root to download the latest version.", -1);
		return;
	}

	// Load images
	scene.images.resize(glTFInput.images.size());
	for (size_t i = 0; i < glTFInput.images.size(); i++) {
		tinygltf::Image& glTFImage = glTFInput.images[i];
		scene.images[i].texture.loadFromFile(path + "/" + glTFImage.uri, VK_FORMAT_R8G8B8A8_UNORM, vulkanDevice, queue);
	}
	// Load textures
	scene.textures.resize(glTFInput.textures.size());
	for (size_t i = 0; i < glTFInput.textures.size(); i++) {
		scene.textures[i].imageIndex = glTFInput.textures[i].source;
	}
	// Load materials
	scene.materials.resize(glTFInput.materials.size());
	for (size_t i = 0; i < glTFInput.materials.size(); i++) {
		// We only read the most basic properties required for our sample
		tinygltf::Material glTFMaterial = glTFInput.materials[i];
		// Get the base color factor
		if (glTFMaterial.values.find("baseColorFactor") != glTFMaterial.values.end()) {
			scene.materials[i].baseColorFactor = glm::make_vec4(glTFMaterial.values["baseColorFactor"].ColorFactor().data());
		}
		// Get base color texture index
		if (glTFMaterial.values.find("baseColorTexture") != glTFMaterial.values.end()) {
			scene.materials[i].baseColorTextureIndex = glTFMaterial.values["baseColorTexture"].TextureIndex();
		}
		// Get the normal map texture index
		if (glTFMaterial.additionalValues.find("normalTexture") != glTFMaterial.additionalValues.end()) {
			scene.materials[i].normalTextureIndex = glTFMaterial.additionalValues["normalTexture"].TextureIndex();
		}
		// Get some additional material parameters that are used in this sample
		scene.materials[i].alphaMode = glTFMaterial.alphaMode;
		scene.materials[i].alphaCutOff = (float)glTFMaterial.alphaCutoff;
	}
	// Load nodes
	const tinygltf::Scene& scene = glTFInput.scenes[0];
	for (size_t i = 0; i < scene.nodes.size(); i++) {
		const tinygltf::Node node = glTFInput.nodes[scene.nodes[i]];
		loadSceneNode(node, glTFInput, nullptr);
	}

	uploadVertexData();
}

void VulkanExample::uploadVertexData()
{
	// Upload vertex and index buffers

	// Anonymous functions to simplify buffer creation
	// Create a staging buffer used as a source for copies
	auto createStagingBuffer = [this](vks::Buffer& buffer, void* data, VkDeviceSize size) {
		VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &buffer, size, data));
	};
	// Create a device local buffer used as a target for copies
	auto createDeviceBuffer = [this](vks::Buffer& buffer, VkDeviceSize size, VkBufferUsageFlags usageFlags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT) {
		VK_CHECK_RESULT(vulkanDevice->createBuffer(usageFlags | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &buffer, size));
	};

	VkCommandBuffer copyCmd;
	VkBufferCopy copyRegion{};

	/*
		Interleaved vertex attributes
		We create one single buffer containing the interleaved vertex attributes
	*/
	size_t vertexBufferSize = vertexBuffer.size() * sizeof(Vertex);
	vks::Buffer vertexStaging;
	createStagingBuffer(vertexStaging, vertexBuffer.data(), vertexBufferSize);
	createDeviceBuffer(interleavedVertexBuffer, vertexStaging.size);

	// Copy data from staging buffer (host) do device local buffer (gpu)
	copyCmd = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
	copyRegion.size = vertexBufferSize;
	vkCmdCopyBuffer(copyCmd, vertexStaging.buffer, interleavedVertexBuffer.buffer, 1, &copyRegion);
	vulkanDevice->flushCommandBuffer(copyCmd, queue, true);
	vertexStaging.destroy();

	/*
		Separate vertex attributes
		We create multiple separate buffers for each of the vertex attributes (position, normals, etc.)
	*/
	std::array<vks::Buffer, 4> stagingBuffers;
	createStagingBuffer(stagingBuffers[0], vertexAttributeBuffers.pos.data(), vertexAttributeBuffers.pos.size() * sizeof(vertexAttributeBuffers.pos[0]));
	createStagingBuffer(stagingBuffers[1], vertexAttributeBuffers.normal.data(), vertexAttributeBuffers.normal.size() * sizeof(vertexAttributeBuffers.normal[0]));
	createStagingBuffer(stagingBuffers[2], vertexAttributeBuffers.uv.data(), vertexAttributeBuffers.uv.size() * sizeof(vertexAttributeBuffers.uv[0]));
	createStagingBuffer(stagingBuffers[3], vertexAttributeBuffers.tangent.data(), vertexAttributeBuffers.tangent.size() * sizeof(vertexAttributeBuffers.tangent[0]));

	createDeviceBuffer(separateVertexBuffers.pos, stagingBuffers[0].size);
	createDeviceBuffer(separateVertexBuffers.normal, stagingBuffers[1].size);
	createDeviceBuffer(separateVertexBuffers.uv, stagingBuffers[2].size);
	createDeviceBuffer(separateVertexBuffers.tangent, stagingBuffers[3].size);

	// Stage
	std::vector<vks::Buffer> attributeBuffers = {
		separateVertexBuffers.pos,
		separateVertexBuffers.normal,
		separateVertexBuffers.uv,
		separateVertexBuffers.tangent,
	};

	// Copy data from staging buffer (host) do device local buffer (gpu)
	copyCmd = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
	for (size_t i = 0; i < attributeBuffers.size(); i++) {
		copyRegion.size = attributeBuffers[i].size;
		vkCmdCopyBuffer(copyCmd, stagingBuffers[i].buffer, attributeBuffers[i].buffer, 1, &copyRegion);
	}
	vulkanDevice->flushCommandBuffer(copyCmd, queue, true);

	for (size_t i = 0; i < 4; i++) {
		stagingBuffers[i].destroy();
	}

	/*
		Index buffer
		The index buffer is always the same, no matter how we pass the vertex attributes
	*/
	size_t indexBufferSize = indexBuffer.size() * sizeof(uint32_t);
	vks::Buffer indexStaging;
	createStagingBuffer(indexStaging, indexBuffer.data(), indexBufferSize);
	createDeviceBuffer(indices, indexStaging.size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
	// Copy data from staging buffer (host) do device local buffer (gpu)
	copyCmd = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
	copyRegion.size = indexBufferSize;
	vkCmdCopyBuffer(copyCmd, indexStaging.buffer, indices.buffer, 1, &copyRegion);
	vulkanDevice->flushCommandBuffer(copyCmd, queue, true);
	// Free staging resources
	indexStaging.destroy();
}

void VulkanExample::loadAssets()
{
	loadglTFFile(getAssetPath() + "models/sponza/sponza.gltf");
}

void VulkanExample::setupDescriptors()
{
	// One ubo to pass dynamic data to the shader
	// Two combined image samplers per material as each material uses color and normal maps
	std::vector<VkDescriptorPoolSize> poolSizes = {
		vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1),
		vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(scene.materials.size()) * 2),
	};
	// One set for matrices and one per model image/texture
	const uint32_t maxSetCount = static_cast<uint32_t>(scene.images.size()) + 1;
	VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, maxSetCount);
	VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));
	// Descriptor set layout for passing matrices
	std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
		vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0)
	};
	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings.data(), static_cast<uint32_t>(setLayoutBindings.size()));
	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCI, nullptr, &descriptorSetLayouts.matrices));
	// Descriptor set layout for passing material textures
	setLayoutBindings = {
		// Color map
		vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0),
		// Normal map
		vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
	};
	descriptorSetLayoutCI.pBindings = setLayoutBindings.data();
	descriptorSetLayoutCI.bindingCount = 2;
	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCI, nullptr, &descriptorSetLayouts.textures));

	// Pipeline layout using both descriptor sets (set 0 = matrices, set 1 = material)
	std::array<VkDescriptorSetLayout, 2> setLayouts = { descriptorSetLayouts.matrices, descriptorSetLayouts.textures };
	VkPipelineLayoutCreateInfo pipelineLayoutCI = vks::initializers::pipelineLayoutCreateInfo(setLayouts.data(), static_cast<uint32_t>(setLayouts.size()));
	// We will use push constants to push the local matrices of a primitive to the vertex shader
	VkPushConstantRange pushConstantRange = vks::initializers::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(PushConstBlock), 0);
	// Push constant ranges are part of the pipeline layout
	pipelineLayoutCI.pushConstantRangeCount = 1;
	pipelineLayoutCI.pPushConstantRanges = &pushConstantRange;
	VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &pipelineLayout));

	// Descriptor set for scene matrices
	VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayouts.matrices, 1);
	VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));
	VkWriteDescriptorSet writeDescriptorSet = vks::initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &shaderData.buffer.descriptor);
	vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);

	// Descriptor sets for the materials
	for (auto& material : scene.materials) {
		const VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayouts.textures, 1);
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &material.descriptorSet));
		VkDescriptorImageInfo colorMap = scene.images[material.baseColorTextureIndex].texture.descriptor;
		VkDescriptorImageInfo normalMap = scene.images[material.normalTextureIndex].texture.descriptor;
		std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
			vks::initializers::writeDescriptorSet(material.descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &colorMap),
			vks::initializers::writeDescriptorSet(material.descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &normalMap),
		};
		vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
	}
}

void VulkanExample::preparePipelines()
{
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
	VkPipelineRasterizationStateCreateInfo rasterizationStateCI = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
	VkPipelineColorBlendAttachmentState blendAttachmentStateCI = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
	VkPipelineColorBlendStateCreateInfo colorBlendStateCI = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentStateCI);
	VkPipelineDepthStencilStateCreateInfo depthStencilStateCI = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
	VkPipelineViewportStateCreateInfo viewportStateCI = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
	VkPipelineMultisampleStateCreateInfo multisampleStateCI = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
	const std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicStateCI = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables.data(), static_cast<uint32_t>(dynamicStateEnables.size()), 0);
	VkPipelineVertexInputStateCreateInfo vertexInputStateCI = vks::initializers::pipelineVertexInputStateCreateInfo();
	std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

	VkGraphicsPipelineCreateInfo pipelineCI = vks::initializers::pipelineCreateInfo(pipelineLayout, renderPass, 0);
	pipelineCI.pVertexInputState = &vertexInputStateCI;
	pipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
	pipelineCI.pRasterizationState = &rasterizationStateCI;
	pipelineCI.pColorBlendState = &colorBlendStateCI;
	pipelineCI.pMultisampleState = &multisampleStateCI;
	pipelineCI.pViewportState = &viewportStateCI;
	pipelineCI.pDepthStencilState = &depthStencilStateCI;
	pipelineCI.pDynamicState = &dynamicStateCI;
	pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
	pipelineCI.pStages = shaderStages.data();

	shaderStages[0] = loadShader(getShadersPath() + "vertexattributes/scene.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = loadShader(getShadersPath() + "vertexattributes/scene.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

	// Interleaved vertex attributes 
	// One Binding (one buffer) and multiple attributes
	const std::vector<VkVertexInputBindingDescription> vertexInputBindingsInterleaved = {
		{ 0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX },
	};
	const std::vector<VkVertexInputAttributeDescription> vertexInputAttributesInterleaved = {
		{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos) },
		{ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal) },
		{ 2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv) },
		{ 3, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, tangent) },
	};

	vertexInputStateCI = vks::initializers::pipelineVertexInputStateCreateInfo(vertexInputBindingsInterleaved, vertexInputAttributesInterleaved);
	VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.vertexAttributesInterleaved));

	// Separate vertex attribute
	// Multiple bindings (for each attribute buffer) and multiple attribues
	const std::vector<VkVertexInputBindingDescription> vertexInputBindingsSeparate = {
		{ 0, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX },
		{ 1, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX },
		{ 2, sizeof(glm::vec2), VK_VERTEX_INPUT_RATE_VERTEX },
		{ 3, sizeof(glm::vec4), VK_VERTEX_INPUT_RATE_VERTEX },
	};
	const std::vector<VkVertexInputAttributeDescription> vertexInputAttributesSeparate = {
		{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },
		{ 1, 1, VK_FORMAT_R32G32B32_SFLOAT, 0 },
		{ 2, 2, VK_FORMAT_R32G32_SFLOAT, 0 },
		{ 3, 3, VK_FORMAT_R32G32B32A32_SFLOAT, 0 },
	};

	vertexInputStateCI = vks::initializers::pipelineVertexInputStateCreateInfo(vertexInputBindingsSeparate, vertexInputAttributesSeparate);
	VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.vertexAttributesSeparate));
}

void VulkanExample::prepareUniformBuffers()
{
	VK_CHECK_RESULT(vulkanDevice->createBuffer(
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&shaderData.buffer,
		sizeof(shaderData.values)));
	VK_CHECK_RESULT(shaderData.buffer.map());
	updateUniformBuffers();
}

void VulkanExample::updateUniformBuffers()
{
	shaderData.values.projection = camera.matrices.perspective;
	shaderData.values.view = camera.matrices.view;
	shaderData.values.viewPos = camera.viewPos;
	memcpy(shaderData.buffer.mapped, &shaderData.values, sizeof(shaderData.values));
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

void VulkanExample::drawSceneNode(VkCommandBuffer commandBuffer, Node node)
{
	if (node.mesh.primitives.size() > 0) {
		PushConstBlock pushConstBlock;
		glm::mat4 nodeMatrix = node.matrix;
		Node* currentParent = node.parent;
		while (currentParent) {
			nodeMatrix = currentParent->matrix * nodeMatrix;
			currentParent = currentParent->parent;
		}
		for (Primitive& primitive : node.mesh.primitives) {
			if (primitive.indexCount > 0) {
				Material& material = scene.materials[primitive.materialIndex];
				pushConstBlock.nodeMatrix = nodeMatrix;
				pushConstBlock.alphaMask = (material.alphaMode == "MASK");
				pushConstBlock.alphaMaskCutoff = material.alphaCutOff;
				vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstBlock), &pushConstBlock);
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &material.descriptorSet, 0, nullptr);
				vkCmdDrawIndexed(commandBuffer, primitive.indexCount, 1, primitive.firstIndex, 0, 0);
			}
		}
	}
	for (auto& child : node.children) {
		drawSceneNode(commandBuffer, child);
	}
}

void VulkanExample::render()
{
	renderFrame();
	if (camera.updated) {
		updateUniformBuffers();
	}
}

void VulkanExample::OnUpdateUIOverlay(vks::UIOverlay* overlay)
{
	if (overlay->header("Vertex buffer attributes")) {
		bool interleaved = (vertexAttributeSettings == VertexAttributeSettings::interleaved);
		bool separate = (vertexAttributeSettings == VertexAttributeSettings::separate);
		if (overlay->radioButton("Interleaved", interleaved)) {
			vertexAttributeSettings = VertexAttributeSettings::interleaved;
			buildCommandBuffers();
		}
		if (overlay->radioButton("Separate", separate)) {
			vertexAttributeSettings = VertexAttributeSettings::separate;
			buildCommandBuffers();
		}
	}
}

VULKAN_EXAMPLE_MAIN()