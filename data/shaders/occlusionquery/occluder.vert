#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inColor;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 modelview;
	vec4 lightPos;
} ubo;

layout (location = 0) out vec3 outColor;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main() 
{
	outColor = inColor;
	gl_Position = ubo.projection * ubo.modelview * vec4(inPos.xyz, 1.0);
}