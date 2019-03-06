#version 450

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec3 inViewVec;
layout (location = 3) in vec3 inLightVec;

layout (location = 0) out vec4 outColor;

void main() 
{
	vec3 N = normalize(inNormal);
	vec3 L = normalize(inLightVec);
	vec3 V = normalize(inViewVec);
	vec3 R = reflect(-L, N);
	vec3 ambient = vec3(0.1);
	vec3 diffuse = max(dot(N, L), 0.0) * vec3(1.0);
	vec3 specular = pow(max(dot(R, V), 0.0), 16.0) * vec3(0.75);
	outColor = vec4((ambient + diffuse) * inColor.rgb + specular, 1.0);		
}