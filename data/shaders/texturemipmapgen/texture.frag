#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (set = 0, binding = 1) uniform texture2D textureColor;
layout (set = 0, binding = 2) uniform sampler samplers[3];

layout (location = 0) in vec2 inUV;
layout (location = 1) in float inLodBias;
layout (location = 2) in float inSamplerIndex;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	highp int samplerIndex = int(inSamplerIndex);
	outFragColor = texture(sampler2D(textureColor, samplers[samplerIndex]), inUV, inLodBias);
}