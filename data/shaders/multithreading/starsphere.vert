#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPos;

layout (std140, push_constant) uniform PushConsts 
{
	mat4 mvp;
} pushConsts;

layout (location = 0) out vec3 outUVW;

void main() 
{
	outUVW = inPos;
	gl_Position = pushConsts.mvp * vec4(inPos.xyz, 1.0);
}
