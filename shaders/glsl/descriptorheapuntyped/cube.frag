/* Copyright (c) 2026, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

#version 450

#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_descriptor_heap: require

layout(descriptor_heap) uniform texture2D textureImage[];
layout(descriptor_heap) uniform sampler textureSampler[];

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inUV;
layout (location = 3) flat in int inInstanceIndex;
layout (location = 4) flat in uint inSamplerIndex;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	outFragColor = texture(sampler2D(textureImage[inInstanceIndex], textureSampler[inSamplerIndex]), inUV) * vec4(inColor, 1.0);
}