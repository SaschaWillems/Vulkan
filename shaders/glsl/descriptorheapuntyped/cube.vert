/* Copyright (c) 2026, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

#version 450

#extension GL_EXT_descriptor_heap: require
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_nonuniform_qualifier : require

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inColor;

// Per-Model data via heaps
layout(descriptor_heap) buffer ModelData {
	vec4 pos;
	vec4 color;
} modelData[];

// Global data via BDA
layout (buffer_reference) readonly buffer MatrixReference {
	mat4 mvp;
	uint samplerIndex;
	uint imageHeapIndexOffset;
};

layout (push_constant) uniform PushConstants
{
	MatrixReference matrixReference;
} pushConstants;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec2 outUV;
layout (location = 3) flat out int outInstanceIndex;

void main() 
{
	outNormal = inNormal;
	outColor = inColor;
	outUV = inUV;
	MatrixReference uniformData = pushConstants.matrixReference;
	
	vec3 localPos = inPos * 0.25f + modelData[nonuniformEXT(gl_InstanceIndex)].pos.xyz;

	gl_Position = uniformData.mvp * vec4(localPos, 1.0);
	outColor = modelData[nonuniformEXT(gl_InstanceIndex)].color.rgb;

	outInstanceIndex = gl_InstanceIndex;
}