#version 450

layout (binding = 1) uniform sampler2D colorMap;

layout (location = 0) in vec3 inColor;
layout (location = 1) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	outFragColor.rgb = inColor;
//	outFragColor = texture(colorMap, inUV);// * vec4(inColor, 1.0);
}