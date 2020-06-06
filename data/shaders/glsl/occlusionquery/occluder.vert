#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inColor;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 view;
	mat4 model;
	vec4 color;
	vec4 lightPos;
} ubo;

layout (location = 0) out vec3 outColor;

void main() 
{
	outColor = inColor * ubo.color.rgb;
	gl_Position = ubo.projection * ubo.view * ubo.model * vec4(inPos.xyz, 1.0);
}