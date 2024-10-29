#version 450

layout (location = 0) in vec3 inPos;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 modelview;
	vec4 lightPos;
} ubo;

layout(push_constant) uniform PushConsts {
	vec3 objPos;
} pushConsts;

void main() 
{
	vec3 locPos = vec3(ubo.modelview * vec4(inPos, 1.0));
	vec3 worldPos = vec3(ubo.modelview * vec4(inPos + pushConsts.objPos, 1.0));
	gl_Position =  ubo.projection /* ubo.modelview */ * vec4(worldPos, 1.0);
}