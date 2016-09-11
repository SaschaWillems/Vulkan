#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
// Required for the sparse* commands used in this shader
#extension GL_ARB_sparse_texture2 : enable

layout (binding = 1) uniform sampler2D samplerColor;

layout (location = 0) in vec2 inUV;
layout (location = 1) in float inLodBias;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inViewVec;
layout (location = 4) in vec3 inLightVec;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	// Get residency code for current texel
	int residencyCode = sparseTextureARB(samplerColor, inUV, outFragColor);
	// Check if texel is resident
	bool texelResident = sparseTexelsResidentARB(residencyCode);

	vec4 color;

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