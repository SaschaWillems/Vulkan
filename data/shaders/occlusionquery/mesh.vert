#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inColor;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 modelview;
	vec4 lightPos;
	float visible;
} ubo;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out float outVisible;
layout (location = 3) out vec3 outViewVec;
layout (location = 4) out vec3 outLightVec;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main() 
{
	outNormal = inNormal;
	outColor = inColor;
	outVisible = ubo.visible;
	
	gl_Position = ubo.projection * ubo.modelview * vec4(inPos.xyz, 1.0);
	
    vec4 pos = ubo.modelview * vec4(inPos, 1.0);
    outNormal = mat3(ubo.modelview) * inNormal;
    outLightVec = ubo.lightPos.xyz - pos.xyz;
    outViewVec = -pos.xyz;
}