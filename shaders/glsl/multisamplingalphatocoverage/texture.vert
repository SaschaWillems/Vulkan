/* Copyright (c) 2026, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;

layout (set = 0, binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 model;
	vec4 lightPos;
} ubo;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec2 outUV;
layout (location = 2) out vec3 outViewVec;
layout (location = 3) out vec3 outLightVec;

layout(push_constant) uniform PushConsts {
	vec3 position;
} pushConsts;

void main() 
{
	vec4 locPos = vec4(inPos + pushConsts.position, 1.0);
	outNormal = inNormal;
	outUV = inUV;
	gl_Position = ubo.projection * ubo.model * locPos;
	
	vec4 pos = ubo.model * locPos;
	outNormal = mat3(ubo.model) * inNormal;
	vec3 lPos = mat3(ubo.model) * ubo.lightPos.xyz;
	outLightVec = lPos - pos.xyz;
	outViewVec = -pos.xyz;		
}