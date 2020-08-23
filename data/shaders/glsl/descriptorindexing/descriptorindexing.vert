#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUV;
layout (location = 2) in int inTextureIndex;

layout (binding = 0) uniform Matrices
{
	mat4 projection;
	mat4 view;
	mat4 model;
} matrices;

layout (location = 0) out vec2 outUV;
layout (location = 1) flat out int outTexIndex;

void main() 
{
	outUV = inUV;
	outTexIndex = 0;
	outTexIndex = inTextureIndex;
	outTexIndex = gl_InstanceIndex;
	vec4 pos = vec4(inPos, 1.0f);
	pos.y += gl_InstanceIndex;
	gl_Position = matrices.projection * matrices.view * matrices.model * pos;
}
