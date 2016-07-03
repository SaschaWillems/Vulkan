#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 1) uniform sampler2D samplerPosition;
layout (binding = 2) uniform sampler2D samplerNormal;
layout (binding = 3) uniform sampler2D samplerAlbedo;

layout (location = 0) in vec3 inUV;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	vec3 components[3];
	components[0] = texture(samplerPosition, inUV.st).rgb;  
	components[1] = texture(samplerNormal, inUV.st).rgb;  
	components[2] = texture(samplerAlbedo, inUV.st).rgb;  
	// Uncomment to display specular component
	//components[2] = vec3(texture(samplerAlbedo, inUV.st).a);  
	
	// Select component depending on z coordinate of quad
	highp int index = int(inUV.z);
	outFragColor.rgb = components[index];
}