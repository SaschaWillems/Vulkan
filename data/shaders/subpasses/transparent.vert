#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec4 inPos;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec2 inUV;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 model;
	mat4 view;
} ubo;

layout (location = 0) out vec3 outColor;
layout (location = 1) out vec2 outUV;

void main () 
{
	outColor = inColor;
	outUV = inUV;
	
	gl_Position = ubo.projection * ubo.view * ubo.model * vec4(inPos.xyz, 1.0);		
}