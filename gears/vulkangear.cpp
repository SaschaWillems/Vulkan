/*
* Vulkan Example - Animated gears using multiple uniform buffers
*
* See readme.md for details
*
* Copyright (C) 2015 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "vulkangear.h"

int32_t VulkanGear::newVertex(std::vector<Vertex> *vBuffer, float x, float y, float z, const glm::vec3& normal)
{
	Vertex v(
		glm::vec3(x, y, z),
		normal,
		color
		);
	vBuffer->push_back(v);
	return vBuffer->size() - 1;
}

void VulkanGear::newFace(std::vector<uint32_t> *iBuffer, int a, int b, int c)
{
	iBuffer->push_back(a);
	iBuffer->push_back(b);
	iBuffer->push_back(c);
}

VulkanGear::VulkanGear(VkDevice device, VulkanExampleBase *example)
{
	this->device = device;
	this->exampleBase = example;
}

VulkanGear::~VulkanGear()
{
	// Clean up vulkan resources
	vkDestroyBuffer(device, uniformData.buffer, nullptr);
	vkFreeMemory(device, uniformData.memory, nullptr);

	vkDestroyBuffer(device, vertexBuffer.buf, nullptr);
	vkFreeMemory(device, vertexBuffer.mem, nullptr);

	vkDestroyBuffer(device, indexBuffer.buf, nullptr);
	vkFreeMemory(device, indexBuffer.mem, nullptr);
}

void VulkanGear::generate(float inner_radius, float outer_radius, float width, int teeth, float tooth_depth, glm::vec3 color, glm::vec3 pos, float rotSpeed, float rotOffset)
{
	this->color = color;
	this->pos = pos;
	this->rotOffset = rotOffset;
	this->rotSpeed = rotSpeed;

	std::vector<Vertex> vBuffer;
	std::vector<uint32_t> iBuffer;

	int i, j;
	float r0, r1, r2;
	float ta, da;
	float u1, v1, u2, v2, len;
	float cos_ta, cos_ta_1da, cos_ta_2da, cos_ta_3da, cos_ta_4da;
	float sin_ta, sin_ta_1da, sin_ta_2da, sin_ta_3da, sin_ta_4da;
	int32_t ix0, ix1, ix2, ix3, ix4, ix5;

	r0 = inner_radius;
	r1 = outer_radius - tooth_depth / 2.0;
	r2 = outer_radius + tooth_depth / 2.0;
	da = 2.0 * M_PI / teeth / 4.0;

	glm::vec3 normal;

	for (i = 0; i < teeth; i++)
	{
		ta = i * 2.0 * M_PI / teeth;
		// todo : naming
		cos_ta = cos(ta);
		cos_ta_1da = cos(ta + da);
		cos_ta_2da = cos(ta + 2 * da);
		cos_ta_3da = cos(ta + 3 * da);
		cos_ta_4da = cos(ta + 4 * da);
		sin_ta = sin(ta);
		sin_ta_1da = sin(ta + da);
		sin_ta_2da = sin(ta + 2 * da);
		sin_ta_3da = sin(ta + 3 * da);
		sin_ta_4da = sin(ta + 4 * da);

		u1 = r2 * cos_ta_1da - r1 * cos_ta;
		v1 = r2 * sin_ta_1da - r1 * sin_ta;
		len = sqrt(u1 * u1 + v1 * v1);
		u1 /= len;
		v1 /= len;
		u2 = r1 * cos_ta_3da - r2 * cos_ta_2da;
		v2 = r1 * sin_ta_3da - r2 * sin_ta_2da;

		// front face
		normal = glm::vec3(0.0, 0.0, 1.0);
		ix0 = newVertex(&vBuffer, r0 * cos_ta, r0 * sin_ta, width * 0.5, normal);
		ix1 = newVertex(&vBuffer, r1 * cos_ta, r1 * sin_ta, width * 0.5, normal);
		ix2 = newVertex(&vBuffer, r0 * cos_ta, r0 * sin_ta, width * 0.5, normal);
		ix3 = newVertex(&vBuffer, r1 * cos_ta_3da, r1 * sin_ta_3da, width * 0.5, normal);
		ix4 = newVertex(&vBuffer, r0 * cos_ta_4da, r0 * sin_ta_4da, width * 0.5, normal);
		ix5 = newVertex(&vBuffer, r1 * cos_ta_4da, r1 * sin_ta_4da, width * 0.5, normal);
		newFace(&iBuffer, ix0, ix1, ix2);
		newFace(&iBuffer, ix1, ix3, ix2);
		newFace(&iBuffer, ix2, ix3, ix4);
		newFace(&iBuffer, ix3, ix5, ix4);

		// front sides of teeth
		normal = glm::vec3(0.0, 0.0, 1.0);
		ix0 = newVertex(&vBuffer, r1 * cos_ta, r1 * sin_ta, width * 0.5, normal);
		ix1 = newVertex(&vBuffer, r2 * cos_ta_1da, r2 * sin_ta_1da, width * 0.5, normal);
		ix2 = newVertex(&vBuffer, r1 * cos_ta_3da, r1 * sin_ta_3da, width * 0.5, normal);
		ix3 = newVertex(&vBuffer, r2 * cos_ta_2da, r2 * sin_ta_2da, width * 0.5, normal);
		newFace(&iBuffer, ix0, ix1, ix2);
		newFace(&iBuffer, ix1, ix3, ix2);

		// back face 
		normal = glm::vec3(0.0, 0.0, -1.0);
		ix0 = newVertex(&vBuffer, r1 * cos_ta, r1 * sin_ta, -width * 0.5, normal);
		ix1 = newVertex(&vBuffer, r0 * cos_ta, r0 * sin_ta, -width * 0.5, normal);
		ix2 = newVertex(&vBuffer, r1 * cos_ta_3da, r1 * sin_ta_3da, -width * 0.5, normal);
		ix3 = newVertex(&vBuffer, r0 * cos_ta, r0 * sin_ta, -width * 0.5, normal);
		ix4 = newVertex(&vBuffer, r1 * cos_ta_4da, r1 * sin_ta_4da, -width * 0.5, normal);
		ix5 = newVertex(&vBuffer, r0 * cos_ta_4da, r0 * sin_ta_4da, -width * 0.5, normal);
		newFace(&iBuffer, ix0, ix1, ix2);
		newFace(&iBuffer, ix1, ix3, ix2);
		newFace(&iBuffer, ix2, ix3, ix4);
		newFace(&iBuffer, ix3, ix5, ix4);

		// back sides of teeth 
		normal = glm::vec3(0.0, 0.0, -1.0);
		ix0 = newVertex(&vBuffer, r1 * cos_ta_3da, r1 * sin_ta_3da, -width * 0.5, normal);
		ix1 = newVertex(&vBuffer, r2 * cos_ta_2da, r2 * sin_ta_2da, -width * 0.5, normal);
		ix2 = newVertex(&vBuffer, r1 * cos_ta, r1 * sin_ta, -width * 0.5, normal);
		ix3 = newVertex(&vBuffer, r2 * cos_ta_1da, r2 * sin_ta_1da, -width * 0.5, normal);
		newFace(&iBuffer, ix0, ix1, ix2);
		newFace(&iBuffer, ix1, ix3, ix2);

		// draw outward faces of teeth 
		normal = glm::vec3(v1, -u1, 0.0);
		ix0 = newVertex(&vBuffer, r1 * cos_ta, r1 * sin_ta, width * 0.5, normal);
		ix1 = newVertex(&vBuffer, r1 * cos_ta, r1 * sin_ta, -width * 0.5, normal);
		ix2 = newVertex(&vBuffer, r2 * cos_ta_1da, r2 * sin_ta_1da, width * 0.5, normal);
		ix3 = newVertex(&vBuffer, r2 * cos_ta_1da, r2 * sin_ta_1da, -width * 0.5, normal);
		newFace(&iBuffer, ix0, ix1, ix2);
		newFace(&iBuffer, ix1, ix3, ix2);

		normal = glm::vec3(cos_ta, sin_ta, 0.0);
		ix0 = newVertex(&vBuffer, r2 * cos_ta_1da, r2 * sin_ta_1da, width * 0.5, normal);
		ix1 = newVertex(&vBuffer, r2 * cos_ta_1da, r2 * sin_ta_1da, -width * 0.5, normal);
		ix2 = newVertex(&vBuffer, r2 * cos_ta_2da, r2 * sin_ta_2da, width * 0.5, normal);
		ix3 = newVertex(&vBuffer, r2 * cos_ta_2da, r2 * sin_ta_2da, -width * 0.5, normal);
		newFace(&iBuffer, ix0, ix1, ix2);
		newFace(&iBuffer, ix1, ix3, ix2);

		normal = glm::vec3(v2, -u2, 0.0);
		ix0 = newVertex(&vBuffer, r2 * cos_ta_2da, r2 * sin_ta_2da, width * 0.5, normal);
		ix1 = newVertex(&vBuffer, r2 * cos_ta_2da, r2 * sin_ta_2da, -width * 0.5, normal);
		ix2 = newVertex(&vBuffer, r1 * cos_ta_3da, r1 * sin_ta_3da, width * 0.5, normal);
		ix3 = newVertex(&vBuffer, r1 * cos_ta_3da, r1 * sin_ta_3da, -width * 0.5, normal);
		newFace(&iBuffer, ix0, ix1, ix2);
		newFace(&iBuffer, ix1, ix3, ix2);

		normal = glm::vec3(cos_ta, sin_ta, 0.0);
		ix0 = newVertex(&vBuffer, r1 * cos_ta_3da, r1 * sin_ta_3da, width * 0.5, normal);
		ix1 = newVertex(&vBuffer, r1 * cos_ta_3da, r1 * sin_ta_3da, -width * 0.5, normal);
		ix2 = newVertex(&vBuffer, r1 * cos_ta_4da, r1 * sin_ta_4da, width * 0.5, normal);
		ix3 = newVertex(&vBuffer, r1 * cos_ta_4da, r1 * sin_ta_4da, -width * 0.5, normal);
		newFace(&iBuffer, ix0, ix1, ix2);
		newFace(&iBuffer, ix1, ix3, ix2);

		// draw inside radius cylinder 
		ix0 = newVertex(&vBuffer, r0 * cos_ta, r0 * sin_ta, -width * 0.5, glm::vec3(-cos_ta, -sin_ta, 0.0));
		ix1 = newVertex(&vBuffer, r0 * cos_ta, r0 * sin_ta, width * 0.5, glm::vec3(-cos_ta, -sin_ta, 0.0));
		ix2 = newVertex(&vBuffer, r0 * cos_ta_4da, r0 * sin_ta_4da, -width * 0.5, glm::vec3(-cos_ta_4da, -sin_ta_4da, 0.0));
		ix3 = newVertex(&vBuffer, r0 * cos_ta_4da, r0 * sin_ta_4da, width * 0.5, glm::vec3(-cos_ta_4da, -sin_ta_4da, 0.0));
		newFace(&iBuffer, ix0, ix1, ix2);
		newFace(&iBuffer, ix1, ix3, ix2);
	}

	int vertexBufferSize = vBuffer.size() * sizeof(Vertex);
	int indexBufferSize = iBuffer.size() * sizeof(uint32_t);

	VkMemoryAllocateInfo memAlloc = vkTools::initializers::memoryAllocateInfo();
	VkMemoryRequirements memReqs;

	VkResult err;
	void *data;

	// Generate vertex buffer
	VkBufferCreateInfo vBufferInfo = vkTools::initializers::bufferCreateInfo(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertexBufferSize);
	err = vkCreateBuffer(device, &vBufferInfo, nullptr, &vertexBuffer.buf);
	assert(!err);
	vkGetBufferMemoryRequirements(device, vertexBuffer.buf, &memReqs);
	memAlloc.allocationSize = memReqs.size;
	exampleBase->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memAlloc.memoryTypeIndex);
	err = vkAllocateMemory(device, &memAlloc, nullptr, &vertexBuffer.mem);
	assert(!err);
	err = vkMapMemory(device, vertexBuffer.mem, 0, vertexBufferSize, 0, &data);
	assert(!err);
	memcpy(data, vBuffer.data(), vertexBufferSize);
	vkUnmapMemory(device, vertexBuffer.mem);
	err = vkBindBufferMemory(device, vertexBuffer.buf, vertexBuffer.mem, 0);
	assert(!err);

	// Generate index buffer
	VkBufferCreateInfo iBufferInfo = vkTools::initializers::bufferCreateInfo(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, indexBufferSize);
	err = vkCreateBuffer(device, &iBufferInfo, nullptr, &indexBuffer.buf);
	assert(!err);
	vkGetBufferMemoryRequirements(device, indexBuffer.buf, &memReqs);
	memAlloc.allocationSize = memReqs.size;
	exampleBase->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memAlloc.memoryTypeIndex);
	err = vkAllocateMemory(device, &memAlloc, nullptr, &indexBuffer.mem);
	assert(!err);
	err = vkMapMemory(device, indexBuffer.mem, 0, indexBufferSize, 0, &data);
	assert(!err);
	memcpy(data, iBuffer.data(), indexBufferSize);
	vkUnmapMemory(device, indexBuffer.mem);
	err = vkBindBufferMemory(device, indexBuffer.buf, indexBuffer.mem, 0);
	assert(!err);
	indexBuffer.count = iBuffer.size();

	prepareUniformBuffer();
}

void VulkanGear::draw(VkCommandBuffer cmdbuffer, VkPipelineLayout pipelineLayout)
{
	VkDeviceSize offsets[1] = { 0 };
	vkCmdBindDescriptorSets(cmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);
	vkCmdBindVertexBuffers(cmdbuffer, 0, 1, &vertexBuffer.buf, offsets);
	vkCmdBindIndexBuffer(cmdbuffer, indexBuffer.buf, 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(cmdbuffer, indexBuffer.count, 1, 0, 0, 1);
}

void VulkanGear::updateUniformBuffer(glm::mat4 perspective, glm::vec3 rotation, float zoom, float timer)
{
	ubo.projection = perspective;

	ubo.view = glm::lookAt(
		glm::vec3(0, 0, -zoom),
		glm::vec3(-1.0, -1.5, 0),
		glm::vec3(0, 1, 0)
		);
	ubo.view = glm::rotate(ubo.view, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	ubo.view = glm::rotate(ubo.view, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));

	ubo.model = glm::mat4();
	ubo.model = glm::translate(ubo.model, pos);
	rotation.z = (rotSpeed * timer) + rotOffset;
	ubo.model = glm::rotate(ubo.model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

	ubo.normal = glm::inverseTranspose(ubo.view * ubo.model);

	//ubo.lightPos = lightPos;
	ubo.lightPos = glm::vec3(0.0f, 0.0f, 2.5f);
	ubo.lightPos.x = sin(glm::radians(timer)) * 8.0f;
	ubo.lightPos.z = cos(glm::radians(timer)) * 8.0f;

	uint8_t *pData;
	VkResult err = vkMapMemory(device, uniformData.memory, 0, sizeof(ubo), 0, (void **)&pData);
	assert(!err);
	memcpy(pData, &ubo, sizeof(ubo));
	vkUnmapMemory(device, uniformData.memory);
}

void VulkanGear::setupDescriptorSet(VkDescriptorPool pool, VkDescriptorSetLayout descriptorSetLayout)
{
	VkDescriptorSetAllocateInfo allocInfo =
		vkTools::initializers::descriptorSetAllocateInfo(
			pool,
			&descriptorSetLayout,
			1);

	VkResult vkRes = vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet);
	assert(!vkRes);

	// Binding 0 : Vertex shader uniform buffer
	VkWriteDescriptorSet writeDescriptorSet =
		vkTools::initializers::writeDescriptorSet(
			descriptorSet,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			0,
			&uniformData.descriptor);

	vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, NULL);
}

void VulkanGear::prepareUniformBuffer()
{
	VkResult err;

	// Vertex shader uniform buffer block
	VkMemoryAllocateInfo allocInfo = vkTools::initializers::memoryAllocateInfo();
	VkMemoryRequirements memReqs;

	VkBufferCreateInfo bufferInfo = vkTools::initializers::bufferCreateInfo(
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		sizeof(ubo));

	err = vkCreateBuffer(device, &bufferInfo, nullptr, &uniformData.buffer);
	assert(!err);
	vkGetBufferMemoryRequirements(device, uniformData.buffer, &memReqs);
	allocInfo.allocationSize = memReqs.size;
	exampleBase->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &allocInfo.memoryTypeIndex);
	err = vkAllocateMemory(device, &allocInfo, nullptr, &uniformData.memory);
	assert(!err);
	err = vkBindBufferMemory(device, uniformData.buffer, uniformData.memory, 0);
	assert(!err);

	uniformData.descriptor.buffer = uniformData.buffer;
	uniformData.descriptor.offset = 0;
	uniformData.descriptor.range = sizeof(ubo);
	uniformData.allocSize = allocInfo.allocationSize;
}
