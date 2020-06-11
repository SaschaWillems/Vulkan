#version 450

// Vertex attributes
layout (location = 0) in vec4 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inColor;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 modelview;
} ubo;

layout (location = 0) out vec2 outUV;

void main() 
{
	outUV = inUV * 32.0;
	gl_Position = ubo.projection * ubo.modelview * vec4(inPos.xyz, 1.0);
}
