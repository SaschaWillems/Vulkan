#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec4 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 3) in vec3 inColor;

struct Instance
{
	mat4 model;
	vec4 color;
};

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 view;
	Instance instance[343];
} ubo;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec3 outEyePos;
layout (location = 3) out vec3 outLightVec;

void main() 
{
	outNormal = inNormal;
	outColor = ubo.instance[gl_InstanceIndex].color.rgb;
	mat4 modelView = ubo.view * ubo.instance[gl_InstanceIndex].model;
	gl_Position = ubo.projection * modelView * inPos;
	outEyePos = vec3(modelView * inPos);
	vec4 lightPos = vec4(0.0, 0.0, 0.0, 1.0) * modelView;
	outLightVec = normalize(lightPos.xyz - outEyePos);
}
