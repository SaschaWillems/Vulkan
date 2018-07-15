#version 450

layout (location = 0) in vec3 inColor;

layout (location = 0) out vec4 outColor;
layout (location = 1) out float outDepth;

void main() 
{
	outColor = vec4(inColor, 0.0);
	outDepth = gl_FragDepth;
}