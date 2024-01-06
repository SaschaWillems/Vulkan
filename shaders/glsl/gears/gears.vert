#version 450

layout (location = 0) in vec4 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inColor;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 view;
	vec4 lightpos;
	mat4 model[3];
} ubo;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec3 outEyePos;
layout (location = 3) out vec3 outLightVec;

void main() 
{
	outNormal = normalize(mat3x3(ubo.model[gl_InstanceIndex]) * inNormal);
	outColor = inColor;
	mat4 modelView = ubo.view * ubo.model[gl_InstanceIndex];
	vec4 pos = modelView * inPos;	
	outEyePos = vec3(modelView * pos);
	vec4 lightPos = vec4(ubo.lightpos.xyz, 1.0) * modelView;
	outLightVec = normalize(lightPos.xyz - outEyePos);
	gl_Position = ubo.projection * pos;
}