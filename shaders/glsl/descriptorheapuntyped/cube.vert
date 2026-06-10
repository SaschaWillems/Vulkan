/* Copyright (c) 2026, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

#version 450

#extension GL_EXT_descriptor_heap: require
#extension GL_EXT_buffer_reference : require

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inColor;

// Untyped pointers don't support structured buffers (yet), so we use BDA for that insteadf
layout (buffer_reference) readonly buffer MatrixReference {
	mat4 mvp;
	vec4 color[2];
	vec4 pos[2];
	uint samplerIndex;
};

layout (push_constant) uniform PushConstants
{
	MatrixReference matrixReference;
} pushConstants;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec2 outUV;
layout (location = 3) flat out int outInstanceIndex;
layout (location = 4) flat out uint outSamplerIndex;

void main() 
{
	outNormal = inNormal;
	outColor = inColor;
	outUV = inUV;
	MatrixReference uniformData = pushConstants.matrixReference;
	
	vec3 localPos = inPos * 0.25f + uniformData.pos[gl_InstanceIndex].xyz;

	gl_Position = uniformData.mvp * vec4(localPos, 1.0);
	outColor = uniformData.color[gl_InstanceIndex].rgb;

	outSamplerIndex = uniformData.samplerIndex;
	outInstanceIndex = gl_InstanceIndex;
}