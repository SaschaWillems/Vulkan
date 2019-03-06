#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;

layout (location = 0) out vec3 outNormal;

void main(void)
{
	outNormal = inNormal;
	gl_Position = vec4(inPos.xyz, 1.0);
}