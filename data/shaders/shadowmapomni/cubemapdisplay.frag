#version 450

layout (binding = 1) uniform samplerCube shadowCubeMap;

layout (location = 0) in vec3 inUVW;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	float dist = length(texture(shadowCubeMap, inUVW).rgb) * 0.005;
	outFragColor = vec4(vec3(dist), 1.0);
}