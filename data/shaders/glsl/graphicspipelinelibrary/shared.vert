#version 450 

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
layout (location = 2) out vec3 outViewVec;
layout (location = 3) out vec3 outLightVec;
layout (location = 4) flat out vec3 outFlatNormal;

void main() 
{
	outNormal = inNormal;
	outColor = inColor;
	vec4 pos = vec4(inPos.xyz, 1.0);

	gl_Position = ubo.projection * ubo.model * pos;
	
	pos = ubo.model * pos;
	outNormal = mat3(ubo.model) * inNormal;
	vec3 lPos = ubo.lightPos.xyz;
	outLightVec = lPos - pos.xyz;
	outViewVec = -pos.xyz;		
}