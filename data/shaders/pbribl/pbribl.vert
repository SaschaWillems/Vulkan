#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 model;
	mat4 view;
	vec3 camPos;
} ubo;

layout (location = 0) out vec3 outWorldPos;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec2 outUV;

layout(push_constant) uniform PushConsts {
	vec3 objPos;
} pushConsts;

out gl_PerVertex 
{
	vec4 gl_Position;
};

void main() 
{
	vec3 locPos = vec3(ubo.model * vec4(inPos, 1.0));
	outWorldPos = locPos + pushConsts.objPos;
	outNormal = mat3(ubo.model) * inNormal;
	outUV = inUV;
	outUV.t = 1.0 - inUV.t;
	gl_Position =  ubo.projection * ubo.view * vec4(outWorldPos, 1.0);
}
