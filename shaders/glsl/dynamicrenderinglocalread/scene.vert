/* Copyright (c) 2026, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec2 inUV;

layout (set = 0, binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 model;
	mat4 view;
} ubo;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec4 outColor;
layout (location = 2) out vec3 outWorldPos;
layout (location = 3) out vec2 outUV;

void main() 
{
	gl_Position = ubo.projection * ubo.view * ubo.model * vec4(inPos, 1.0);
	
	// Vertex position in world space
	outWorldPos = vec3(ubo.model * vec4(inPos, 1.0));
	// GL to Vulkan coord space
	outWorldPos.y = -outWorldPos.y;
	
	// Normal in world space
	mat3 mNormal = transpose(inverse(mat3(mat3(ubo.model))));
	outNormal = normalize(mNormal * inNormal);
	
	outColor = vec4(inColor, 1.0);
	outUV = inUV;
}
