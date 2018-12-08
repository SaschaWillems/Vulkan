#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUV;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 modelview;
} ubo;

layout (location = 0) out vec2 outUV;

void main() 
{
	outUV = inUV;
	gl_Position = ubo.projection * ubo.modelview * vec4(inPos.xyz, 1.0);
}
