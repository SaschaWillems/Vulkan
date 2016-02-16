#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#define lightCount 6

layout (location = 0) in vec3 inNormal;
layout (location = 2) in vec3 inColor;

layout (location = 3) in vec4 inLightVec[lightCount];

layout (location = 0) out vec4 outFragColor;

#define MAX_LIGHT_DIST 9.0 * 9.0

void main() 
{
	vec3 lightColor[lightCount];
	lightColor[0] = vec3(1.0, 0.0, 0.0);
	lightColor[1] = vec3(0.0, 1.0, 0.0);
	lightColor[2] = vec3(0.0, 0.0, 1.0);
	lightColor[3] = vec3(1.0, 0.0, 1.0);
	lightColor[4] = vec3(0.0, 1.0, 1.0);
	lightColor[5] = vec3(1.0, 1.0, 0.0);
	
	vec3 diffuse = vec3(0.0);
	// Just some very basic attenuation
	for (int i = 0; i < lightCount; ++i)
	{				
		float lRadius =  MAX_LIGHT_DIST * inLightVec[i].w;
	
		float dist = min(dot(inLightVec[i], inLightVec[i]), lRadius) / lRadius;
		float distFactor = 1.0 - dist;		
	
		diffuse += lightColor[i] * distFactor;		
	}
			
	outFragColor.rgb = diffuse;			
}