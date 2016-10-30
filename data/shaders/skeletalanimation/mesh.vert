#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inColor;
layout (location = 4) in vec4 inBoneWeights;
layout (location = 5) in ivec4 inBoneIDs;

#define MAX_BONES 64

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 view;
	mat4 model;
	mat4 bones[MAX_BONES];	
	vec4 lightPos;
	vec4 viewPos;
} ubo;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec2 outUV;
layout (location = 3) out vec3 outViewVec;
layout (location = 4) out vec3 outLightVec;

out gl_PerVertex 
{
	vec4 gl_Position;   
};

void main() 
{
	mat4 boneTransform = ubo.bones[inBoneIDs[0]] * inBoneWeights[0];
	boneTransform     += ubo.bones[inBoneIDs[1]] * inBoneWeights[1];
	boneTransform     += ubo.bones[inBoneIDs[2]] * inBoneWeights[2];
	boneTransform     += ubo.bones[inBoneIDs[3]] * inBoneWeights[3];	

	outColor = inColor;
	outUV = inUV;

	gl_Position = ubo.projection * ubo.view * ubo.model * boneTransform * vec4(inPos.xyz, 1.0);

	vec4 pos = ubo.model * vec4(inPos, 1.0);
	outNormal = mat3(inverse(transpose(ubo.model * boneTransform))) * inNormal;
	outLightVec = ubo.lightPos.xyz - pos.xyz;
	outViewVec = ubo.viewPos.xyz - pos.xyz;		
}