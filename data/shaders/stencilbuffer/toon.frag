#version 450

layout (binding = 1) uniform sampler2D samplerColorMap;

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec3 inLightVec;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	vec3 color;
	vec3 N = normalize(inNormal);
	vec3 L = normalize(inLightVec);
	float intensity = dot(N,L);
	if (intensity > 0.98)
		color = inColor * 1.5;
	else if  (intensity > 0.9)
		color = inColor * 1.0;
	else if (intensity > 0.5)
		color = inColor * 0.6;
	else if (intensity > 0.25)
		color = inColor * 0.4;
	else
		color = inColor * 0.2;
	// Desaturate a bit
	color = vec3(mix(color, vec3(dot(vec3(0.2126,0.7152,0.0722), color)), 0.1));	
	outFragColor.rgb = color;
}