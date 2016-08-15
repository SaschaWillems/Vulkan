#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 model;
	vec4 viewPos;
	float lodBias;
	float samplerIndex;
} ubo;

layout (location = 0) out vec2 outUV;
layout (location = 1) out float outLodBias;
layout (location = 2) out float outSamplerIndex;

out gl_PerVertex 
{
	vec4 gl_Position;   
};

void main() 
{
	outUV = inUV;
	outUV.s *= 10.0;
	outLodBias = ubo.lodBias;
	outSamplerIndex = ubo.samplerIndex;
	gl_Position = ubo.projection * ubo.model * vec4(inPos, 1.0);
}
