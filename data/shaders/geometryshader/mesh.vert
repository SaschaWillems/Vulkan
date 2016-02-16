#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec4 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTexCoord;
layout (location = 3) in vec3 inColor;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 model;
} ubo;

layout (location = 0) out vec2 vTexcoord;
layout (location = 1) out vec3 vNormal;
layout (location = 2) out vec3 vColor;
layout (location = 3) out vec3 vEyePos;
layout (location = 4) out vec3 vLightVec;

void main() 
{
	vTexcoord = inTexCoord.st;
	vNormal = inNormal;
	vColor = inColor;
	gl_Position = ubo.projection * ubo.model * inPos;
	vEyePos = vec3(ubo.model * inPos);
	vec4 lightPos = vec4(0.0, 0.0, 0.0, 1.0) * ubo.model;
	vLightVec = normalize(lightPos.xyz - vEyePos);
}
