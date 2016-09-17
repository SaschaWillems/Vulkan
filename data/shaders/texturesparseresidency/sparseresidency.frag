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

//#define MIN_LOD
#ifdef MIN_LOD
	// Fetch sparse until we get a valid texel
	// todo: does not work in SPIR-V with current drivers (will be fixed in a new release)
	float minLod = 1.0;
	while (!sparseTexelsResidentARB(residencyCode)) 
	{
		residencyCode = sparseTextureClampARB(samplerColor, inUV, minLod, color);
		minLod += 1.0f;
	} 
#endif
	// Check if texel is resident
	bool texelResident = sparseTexelsResidentARB(residencyCode);

	float lodClamp = 1.0f; 
	if (texelResident)
	{
		color = texture(samplerColor, inUV, inLodBias);
	}
	else
	{
		color = vec4(1.0, 0.0, 0.0, 0.0);
	}

	outFragColor = vec4(color.rgb, 1.0);	
}