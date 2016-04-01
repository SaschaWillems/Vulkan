#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inColor;

layout (std140, push_constant) uniform PushConsts 
{
	mat4 mvp;
	vec3 color;
} pushConsts;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 3) out vec3 outViewVec;
layout (location = 4) out vec3 outLightVec;

void main() 
{
	outNormal = inNormal;

	if ( (inColor.r == 1.0) && (inColor.g == 0.0) && (inColor.b == 0.0))
	{	
		outColor = pushConsts.color;
	}
	else
	{
		outColor = inColor;
	}
	
	gl_Position = pushConsts.mvp * vec4(inPos.xyz, 1.0);
	
    vec4 pos = pushConsts.mvp * vec4(inPos, 1.0);
    outNormal = mat3(pushConsts.mvp) * inNormal;
//	vec3 lPos = ubo.lightPos.xyz;
vec3 lPos = vec3(0.0);
    outLightVec = lPos - pos.xyz;
    outViewVec = -pos.xyz;
}