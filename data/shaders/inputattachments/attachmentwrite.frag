#version 450

layout (location = 0) in vec3 inColor;

layout (location = 0) out vec4 outColor;

void main() 
{
	outColor = vec4(inColor, 0.0);
	// Depth attachment does not need to be explicitly written
}