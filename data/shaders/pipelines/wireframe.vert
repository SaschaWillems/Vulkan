#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec4 inPos;
layout (location = 3) in vec3 inColor;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 model;
} ubo;

layout (location = 0) out vec3 outColor;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main() 
{
	{
		outColor = inColor;
	}
	gl_Position = ubo.projection * ubo.model * inPos;
}
