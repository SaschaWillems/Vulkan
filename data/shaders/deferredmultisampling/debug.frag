#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 1) uniform sampler2DMS samplerPosition;
layout (binding = 2) uniform sampler2DMS samplerNormal;
layout (binding = 3) uniform sampler2DMS samplerAlbedo;

layout (location = 0) in vec3 inUV;

layout (location = 0) out vec4 outFragColor;

#define NUM_SAMPLES 8

vec4 resolve(sampler2DMS tex, ivec2 uv)
{
	vec4 result = vec4(0.0);	   
	int count = 0;
	for (int i = 0; i < NUM_SAMPLES; i++)
	{
		vec4 val = texelFetch(tex, uv, i); 
		result += val;
		count++;
	}    
	return result / float(NUM_SAMPLES);
}

void main() 
{
	ivec2 attDim = textureSize(samplerPosition);
	ivec2 UV = ivec2(inUV.st * attDim * 2.0);

	highp int index = 0;
	if (inUV.s > 0.5)
	{
		index = 1;
		UV.s -= attDim.x;
	}
	if (inUV.t > 0.5)
	{
		index = 2;
		UV.t -= attDim.y;
	}

	vec3 components[3];
	components[0] = resolve(samplerPosition, UV).rgb;  
	components[1] = resolve(samplerNormal, UV).rgb;  
	components[2] = resolve(samplerAlbedo, UV).rgb;  
	// Uncomment to display specular component
	//components[2] = vec3(texture(samplerAlbedo, inUV.st).a);  
	
	// Select component depending on UV
	outFragColor.rgb = components[index];
}