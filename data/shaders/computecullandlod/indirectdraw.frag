#version 450

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec3 inViewVec;
layout (location = 3) in vec3 inLightVec;

layout (location = 0) out vec4 outFragColor;

void main()
{
	vec3 N = normalize(inNormal);
	vec3 L = normalize(inLightVec);
	vec3 ambient = vec3(0.25);
	vec3 diffuse = vec3(max(dot(N, L), 0.0));
	outFragColor = vec4((ambient + diffuse) * inColor, 1.0);
}
