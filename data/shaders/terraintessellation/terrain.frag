#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (set = 0, binding = 1) uniform sampler2D displacementMap; 
layout (set = 0, binding = 2) uniform sampler2DArray terrainLayers;

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inEyePos;
layout (location = 3) in vec3 inLightVec;

layout (location = 0) out vec4 outFragColor;

vec4 sampleTerrainLayer()
{
	// Define some layer ranges for sampling depending on terrain height
	vec2 layers[6];
	layers[0] = vec2(-10.0, 10.0);
	layers[1] = vec2(5.0, 35.0);
	layers[2] = vec2(30.0, 70.0);
	layers[3] = vec2(60.0, 95.0);
	layers[4] = vec2(85.0, 140.0);
	layers[5] = vec2(140.0, 190.0);

	vec3 color = vec3(0.0);
	
	// Get height from displacement map
	float height = textureLod(displacementMap, inUV, 0.0).r * 255.0;
	
	for (int i = 0; i < 6; i++)
	{
		float range = layers[i].y - layers[i].x;
		float weight = (range - abs(height - layers[i].y)) / range;
		weight = max(0.0, weight);
		color += weight * texture(terrainLayers, vec3(inUV * 16.0, i)).rgb;
	}

	return vec4(color, 1.0);
}

void main()
{
/* todo: no lighting yet
	vec3 N = normalize(inNormal);
	vec3 L = normalize(vec3(1.0));	
	vec3 Eye = normalize(inEyePos);
	vec3 Reflected = normalize(reflect(-inLightVec, inNormal)); 

	vec4 IAmbient = vec4(vec3(0.15), 1.0);
	vec4 IDiffuse = vec4(1.0) * max(dot(inNormal, inLightVec), 0.0);

	outFragColor = vec4((IAmbient + IDiffuse) * vec4(texture(terrainLayers, vec3(inUV, 0.0)).rgb, 1.0));	
*/
	outFragColor = sampleTerrainLayer();
}