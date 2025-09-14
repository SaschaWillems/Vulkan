/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

#version 450

#extension GL_EXT_fragment_shader_barycentric : require

layout (location = 0) in vec3 inColor;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	// Use barycentric coordinates to emulate a wireframe overlay by drawing thick black borders at the triangle edges
	if (gl_BaryCoordEXT.x < 0.02 || gl_BaryCoordEXT.y < 0.02 || gl_BaryCoordEXT.z < 0.02) {
		outFragColor = vec4(inColor, 1.0) * 2.0;
	} else {
		outFragColor = vec4(inColor, 1.0) * 0.5;
	}
}