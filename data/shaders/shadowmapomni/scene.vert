#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPos;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec3 inNormal;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 view;
	mat4 model;
	vec4 lightPos;
} ubo;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec3 outEyePos;
layout (location = 3) out vec3 outLightVec;
layout (location = 4) out vec3 outWorldPos;
layout (location = 5) out vec3 outLightPos;

out gl_PerVertex 
{
	vec4 gl_Position;
};

void main() 
{
	outColor = inColor;
	outNormal = inNormal;
	
	gl_Position = ubo.projection * ubo.view * ubo.model * vec4(inPos.xyz, 1.0);
	outEyePos = vec3(ubo.model * vec4(inPos, 1.0f));
	outLightVec = normalize(ubo.lightPos.xyz - inPos.xyz);	
	outWorldPos = inPos;
	
	outLightPos = ubo.lightPos.xyz;
}

