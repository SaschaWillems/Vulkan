/* Copyright (c) 2026, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

#version 450

layout (input_attachment_index = 0, binding = 0) uniform subpassInput positionDepthAttachment;
layout (input_attachment_index = 1, binding = 1) uniform subpassInput normalAttachment;
layout (input_attachment_index = 2, binding = 2) uniform subpassInput albedoAttachment;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;

struct Light {
	vec4 position;
	vec3 color;
	float radius;
};

layout(std140, binding = 3) readonly buffer LightsBuffer
{
   Light lights[ ];
};


void main() 
{
	// Read G-Buffer values from previous sub pass
	vec3 fragPos = subpassLoad(positionDepthAttachment).rgb;
	vec3 normal = subpassLoad(normalAttachment).rgb;
	vec4 albedo = subpassLoad(albedoAttachment);

	#define ambient 0.15
	
	// Ambient part
	vec3 fragcolor  = albedo.rgb * ambient;
	
	for(int i = 0; i < lights.length(); ++i)
	{
		vec3 L = lights[i].position.xyz - fragPos;
		float dist = length(L);

		float attenuation = lights[i].radius / (pow(dist, 8.0) + 1.0);

		float NdotL = max(0.0, dot(normalize(normal), normalize(L)));
		vec3 diffuse = lights[i].color * albedo.rgb * NdotL * attenuation;

		fragcolor += diffuse;
	}    	
   
	outColor = vec4(fragcolor, 1.0);
}