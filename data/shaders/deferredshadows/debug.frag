#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 1) uniform sampler2D samplerPosition;
layout (binding = 2) uniform sampler2D samplerNormal;
layout (binding = 3) uniform sampler2D samplerAlbedo;
layout (binding = 5) uniform sampler2DArray samplerDepth;

layout (location = 0) in vec3 inUV;

layout (location = 0) out vec4 outFragColor;

float LinearizeDepth(float depth)
{
  float n = 1.0; // camera z near
  float f = 96.0; // camera z far
  float z = depth;
  return (2.0 * n) / (f + n - z * (f - n));	
}

void main() 
{
	vec3 components[3];
	components[0] = texture(samplerPosition, inUV.st).rgb;  
	components[1] = texture(samplerNormal, inUV.st).rgb;  
	//components[2] = texture(samplerDepth, inUV.st).rgb;  
	// Uncomment to display specular component
	//components[2] = vec3(texture(samplerAlbedo, inUV.st).a);  
	
	// Select component depending on z coordinate of quad
	highp int index = int(inUV.z);
	if (index == 2)
	{
		float depth = texture(samplerDepth, vec3(inUV.st, 2.0)).r;
		outFragColor = vec4(vec3(1.0-LinearizeDepth(depth)), 1.0);
	}
	else
	{
		outFragColor.rgb = components[index];
	}
}