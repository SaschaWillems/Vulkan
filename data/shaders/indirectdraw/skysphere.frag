#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 2) uniform sampler2D samplerColor;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	const vec4 gradientStart = vec4(0.93, 0.9, 0.81, 1.0);
	const vec4 gradientEnd = vec4(0.35, 0.5, 1.0, 1.0);
	outFragColor = mix(gradientStart, gradientEnd, min(0.5 - inUV.t, 0.5)/0.15 + 0.5);
}