#version 450

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec3 inPos;

layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outAlbedo;

layout (set = 0, binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 model;
	mat4 view;
	float nearPlane;
	float farPlane;
} ubo;

layout (set = 1, binding = 0) uniform sampler2D samplerColormap;

float linearDepth(float depth)
{
	float z = depth * 2.0f - 1.0f; 
	return (2.0f * ubo.nearPlane * ubo.farPlane) / (ubo.farPlane + ubo.nearPlane - z * (ubo.farPlane - ubo.nearPlane));	
}

void main() 
{
	outPosition = vec4(inPos, linearDepth(gl_FragCoord.z));
	outNormal = vec4(normalize(inNormal) * 0.5 + 0.5, 1.0);
	outAlbedo = texture(samplerColormap, inUV) * vec4(inColor, 1.0);
}