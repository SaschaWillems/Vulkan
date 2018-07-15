#version 450

layout (location = 0) in vec3 inColor;

layout (location = 0) out vec4 outSwapChainColor;
layout (location = 1) out vec4 outColor;
layout (location = 2) out float outDepth;

void main() 
{
	outSwapChainColor = vec4(0.0);
	outColor = vec4(inColor, 0.0);
	outDepth = gl_FragDepth;
}