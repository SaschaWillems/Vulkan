#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (input_attachment_index = 0, binding = 1) uniform subpassInput samplerPositionDepth;
layout (binding = 2) uniform sampler2D samplerTexture;

layout (location = 0) in vec3 inColor;
layout (location = 1) in vec2 inUV;

layout (location = 0) out vec4 outColor;

layout (constant_id = 0) const float NEAR_PLANE = 0.1f;
layout (constant_id = 1) const float FAR_PLANE = 256.0f;

float linearDepth(float depth)
{
	float z = depth * 2.0f - 1.0f; 
	return (2.0f * NEAR_PLANE * FAR_PLANE) / (FAR_PLANE + NEAR_PLANE - z * (FAR_PLANE - NEAR_PLANE));	
}

void main () 
{
	// Sample depth from deferred depth buffer and discard if obscured
	float depth = subpassLoad(samplerPositionDepth).a;
	if ((depth != 0.0) && (linearDepth(gl_FragCoord.z) > depth))
	{
		discard;
	};

	outColor = texture(samplerTexture, inUV);
}
