#version 450

layout (location = 0) in vec4 inPos;
layout (location = 2) in vec3 inColor;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 model;
} ubo;

layout (location = 0) out vec3 outColor;

void main() 
{
	outColor = inColor;
	gl_Position = ubo.projection * ubo.model * inPos;
}
