#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 1) uniform sampler2D samplerColorMap;

layout (location = 0) in vec2 inUV;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inViewVec;
layout (location = 3) in vec3 inLightVec;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	vec4 color = texture(samplerColorMap, inUV);

	float distSqr = dot(inLightVec, inLightVec);
	vec3 lVec = inLightVec * inversesqrt(distSqr);

	const float attInvRadius = 1.0/5000.0;	
	float atten = max(clamp(1.0 - attInvRadius * sqrt(distSqr), 0.0, 1.0), 0.0);

	// Fake drop shadow	
	const float shadowInvRadius = 1.0/2500.0;
	float dropshadow = max(clamp(1.0 - shadowInvRadius * sqrt(distSqr), 0.0, 1.0), 0.0);
	
	outFragColor = vec4(color.rgba * (1.0 - dropshadow));
	outFragColor.rgb *= atten;
}