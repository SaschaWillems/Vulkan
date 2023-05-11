#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec4 inTangent;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 model;
	mat4 normal;
	vec4 lightPos;
} ubo;

layout (location = 0) out vec2 outUV;
layout (location = 1) out vec3 outLightVec;
layout (location = 2) out vec3 outLightVecB;
layout (location = 3) out vec3 outLightDir;
layout (location = 4) out vec3 outViewVec;

void main(void) 
{
	vec3 vertexPosition = vec3(ubo.model *  vec4(inPos, 1.0));
	outLightDir = normalize(ubo.lightPos.xyz - vertexPosition);

	vec3 biTangent = cross(inNormal, inTangent.xyz);

	// Setup (t)angent-(b)inormal-(n)ormal matrix for converting
	// object coordinates into tangent space
	mat3 tbnMatrix;
	tbnMatrix[0] =  mat3(ubo.normal) * inTangent.xyz;
	tbnMatrix[1] =  mat3(ubo.normal) * biTangent;
	tbnMatrix[2] =  mat3(ubo.normal) * inNormal;

	outLightVec.xyz = vec3(ubo.lightPos.xyz - vertexPosition) * tbnMatrix;

	vec3 lightDist = ubo.lightPos.xyz - inPos;
	outLightVecB.x = dot(inTangent.xyz, lightDist);
	outLightVecB.y = dot(biTangent, lightDist);
	outLightVecB.z = dot(inNormal, lightDist);

	outViewVec.x = dot(inTangent.xyz, inPos);
	outViewVec.y = dot(biTangent, inPos);
	outViewVec.z = dot(inNormal, inPos);

	outUV = inUV;
	
	gl_Position = ubo.projection * ubo.model * vec4(inPos, 1.0);
}