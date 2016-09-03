#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 view;
	mat4 model;
	float lodBias;
	int samplerIndex;
} ubo;

layout (location = 0) out vec2 outUV;
layout (location = 1) out float outLodBias;
layout (location = 2) flat out int outSamplerIndex;

out gl_PerVertex 
{
	vec4 gl_Position;   
};

void main() 
{
	outUV = inUV * vec2(50.0, 2.0);
	outLodBias = ubo.lodBias;
	outSamplerIndex = ubo.samplerIndex;
	gl_Position = ubo.projection * ubo.view * ubo.model * vec4(inPos, 1.0);
}
