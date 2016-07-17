#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUV;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 modelview;
} ubo;

layout (location = 0) out vec3 outUV;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main() 
{
	outUV = vec3(inUV.st, gl_InstanceIndex);
	vec4 tmpPos = vec4(inPos, 1.0);
	tmpPos.y += gl_InstanceIndex;
	tmpPos.xy *= vec2(1.0/4.0, 1.0/3.0);
	gl_Position = ubo.projection * ubo.modelview * tmpPos;
}
