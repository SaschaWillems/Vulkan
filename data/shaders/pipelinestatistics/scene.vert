#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inColor;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 modelview;
	vec4 lightPos;
} ubo;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec3 outViewVec;
layout (location = 3) out vec3 outLightVec;

layout(push_constant) uniform PushConsts {
	vec3 objPos;
} pushConsts;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main() 
{
	outNormal = inNormal;
	outColor = inColor;

	vec3 locPos = vec3(ubo.modelview * vec4(inPos, 1.0));
	vec3 worldPos = vec3(ubo.modelview * vec4(inPos + pushConsts.objPos, 1.0));
	gl_Position =  ubo.projection /* ubo.modelview */ * vec4(worldPos, 1.0);
	
    vec4 pos = ubo.modelview * vec4(worldPos, 1.0);
    outNormal = mat3(ubo.modelview) * inNormal;
    outLightVec = ubo.lightPos.xyz - pos.xyz;
    outViewVec = -pos.xyz;
}