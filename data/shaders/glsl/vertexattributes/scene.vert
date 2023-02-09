#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec4 inTangent;

layout (set = 0, binding = 0) uniform UBOScene 
{
	mat4 projection;
	mat4 view;
	vec4 lightPos;
	vec4 viewPos;
} uboScene;

layout(push_constant) uniform PushConsts {
	mat4 model;
	uint alphaMask;
	float alphaMaskCuttoff;
} pushConsts;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec2 outUV;
layout (location = 2) out vec3 outViewVec;
layout (location = 3) out vec3 outLightVec;
layout (location = 4) out vec4 outTangent;

void main() 
{
	outNormal = inNormal;
	outUV = inUV;
	outTangent = inTangent;
	gl_Position = uboScene.projection * uboScene.view * pushConsts.model * vec4(inPos.xyz, 1.0);
	
	outNormal = mat3(pushConsts.model) * inNormal;
	vec4 pos = pushConsts.model * vec4(inPos, 1.0);
	outLightVec = uboScene.lightPos.xyz - pos.xyz;
	outViewVec = uboScene.viewPos.xyz - pos.xyz;
}