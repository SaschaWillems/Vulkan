#version 450

layout (set = 0, binding = 2) uniform sampler2D samplerColorMap;

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	outFragColor = texture(samplerColorMap, inUV) * vec4(inColor, 1.0);
}