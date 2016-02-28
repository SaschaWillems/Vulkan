#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 1) uniform samplerCube shadowCubeMap;

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec3 inEyePos;
layout (location = 3) in vec3 inLightVec;
layout (location = 4) in vec3 inWorldPos;
layout (location = 5) in vec3 inLightPos;

layout (location = 0) out vec4 outFragColor;

#define EPSILON 0.15
#define SHADOW_OPACITY 0.5

void main() 
{
	// Lighting
	vec3 N = normalize(inNormal);
	vec3 L = normalize(vec3(1.0));	
	
	vec3 Eye = normalize(-inEyePos);
	vec3 Reflected = normalize(reflect(-inLightVec, inNormal)); 

	vec4 IAmbient = vec4(vec3(0.05), 1.0);
	vec4 IDiffuse = vec4(1.0) * max(dot(inNormal, inLightVec), 0.0);

	outFragColor = vec4(IAmbient + IDiffuse * vec4(inColor, 1.0));		
		
	// Shadow
	vec3 lightVec = inWorldPos - inLightPos;
    float sampledDist = texture(shadowCubeMap, lightVec).r;
    float dist = length(lightVec);

	// Check if fragment is in shadow
    float shadow = (dist <= sampledDist + EPSILON) ? 1.0 : SHADOW_OPACITY;
    		
	outFragColor.rgb *= shadow;
}