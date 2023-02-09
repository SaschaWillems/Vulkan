#version 450

layout (location = 0) out float outFragColor;

layout (location = 0) in vec4 inPos;
layout (location = 1) in vec3 inLightPos;

void main() 
{
	// Store distance to light as 32 bit float value
    vec3 lightVec = inPos.xyz - inLightPos;
    outFragColor = length(lightVec);
}