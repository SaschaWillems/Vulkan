#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 1) uniform sampler2D samplerColorMap;

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inViewVec;
layout (location = 3) in vec3 inLightVec;
layout (location = 4) in vec3 inEyeNormal;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	vec4 color = texture(samplerColorMap, inUV);

	vec3 N = normalize(inNormal);
	vec3 L = normalize(inLightVec);
	vec3 V = normalize(inViewVec);
	vec3 R = reflect(-L, N);
	vec3 diffuse = max(dot(N, L), 0.0) * color.rgb;
	vec3 specular = pow(max(dot(R, V), 0.0), 1.0) * vec3(color.a);
	outFragColor = vec4(diffuse + specular, 1.0);	
}