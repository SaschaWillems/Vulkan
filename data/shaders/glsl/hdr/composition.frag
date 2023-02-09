#version 450

layout (binding = 0) uniform sampler2D samplerColor0;
layout (binding = 1) uniform sampler2D samplerColor1;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;

void main() 
{
	outColor = texture(samplerColor0, inUV);
}