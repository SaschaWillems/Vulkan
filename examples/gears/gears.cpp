/*
* Vulkan Example - Drawing multiple animated gears (emulating the look of glxgears)
* 
* All gears are using single index, vertex and uniform buffers to show the Vulkan best practices of keeping the no. of buffer/memory allocations to a mimimum
* We use index offsets and instance indices to offset into the buffers at draw time for each gear
* 
* Copyright (C) 2016-2023 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "vulkanexamplebase.h"

const uint32_t numGears = 3;

// Used for passing the definition of a gear during construction
struct GearDefinition {
	float innerRadius;
	float outerRadius;
	float width;
	int numTeeth;
	float toothDepth;
	glm::vec3 color;
	glm::vec3 pos;
	float rotSpeed;
	float rotOffset;
};

/*
 * Gear
 * This class contains the properties of a single gear and a function to generate vertices and indices
 */
class Gear
{
public:
	// Definition for the vertex data used to render the gears
	struct Vertex {
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec3 color;
	};

	glm::vec3 color;
	glm::vec3 pos;
	float rotSpeed{ 0.0f };
	float rotOffset{ 0.0f };
	// These are used at draw time to offset into the single buffers
	uint32_t indexCount{ 0 };
	uint32_t indexStart{ 0 };

	// Generates the indices and vertices for this gear
	// They are added to the vertex and index buffers passed into the function
	// This way we can put all gears into single vertex and index buffers instead of having to allocate single buffers for each gear (which would be bad practice)
	void generate(GearDefinition& gearDefinition, std::vector<Vertex>& vertexBuffer, std::vector<uint32_t>& indexBuffer) {
		this->color = gearDefinition.color;
		this->pos = gearDefinition.pos;
		this->rotOffset = gearDefinition.rotOffset;
		this->rotSpeed = gearDefinition.rotSpeed;

		int i;
		float r0, r1, r2;
		float ta, da;
		float u1, v1, u2, v2, len;
		float cos_ta, cos_ta_1da, cos_ta_2da, cos_ta_3da, cos_ta_4da;
		float sin_ta, sin_ta_1da, sin_ta_2da, sin_ta_3da, sin_ta_4da;
		int32_t ix0, ix1, ix2, ix3, ix4, ix5;

		// We need to know where this triangle's indices start within the single index buffer
		indexStart = static_cast<uint32_t>(indexBuffer.size());

		r0 = gearDefinition.innerRadius;
		r1 = gearDefinition.outerRadius - gearDefinition.toothDepth / 2.0f;
		r2 = gearDefinition.outerRadius + gearDefinition.toothDepth / 2.0f;
		da = static_cast <float>(2.0 * M_PI / gearDefinition.numTeeth / 4.0);

		glm::vec3 normal;

		// Use lambda functions to simplify vertex and face creation
		auto addFace = [&indexBuffer](int a, int b, int c) {
			indexBuffer.push_back(a);
			indexBuffer.push_back(b);
			indexBuffer.push_back(c);
			};

		auto addVertex = [this, &vertexBuffer](float x, float y, float z, glm::vec3 normal) {
			Vertex v{};
			v.position = { x, y, z };
			v.normal = normal;
			v.color = this->color;
			vertexBuffer.push_back(v);
			return static_cast<int32_t>(vertexBuffer.size()) - 1;
			};

		for (i = 0; i < gearDefinition.numTeeth; i++) {
			ta = i * static_cast <float>(2.0 * M_PI / gearDefinition.numTeeth);

			cos_ta = cos(ta);
			cos_ta_1da = cos(ta + da);
			cos_ta_2da = cos(ta + 2.0f * da);
			cos_ta_3da = cos(ta + 3.0f * da);
			cos_ta_4da = cos(ta + 4.0f * da);
			sin_ta = sin(ta);
			sin_ta_1da = sin(ta + da);
			sin_ta_2da = sin(ta + 2.0f * da);
			sin_ta_3da = sin(ta + 3.0f * da);
			sin_ta_4da = sin(ta + 4.0f * da);

			u1 = r2 * cos_ta_1da - r1 * cos_ta;
			v1 = r2 * sin_ta_1da - r1 * sin_ta;
			len = sqrt(u1 * u1 + v1 * v1);
			u1 /= len;
			v1 /= len;
			u2 = r1 * cos_ta_3da - r2 * cos_ta_2da;
			v2 = r1 * sin_ta_3da - r2 * sin_ta_2da;

			// Front face
			normal = glm::vec3(0.0f, 0.0f, 1.0f);
			ix0 = addVertex(r0 * cos_ta, r0 * sin_ta, gearDefinition.width * 0.5f, normal);
			ix1 = addVertex(r1 * cos_ta, r1 * sin_ta, gearDefinition.width * 0.5f, normal);
			ix2 = addVertex(r0 * cos_ta, r0 * sin_ta, gearDefinition.width * 0.5f, normal);
			ix3 = addVertex(r1 * cos_ta_3da, r1 * sin_ta_3da, gearDefinition.width * 0.5f, normal);
			ix4 = addVertex(r0 * cos_ta_4da, r0 * sin_ta_4da, gearDefinition.width * 0.5f, normal);
			ix5 = addVertex(r1 * cos_ta_4da, r1 * sin_ta_4da, gearDefinition.width * 0.5f, normal);
			addFace(ix0, ix1, ix2);
			addFace(ix1, ix3, ix2);
			addFace(ix2, ix3, ix4);
			addFace(ix3, ix5, ix4);

			// Teeth front face
			normal = glm::vec3(0.0f, 0.0f, 1.0f);
			ix0 = addVertex(r1 * cos_ta, r1 * sin_ta, gearDefinition.width * 0.5f, normal);
			ix1 = addVertex(r2 * cos_ta_1da, r2 * sin_ta_1da, gearDefinition.width * 0.5f, normal);
			ix2 = addVertex(r1 * cos_ta_3da, r1 * sin_ta_3da, gearDefinition.width * 0.5f, normal);
			ix3 = addVertex(r2 * cos_ta_2da, r2 * sin_ta_2da, gearDefinition.width * 0.5f, normal);
			addFace(ix0, ix1, ix2);
			addFace(ix1, ix3, ix2);

			// Back face 
			normal = glm::vec3(0.0f, 0.0f, -1.0f);
			ix0 = addVertex(r1 * cos_ta, r1 * sin_ta, -gearDefinition.width * 0.5f, normal);
			ix1 = addVertex(r0 * cos_ta, r0 * sin_ta, -gearDefinition.width * 0.5f, normal);
			ix2 = addVertex(r1 * cos_ta_3da, r1 * sin_ta_3da, -gearDefinition.width * 0.5f, normal);
			ix3 = addVertex(r0 * cos_ta, r0 * sin_ta, -gearDefinition.width * 0.5f, normal);
			ix4 = addVertex(r1 * cos_ta_4da, r1 * sin_ta_4da, -gearDefinition.width * 0.5f, normal);
			ix5 = addVertex(r0 * cos_ta_4da, r0 * sin_ta_4da, -gearDefinition.width * 0.5f, normal);
			addFace(ix0, ix1, ix2);
			addFace(ix1, ix3, ix2);
			addFace(ix2, ix3, ix4);
			addFace(ix3, ix5, ix4);

			// Teeth back face
			normal = glm::vec3(0.0f, 0.0f, -1.0f);
			ix0 = addVertex(r1 * cos_ta_3da, r1 * sin_ta_3da, -gearDefinition.width * 0.5f, normal);
			ix1 = addVertex(r2 * cos_ta_2da, r2 * sin_ta_2da, -gearDefinition.width * 0.5f, normal);
			ix2 = addVertex(r1 * cos_ta, r1 * sin_ta, -gearDefinition.width * 0.5f, normal);
			ix3 = addVertex(r2 * cos_ta_1da, r2 * sin_ta_1da, -gearDefinition.width * 0.5f, normal);
			addFace(ix0, ix1, ix2);
			addFace(ix1, ix3, ix2);

			// Outard teeth faces
			normal = glm::vec3(v1, -u1, 0.0f);
			ix0 = addVertex(r1 * cos_ta, r1 * sin_ta, gearDefinition.width * 0.5f, normal);
			ix1 = addVertex(r1 * cos_ta, r1 * sin_ta, -gearDefinition.width * 0.5f, normal);
			ix2 = addVertex(r2 * cos_ta_1da, r2 * sin_ta_1da, gearDefinition.width * 0.5f, normal);
			ix3 = addVertex(r2 * cos_ta_1da, r2 * sin_ta_1da, -gearDefinition.width * 0.5f, normal);
			addFace(ix0, ix1, ix2);
			addFace(ix1, ix3, ix2);

			normal = glm::vec3(cos_ta, sin_ta, 0.0f);
			ix0 = addVertex(r2 * cos_ta_1da, r2 * sin_ta_1da, gearDefinition.width * 0.5f, normal);
			ix1 = addVertex(r2 * cos_ta_1da, r2 * sin_ta_1da, -gearDefinition.width * 0.5f, normal);
			ix2 = addVertex(r2 * cos_ta_2da, r2 * sin_ta_2da, gearDefinition.width * 0.5f, normal);
			ix3 = addVertex(r2 * cos_ta_2da, r2 * sin_ta_2da, -gearDefinition.width * 0.5f, normal);
			addFace(ix0, ix1, ix2);
			addFace(ix1, ix3, ix2);

			normal = glm::vec3(v2, -u2, 0.0f);
			ix0 = addVertex(r2 * cos_ta_2da, r2 * sin_ta_2da, gearDefinition.width * 0.5f, normal);
			ix1 = addVertex(r2 * cos_ta_2da, r2 * sin_ta_2da, -gearDefinition.width * 0.5f, normal);
			ix2 = addVertex(r1 * cos_ta_3da, r1 * sin_ta_3da, gearDefinition.width * 0.5f, normal);
			ix3 = addVertex(r1 * cos_ta_3da, r1 * sin_ta_3da, -gearDefinition.width * 0.5f, normal);
			addFace(ix0, ix1, ix2);
			addFace(ix1, ix3, ix2);

			normal = glm::vec3(cos_ta, sin_ta, 0.0f);
			ix0 = addVertex(r1 * cos_ta_3da, r1 * sin_ta_3da, gearDefinition.width * 0.5f, normal);
			ix1 = addVertex(r1 * cos_ta_3da, r1 * sin_ta_3da, -gearDefinition.width * 0.5f, normal);
			ix2 = addVertex(r1 * cos_ta_4da, r1 * sin_ta_4da, gearDefinition.width * 0.5f, normal);
			ix3 = addVertex(r1 * cos_ta_4da, r1 * sin_ta_4da, -gearDefinition.width * 0.5f, normal);
			addFace(ix0, ix1, ix2);
			addFace(ix1, ix3, ix2);

			// Inside cylinder faces
			ix0 = addVertex(r0 * cos_ta, r0 * sin_ta, -gearDefinition.width * 0.5f, glm::vec3(-cos_ta, -sin_ta, 0.0f));
			ix1 = addVertex(r0 * cos_ta, r0 * sin_ta, gearDefinition.width * 0.5f, glm::vec3(-cos_ta, -sin_ta, 0.0f));
			ix2 = addVertex(r0 * cos_ta_4da, r0 * sin_ta_4da, -gearDefinition.width * 0.5f, glm::vec3(-cos_ta_4da, -sin_ta_4da, 0.0f));
			ix3 = addVertex(r0 * cos_ta_4da, r0 * sin_ta_4da, gearDefinition.width * 0.5f, glm::vec3(-cos_ta_4da, -sin_ta_4da, 0.0f));
			addFace(ix0, ix1, ix2);
			addFace(ix1, ix3, ix2);
		}

		// We need to know how many indices this triangle has at draw time
		indexCount = static_cast<uint32_t>(indexBuffer.size()) - indexStart;
	}
};

/*
 * VulkanExample
 */
class VulkanExample : public VulkanExampleBase
{
public:
	std::vector<Gear> gears{};

	VkPipeline pipeline{ VK_NULL_HANDLE };
	VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };
	VkDescriptorSet descriptorSet{ VK_NULL_HANDLE };
	VkDescriptorSetLayout descriptorSetLayout{ VK_NULL_HANDLE };

	// Even though this sample renders multiple objects (gears), we only use single buffers
	// This is a best practices and Vulkan applications should keep the number of memory allocations as small as possible
	// Having as little buffers as possible also reduces the number of buffer binds
	vks::Buffer vertexBuffer;
	vks::Buffer indexBuffer;
	struct UniformData
	{
		glm::mat4 projection;
		glm::mat4 view;
		glm::vec4 lightPos;
		// The model matrix is used to rotate a given gear, so we have one mat4 per gear
		glm::mat4 model[numGears];
	} uniformData;
	vks::Buffer uniformBuffer;

	VulkanExample() : VulkanExampleBase()
	{
		title = "Vulkan gears";
		camera.type = Camera::CameraType::lookat;
		camera.setPosition(glm::vec3(0.0f, 2.5f, -16.0f));
		camera.setRotation(glm::vec3(0.0f, 0.0f, 0.0f));
		camera.setPerspective(60.0f, (float)width / (float)height, 0.001f, 256.0f);
		timerSpeed *= 0.25f;
	}

	~VulkanExample()
	{
		if (device) {
			vkDestroyPipeline(device, pipeline, nullptr);
			vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
			vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
			indexBuffer.destroy();
			vertexBuffer.destroy();
			uniformBuffer.destroy();
		}
	}

	void prepareGears()
	{
		// Set up three differntly shaped and colored gears
		std::vector<GearDefinition> gearDefinitions(3);

		// Large red gear
		gearDefinitions[0].innerRadius = 1.0f;
		gearDefinitions[0].outerRadius = 4.0f;
		gearDefinitions[0].width = 1.0f;
		gearDefinitions[0].numTeeth = 20;
		gearDefinitions[0].toothDepth = 0.7f;
		gearDefinitions[0].color = { 1.0f, 0.0f, 0.0f };
		gearDefinitions[0].pos = { -3.0f, 0.0f, 0.0f };
		gearDefinitions[0].rotSpeed = 1.0f;
		gearDefinitions[0].rotOffset = 0.0f;

		// Medium sized green gear
		gearDefinitions[1].innerRadius = 0.5f;
		gearDefinitions[1].outerRadius = 2.0f;
		gearDefinitions[1].width = 2.0f;
		gearDefinitions[1].numTeeth = 10;
		gearDefinitions[1].toothDepth = 0.7f;
		gearDefinitions[1].color = { 0.0f, 1.0f, 0.2f };
		gearDefinitions[1].pos = { 3.1f, 0.0f, 0.0f };
		gearDefinitions[1].rotSpeed = -2.0f;
		gearDefinitions[1].rotOffset = -9.0f;

		// Small blue gear
		gearDefinitions[2].innerRadius = 1.3f;
		gearDefinitions[2].outerRadius = 2.0f;
		gearDefinitions[2].width = 0.5f;
		gearDefinitions[2].numTeeth = 10;
		gearDefinitions[2].toothDepth = 0.7f;
		gearDefinitions[2].color = { 0.0f, 0.0f, 1.0f };
		gearDefinitions[2].pos = { -3.1f, -6.2f, 0.0f };
		gearDefinitions[2].rotSpeed = -2.0f;
		gearDefinitions[2].rotOffset = -30.0f;

		// We'll be using a single vertex and a single index buffer for all the gears, no matter their number
		// This is a Vulkan best practice as it keeps the no. of memory/buffer allocations low
		// Vulkan offers all the tools to easily index into those buffers at a later point (see the buildCommandBuffers function)
		std::vector<Gear::Vertex> vertices{};
		std::vector<uint32_t> indices{};

		// Fills the vertex and index buffers for each of the gear
		gears.resize(gearDefinitions.size());
		for (int32_t i = 0; i < gears.size(); i++) {
			gears[i].generate(gearDefinitions[i], vertices, indices);
		}

		// Create buffers and stage to device for performances
		size_t vertexBufferSize = vertices.size() * sizeof(Gear::Vertex);
		size_t indexBufferSize = indices.size() * sizeof(uint32_t);

		vks::Buffer vertexStaging, indexStaging;

		// Temorary Staging buffers from vertex and index data
		vulkanDevice->createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &vertexStaging, vertexBufferSize, vertices.data());
		vulkanDevice->createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &indexStaging, indexBufferSize, indices.data());
		// Device local buffers to where our staging buffers will be copied to
		vulkanDevice->createBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &vertexBuffer, vertexBufferSize);
		vulkanDevice->createBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &indexBuffer, indexBufferSize);

		// Copy host (staging) to device
		VkCommandBuffer copyCmd = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
		VkBufferCopy copyRegion = {};
		copyRegion.size = vertexBufferSize;
		vkCmdCopyBuffer(copyCmd, vertexStaging.buffer, vertexBuffer.buffer, 1, &copyRegion);
		copyRegion.size = indexBufferSize;
		vkCmdCopyBuffer(copyCmd, indexStaging.buffer, indexBuffer.buffer, 1, &copyRegion);
		vulkanDevice->flushCommandBuffer(copyCmd, queue, true);

		vertexStaging.destroy();
		indexStaging.destroy();
	}

	void setupDescriptors()
	{
		// We use a single descriptor set for the uniform data that contains both global matrices as well as per-gear model matrices

		// Pool
		std::vector<VkDescriptorPoolSize> poolSizes = {
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1),
		};
		VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, static_cast<uint32_t>(gears.size()));
		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));

		// Layout
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
			// Binding 0 : Vertex shader uniform buffer
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0)
		};
		VkDescriptorSetLayoutCreateInfo descriptorLayout = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayout));

		// Set
		VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));

		VkWriteDescriptorSet writeDescriptorSet = vks::initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffer.descriptor);
		vkUpdateDescriptorSets(vulkanDevice->logicalDevice, 1, &writeDescriptorSet, 0, nullptr);
	}

	void preparePipelines()
	{
		// Layout
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(&descriptorSetLayout, 1);
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

		//  Pipelines
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
		VkPipelineRasterizationStateCreateInfo rasterizationState = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE, 0);
		VkPipelineColorBlendAttachmentState blendAttachmentState = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
		VkPipelineColorBlendStateCreateInfo colorBlendState = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
		VkPipelineDepthStencilStateCreateInfo depthStencilState = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
		VkPipelineViewportStateCreateInfo viewportState = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
		VkPipelineMultisampleStateCreateInfo multisampleState = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
		std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicState = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);

		// Solid rendering pipeline
		// Load shaders
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

		shaderStages[0] = loadShader(getShadersPath() + "gears/gears.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getShadersPath() + "gears/gears.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

		// Vertex bindings and attributes to match the vertex buffers storing the vertices for the gears
		VkVertexInputBindingDescription vertexInputBinding = {
			vks::initializers::vertexInputBindingDescription(0, sizeof(Gear::Vertex), VK_VERTEX_INPUT_RATE_VERTEX)
		};
		std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = {
			vks::initializers::vertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Gear::Vertex, position)),	// Location 0 : Position
			vks::initializers::vertexInputAttributeDescription(0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Gear::Vertex, normal)),	// Location 1 : Normal
			vks::initializers::vertexInputAttributeDescription(0, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Gear::Vertex, color)),	// Location 2 : Color
		};
		VkPipelineVertexInputStateCreateInfo vertexInputStateCI = vks::initializers::pipelineVertexInputStateCreateInfo();
		vertexInputStateCI.vertexBindingDescriptionCount = 1;
		vertexInputStateCI.pVertexBindingDescriptions = &vertexInputBinding;
		vertexInputStateCI.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
		vertexInputStateCI.pVertexAttributeDescriptions = vertexInputAttributes.data();

		VkGraphicsPipelineCreateInfo pipelineCreateInfo = vks::initializers::pipelineCreateInfo(pipelineLayout, renderPass, 0);
		pipelineCreateInfo.pVertexInputState = &vertexInputStateCI;
		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
		pipelineCreateInfo.pRasterizationState = &rasterizationState;
		pipelineCreateInfo.pColorBlendState = &colorBlendState;
		pipelineCreateInfo.pMultisampleState = &multisampleState;
		pipelineCreateInfo.pViewportState = &viewportState;
		pipelineCreateInfo.pDepthStencilState = &depthStencilState;
		pipelineCreateInfo.pDynamicState = &dynamicState;
		pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineCreateInfo.pStages = shaderStages.data();

		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipeline));
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

			vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
			vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

			VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);
			vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

			vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

			// Vertices, indices and uniform data for all gears are stored in single buffers, so we only need to bind one buffer of each type and then index/offset into that for each separate gear
			VkDeviceSize offsets[1] = { 0 };
			vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
			vkCmdBindVertexBuffers(drawCmdBuffers[i], 0, 1, &vertexBuffer.buffer, offsets);
			vkCmdBindIndexBuffer(drawCmdBuffers[i], indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
			for (auto j = 0; j < numGears; j++) {
				// We use the instance index (last argument) to pass the index of the triangle to the shader
				// With this we can index into the model matrices array of the uniform buffer like this (see gears.vert):
				// ubo.model[gl_InstanceIndex];
				vkCmdDrawIndexed(drawCmdBuffers[i], gears[j].indexCount, 1, gears[j].indexStart, 0, j);
			}

			drawUI(drawCmdBuffers[i]);

			vkCmdEndRenderPass(drawCmdBuffers[i]);

			VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
		}
	}

	void prepareUniformBuffers()
	{
		// Create the vertex shader uniform buffer block
		VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &uniformBuffer, sizeof(UniformData)));
		// Map persistent
		VK_CHECK_RESULT(uniformBuffer.map());
	}

	void updateUniformBuffers()
	{
		float degree = timer * 360.0f;

		// Camera specific global matrices
		uniformData.projection = camera.matrices.perspective;
		uniformData.view = camera.matrices.view;
		uniformData.lightPos = glm::vec4(0.0f, 0.0f, 2.5f, 1.0f);

		// Update the model matrix for each gear that contains it's position and rotation
		for (auto i = 0; i < numGears; i++) {
			Gear gear = gears[i];
			uniformData.model[i] = glm::mat4(1.0f);
			uniformData.model[i] = glm::translate(uniformData.model[i], gear.pos);
			uniformData.model[i] = glm::rotate(uniformData.model[i], glm::radians((gear.rotSpeed * degree) + gear.rotOffset), glm::vec3(0.0f, 0.0f, 1.0f));
		}

		memcpy(uniformBuffer.mapped, &uniformData, sizeof(UniformData));
	}

	void prepare()
	{
		VulkanExampleBase::prepare();
		prepareGears();
		prepareUniformBuffers();
		setupDescriptors();
		preparePipelines();
		buildCommandBuffers();
		prepared = true;
	}

	void draw()
	{
		VulkanExampleBase::prepareFrame();
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
		VulkanExampleBase::submitFrame();
	}

	virtual void render()
	{
		if (!prepared)
			return;
		updateUniformBuffers();
		draw();
	}

};

VULKAN_EXAMPLE_MAIN()