#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPos;

layout (binding = 0) uniform UBO 
{
	mat4 depthMVP;
} ubo;
 
void main()
{
	gl_Position =  ubo.depthMVP * vec4(inPos, 1.0);
}