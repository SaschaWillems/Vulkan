#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 1) uniform sampler2D samplerColorMap;

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inColor;
layout (location = 3) in vec3 inEyePos;
layout (location = 4) in vec3 inLightVec;

layout (location = 0) out vec4 outFragColor;

void main() 
{	
	vec4 spec = vec4(0.0);
	float shininess = 32.0;

	vec3 n = normalize(inNormal);
	vec3 l = normalize(inLightVec);
	vec3 e = normalize(inEyePos);
	
	vec4 diffuse = vec4(inColor, 1.0);
	vec4 specular = vec4(1.0);
	vec4 ambient = vec4(vec3(0.05), 0.0);

	float intensity = max(dot(n,l), 0.0);
	
	if (intensity > 0.0) 
	{
		vec3 h = normalize(l + e);
		float intSpec = max(dot(h,n), 0.0);
		spec = specular * pow(intSpec, shininess);
	}

	outFragColor = max(intensity * diffuse + spec, ambient);	
}