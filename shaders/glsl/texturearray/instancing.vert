#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUV;

struct Instance
{
	mat4 model;
	float arrayIndex;
};

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 view;
	Instance instance[8];
} ubo;

layout (location = 0) out vec3 outUV;

void main() 
{
	outUV = vec3(inUV, ubo.instance[gl_InstanceIndex].arrayIndex);
	mat4 modelView = ubo.view * ubo.instance[gl_InstanceIndex].model;
	gl_Position = ubo.projection * modelView * vec4(inPos, 1.0);
}
