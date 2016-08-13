#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec4 pos;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec3 inNormal;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 model;
	float gradientPos;
} ubo;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec3 outEyePos;
layout (location = 3) out vec3 outLightVec;
layout (location = 4) out vec2 outUV;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main() 
{
	outNormal = inNormal;
	outColor = inColor;
	outUV = vec2(ubo.gradientPos, 0.0); 
	gl_Position = ubo.projection * ubo.model * pos;
	outEyePos = vec3(ubo.model * pos);
	vec4 lightPos = vec4(0.0, 0.0, -5.0, 1.0);// * ubo.model;
	outLightVec = normalize(lightPos.xyz - pos.xyz);
}
