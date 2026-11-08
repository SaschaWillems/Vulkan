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
	mat4 modelMatrix[2];
};

layout (push_constant) uniform PushConstants
{
	MatrixReference sceneDataReference;
} pushConstants;

layout (location = 0) out vec3 outColor;

void main() 
{
	MatrixReference sceneData = pushConstants.sceneDataReference;
	outColor = inColor;
	gl_Position = sceneData.matrix * sceneData.modelMatrix[gl_InstanceIndex] * vec4(inPos.xyz, 1.0);
}