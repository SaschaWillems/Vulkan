/*
* Vulkan Example - Animated gears using multiple uniform buffers
*
* See readme.md for details
*
* Copyright (C) 2015 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#include <math.h>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include "vulkan/vulkan.h"

#include "vulkantools.h"
#include "vulkandevice.hpp"
#include "vulkanbuffer.hpp"

struct Vertex
{
	float pos[3];
	float normal[3];
	float color[3];

	Vertex(const glm::vec3& p, const glm::vec3& n, const glm::vec3& c)
	{
		pos[0] = p.x;
		pos[1] = p.y;
		pos[2] = p.z;
		color[0] = c.x;
		color[1] = c.y;
		color[2] = c.z;
		normal[0] = n.x;
		normal[1] = n.y;
		normal[2] = n.z;
	}
};

struct GearInfo
{
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

class VulkanGear
{
private:
	struct UBO
	{
		glm::mat4 projection;
		glm::mat4 model;
		glm::mat4 normal;
		glm::mat4 view;
		glm::vec3 lightPos;
	};

	vk::VulkanDevice *vulkanDevice;

	glm::vec3 color;
	glm::vec3 pos;
	float rotSpeed;
	float rotOffset;

	vk::Buffer vertexBuffer;
	vk::Buffer indexBuffer;
	uint32_t indexCount;

	UBO ubo;
	vkTools::UniformData uniformData;

	int32_t newVertex(std::vector<Vertex> *vBuffer, float x, float y, float z, const glm::vec3& normal);
	void newFace(std::vector<uint32_t> *iBuffer, int a, int b, int c);

	void prepareUniformBuffer();
public:
	VkDescriptorSet descriptorSet;

	void draw(VkCommandBuffer cmdbuffer, VkPipelineLayout pipelineLayout);
	void updateUniformBuffer(glm::mat4 perspective, glm::vec3 rotation, float zoom, float timer);

	void setupDescriptorSet(VkDescriptorPool pool, VkDescriptorSetLayout descriptorSetLayout);

	VulkanGear(vk::VulkanDevice *vulkanDevice) : vulkanDevice(vulkanDevice) {};
	~VulkanGear();

	void generate(GearInfo *gearinfo, VkQueue queue);

};

