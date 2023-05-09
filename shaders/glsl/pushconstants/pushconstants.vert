#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inColor;

#define lightCount 6

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 model;
	mat4 view;
} ubo;

layout(push_constant) uniform PushConsts {
	vec4 color;
	vec4 position;
} pushConsts;

layout (location = 0) out vec3 outColor;

void main() 
{
	outColor = inColor * pushConsts.color.rgb;	
	vec3 locPos = vec3(ubo.model * vec4(inPos, 1.0));
	vec3 worldPos = locPos + pushConsts.position.xyz;
	gl_Position =  ubo.projection * ubo.view * vec4(worldPos, 1.0);
}