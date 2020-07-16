#version 450

#extension GL_ARB_sparse_texture2 : enable
#extension GL_ARB_sparse_texture_clamp : enable

layout (binding = 1) uniform sampler2D samplerColor;

layout (location = 0) in vec2 inUV;
layout (location = 1) in float inLodBias;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	vec4 color = vec4(0.0);

	// Get residency code for current texel
	int residencyCode = sparseTextureARB(samplerColor, inUV, color, inLodBias);

	// Fetch sparse until we get a valid texel
	/*
	float minLod = 1.0;
	while (!sparseTexelsResidentARB(residencyCode)) 
	{
		residencyCode = sparseTextureClampARB(samplerColor, inUV, minLod, color);
		minLod += 1.0f;
	}
	*/

	// Check if texel is resident
	bool texelResident = sparseTexelsResidentARB(residencyCode);

	if (!texelResident)
	{
		color = vec4(0.0, 0.0, 0.0, 0.0);
	}

	outFragColor = color;
}