#version 450

layout (binding = 2) uniform sampler2D samplerColor;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	outFragColor = texture(samplerColor, inUV);
}