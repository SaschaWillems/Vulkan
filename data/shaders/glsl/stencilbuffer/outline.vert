#version 450

layout (location = 0) in vec4 inPos;
layout (location = 2) in vec3 inNormal;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 model;
	vec4 lightPos;
	float outlineWidth;
} ubo;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main() 
{
	// Extrude along normal
	vec4 pos = vec4(inPos.xyz + inNormal * ubo.outlineWidth, inPos.w);
	gl_Position = ubo.projection * ubo.model * pos;
}
