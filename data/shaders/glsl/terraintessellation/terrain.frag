#version 450

layout (set = 0, binding = 1) uniform sampler2D samplerHeight; 
layout (set = 0, binding = 2) uniform sampler2DArray samplerLayers;

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inViewVec;
layout (location = 3) in vec3 inLightVec;
layout (location = 4) in vec3 inEyePos;
layout (location = 5) in vec3 inWorldPos;

layout (location = 0) out vec4 outFragColor;

vec3 sampleTerrainLayer()
{
	// Define some layer ranges for sampling depending on terrain height
	vec2 layers[6];
	layers[0] = vec2(-10.0, 10.0);
	layers[1] = vec2(5.0, 45.0);
	layers[2] = vec2(45.0, 80.0);
	layers[3] = vec2(75.0, 100.0);
	layers[4] = vec2(95.0, 140.0);
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

float fog(float density)
{
	const float LOG2 = -1.442695;
	float dist = gl_FragCoord.z / gl_FragCoord.w * 0.1;
	float d = density * dist;
	return 1.0 - clamp(exp2(d * d * LOG2), 0.0, 1.0);
}

void main()
{
	vec3 N = normalize(inNormal);
	vec3 L = normalize(inLightVec);
	vec3 ambient = vec3(0.5);
	vec3 diffuse = max(dot(N, L), 0.0) * vec3(1.0);

	vec4 color = vec4((ambient + diffuse) * sampleTerrainLayer(), 1.0);

	const vec4 fogColor = vec4(0.47, 0.5, 0.67, 0.0);
	outFragColor  = mix(color, fogColor, fog(0.25));	
}
