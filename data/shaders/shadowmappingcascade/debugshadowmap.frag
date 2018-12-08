#version 450

layout (binding = 1) uniform sampler2DArray shadowMap;

layout (location = 0) in vec2 inUV;
layout (location = 1) flat in uint inCascadeIndex;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	float depth = texture(shadowMap, vec3(inUV, float(inCascadeIndex))).r;
	outFragColor = vec4(vec3((depth)), 1.0);
}