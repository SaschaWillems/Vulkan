#version 450

layout (location = 0) in vec3 inPos;
layout (location = 2) in vec3 inNormal;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outViewVec;
layout (location = 2) out vec3 outLightVec;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 modelview;
	vec4 lightPos;
} ubo;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main () 
{
	vec4 eyePos = ubo.modelview * vec4(inPos.x, inPos.y, inPos.z, 1.0); 
	gl_Position = ubo.projection * eyePos;
	vec4 pos = vec4(inPos, 1.0);
	vec3 lPos = ubo.lightPos.xyz;
	outLightVec = lPos - pos.xyz;
	outViewVec = -pos.xyz;
	outNormal = inNormal;
}