#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec4 inVel;

layout (location = 0) out float outGradientPos;

layout (binding = 2) uniform UBO 
{
	mat4 projection;
	mat4 modelview;
} ubo;

out gl_PerVertex
{
	vec4 gl_Position;
	float gl_PointSize;
};

void main () 
{
	gl_PointSize = 8.0;	
	outGradientPos = inVel.w;
	gl_Position = ubo.projection * ubo.modelview * vec4(inPos, 1.0);
}