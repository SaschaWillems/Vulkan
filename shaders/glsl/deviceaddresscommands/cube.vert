/* Copyright (c) 2026, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

#version 450

#extension GL_EXT_scalar_block_layout: require
#extension GL_EXT_buffer_reference : require

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inColor;

layout (buffer_reference, scalar) readonly buffer MatrixReference {
	mat4 matrix;
};

layout (push_constant) uniform PushConstants
{
	// Pointer to the buffer with the scene's MVP matrix
	MatrixReference sceneDataReference;
	// Pointer to the buffer for the data for each model
	MatrixReference modelDataReference;
} pushConstants;

layout (location = 0) out vec3 outColor;

void main() 
{
	MatrixReference sceneData = pushConstants.sceneDataReference;
	MatrixReference modelData = pushConstants.modelDataReference;

	outColor = inColor;
	gl_Position = sceneData.matrix * modelData.matrix * vec4(inPos.xyz, 1.0);
}