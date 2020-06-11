#version 450

// Vertex attributes
layout (location = 0) in vec4 inPos;
layout (location = 2) in vec2 inUV;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 modelview;
} ubo;

layout (location = 0) out vec2 outUV;

void main() 
{
	outUV = inUV;
	// Skysphere always at center, only use rotation part of modelview matrix
	gl_Position = ubo.projection * mat4(mat3(ubo.modelview)) * vec4(inPos.xyz, 1.0);
}
