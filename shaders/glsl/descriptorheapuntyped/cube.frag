/* Copyright (c) 2026, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

#version 450

#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_descriptor_heap: require

layout(descriptor_heap) uniform texture2D textureImage[];
layout(descriptor_heap) uniform sampler textureSampler[];

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inUV;
layout (location = 3) flat in int inInstanceIndex;

layout (location = 0) out vec4 outFragColor;

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


void main() 
{
	MatrixReference uniformData = pushConstants.matrixReference;
	outFragColor = texture(sampler2D(textureImage[uniformData.imageHeapIndexOffset + inInstanceIndex], textureSampler[uniformData.samplerIndex]), inUV) * vec4(inColor, 1.0);
}