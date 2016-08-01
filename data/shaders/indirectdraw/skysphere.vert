#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// Vertex attributes
layout (location = 0) in vec4 inPos;
layout (location = 2) in vec2 inUV;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 modelview;
} ubo;

layout (location = 0) out vec2 outUV;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main() 
{
	outUV = vec2(inUV.s, 1.0-inUV.t);
	// Skysphere always at center, only use rotation part of modelview matrix
	gl_Position = ubo.projection * mat4(mat3(ubo.modelview)) * vec4(inPos.xyz, 1.0);
}
