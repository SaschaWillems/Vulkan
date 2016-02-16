#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inColor;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 model;
	vec4 lightPos;
} ubo;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec2 outUV;
layout (location = 3) out vec3 outEyePos;
layout (location = 4) out vec3 outLightVec;

void main() 
{
	outNormal = normalize(inNormal);
	outColor = inColor;
	vec4 pos = ubo.model * vec4(inPos.xyz, 1.0);
	gl_Position = ubo.projection * ubo.model * vec4(inPos.xyz, 1.0);

	vec4 lPos = ubo.model * ubo.lightPos;
	
	outLightVec = vec3(lPos.xyz - pos.xyz);
	outEyePos = vec3(-pos);
}