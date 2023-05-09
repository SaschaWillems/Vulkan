#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 model;
	vec4 viewPos;
	float lodBias;
} ubo;

layout (location = 0) out vec2 outUV;
layout (location = 1) out float outLodBias;

void main() 
{
	outUV = inUV;
	outLodBias = ubo.lodBias;
	gl_Position = ubo.projection * ubo.model * vec4(inPos.xyz, 1.0);
}
