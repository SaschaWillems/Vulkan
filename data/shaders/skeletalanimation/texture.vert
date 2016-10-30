#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 model;
	mat4 view;
	vec4 lightPos;	
	vec4 viewPos;
	vec2 uvOffset;
} ubo;

layout (location = 0) out vec2 outUV;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec3 outViewVec;
layout (location = 3) out vec3 outLightVec;

out gl_PerVertex 
{
	vec4 gl_Position;   
};

void main() 
{
	outUV = inUV + ubo.uvOffset;
	vec4 pos = vec4(inPos, 1.0);
	gl_Position = ubo.projection * ubo.view * ubo.model * vec4(pos);
	
	outNormal = mat3(ubo.model) * inNormal;
	outLightVec = ubo.lightPos.xyz - pos.xyz;
	outViewVec = ubo.viewPos.xyz - pos.xyz;			
}
