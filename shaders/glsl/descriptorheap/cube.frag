/* Copyright (c) 2026, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

#version 450

layout (set = 1, binding = 0) uniform texture2D textureImage[2];
layout (set = 2, binding = 0) uniform sampler textureSampler[2];

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inUV;
layout (location = 3) flat in int inInstanceIndex;

layout (set = 0, binding = 0) uniform UBO {
	mat4 projection;
	mat4 view;
	mat4 model[2];
	int selectedSampler;
} ubo;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	outFragColor = texture(sampler2D(textureImage[inInstanceIndex], textureSampler[ubo.selectedSampler]), inUV) * vec4(inColor, 1.0);
}