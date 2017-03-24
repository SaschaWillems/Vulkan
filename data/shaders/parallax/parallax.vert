#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inTangent;
layout (location = 4) in vec3 inBiTangent;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 view;
	mat4 model;
	vec4 lightPos;
	vec4 cameraPos;
} ubo;

layout (location = 0) out vec2 outUV;
layout (location = 1) out vec3 outTangentLightPos;
layout (location = 2) out vec3 outTangentViewPos;
layout (location = 3) out vec3 outTangentFragPos;

void main(void) 
{
	gl_Position = ubo.projection * ubo.view * ubo.model * vec4(inPos, 1.0f);
	outTangentFragPos = vec3(ubo.model * vec4(inPos, 1.0));   
	outUV = inUV;
		
	vec3 T = normalize(mat3(ubo.model) * inTangent);
	vec3 B = normalize(mat3(ubo.model) * inBiTangent);
	vec3 N = normalize(mat3(ubo.model) * inNormal);
	mat3 TBN = transpose(mat3(T, B, N));

	outTangentLightPos = TBN * ubo.lightPos.xyz;
	outTangentViewPos  = TBN * ubo.cameraPos.xyz;
	outTangentFragPos  = TBN * outTangentFragPos;
}
