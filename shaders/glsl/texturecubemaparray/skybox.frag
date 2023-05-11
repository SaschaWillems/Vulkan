#version 450

layout (binding = 1) uniform samplerCubeArray samplerCubeMapArray;

layout (binding = 0) uniform UBO
{
	mat4 projection;
	mat4 model;
	mat4 invModel;
	float lodBias;
	int cubeMapIndex;
} ubo;

layout (location = 0) in vec3 inUVW;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	outFragColor = textureLod(samplerCubeMapArray, vec4(inUVW, ubo.cubeMapIndex), ubo.lodBias);
}