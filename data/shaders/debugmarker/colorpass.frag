#version 450

layout (location = 0) in vec3 inColor;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	outFragColor.rgb = inColor;
}