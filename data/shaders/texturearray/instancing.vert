#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec4 inPos;
layout (location = 1) in vec2 inUV;

struct Instance
{
	mat4 model;
	vec4 arrayIndex;
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
	outUV = vec3(inUV, ubo.instance[gl_InstanceIndex].arrayIndex.x);
	mat4 modelView = ubo.view * ubo.instance[gl_InstanceIndex].model;
	gl_Position = ubo.projection * modelView * inPos;
}
