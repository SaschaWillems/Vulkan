#version 450

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inViewVec;
layout (location = 3) in vec3 inLightVec;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	vec3 N = normalize(inNormal);
	vec3 L = normalize(inLightVec);
	vec3 V = normalize(inViewVec);
	vec3 R = reflect(-L, N);
	float diffuse = max(dot(N, L), 0.0);
	float specular = pow(max(dot(R, V), 0.0), 1.0);
	outFragColor = vec4(vec3(diffuse + specular) * vec3(0.25), 1.0);	
}