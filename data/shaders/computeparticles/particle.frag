#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 0) uniform sampler2D samplerColorMap;
layout (binding = 1) uniform sampler2D samplerGradientRamp;

layout (location = 0) in vec4 inColor;
layout (location = 1) in float inGradientPos;

layout (location = 0) out vec4 outFragColor;

void main () 
{
	vec3 color = texture(samplerGradientRamp, vec2(inGradientPos, 0.0)).rgb;
	outFragColor.rgb = texture(samplerColorMap, gl_PointCoord).rgb * color;
}
