#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 model;
	vec4 viewPos;
	float lodBias;
} ubo;

layout (location = 0) out vec2 outUV;
layout (location = 1) out float outLodBias;
layout (location = 2) out vec3 outNormal;
layout (location = 3) out vec3 outViewVec;
layout (location = 4) out vec3 outLightVec;

out gl_PerVertex 
{
	vec4 gl_Position;   
};

void main() 
{
	outUV = inUV;
	outLodBias = ubo.lodBias;
	outNormal = inNormal;

	vec3 worldPos = vec3(ubo.model * vec4(inPos, 1.0));

	gl_Position = ubo.projection * ubo.model * vec4(inPos.xyz, 1.0);

	vec3 lightPos = vec3(0.0, 50.0f, 0.0f);
	outLightVec = lightPos - inPos.xyz;
	outViewVec = ubo.viewPos.xyz - worldPos.xyz;		
}
