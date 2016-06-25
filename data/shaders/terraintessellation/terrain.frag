#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (set = 0, binding = 1) uniform sampler2D samplerHeight; 
layout (set = 0, binding = 2) uniform sampler2DArray samplerLayers;

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inViewVec;
layout (location = 3) in vec3 inLightVec;

layout (location = 0) out vec4 outFragColor;

vec3 sampleTerrainLayer()
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
	float height = textureLod(samplerHeight, inUV, 0.0).r * 255.0;
	
	for (int i = 0; i < 6; i++)
	{
		float range = layers[i].y - layers[i].x;
		float weight = (range - abs(height - layers[i].y)) / range;
		weight = max(0.0, weight);
		color += weight * texture(samplerLayers, vec3(inUV * 16.0, i)).rgb;
	}

	return color;
}

void main()
{
	vec3 N = normalize(inNormal);
	vec3 L = normalize(inLightVec);
	vec3 ambient = vec3(0.5);
	vec3 diffuse = max(dot(N, L), 0.0) * vec3(1.0);
	outFragColor = vec4((ambient + diffuse) * sampleTerrainLayer(), 1.0);	
}
