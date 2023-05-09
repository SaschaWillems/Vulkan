#version 450

layout (location = 0) in vec3 inPos;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 view;
	mat4 model;
} ubo;

layout (location = 0) out vec4 outPos;

void main() 
{
	outPos = ubo.projection * ubo.view * ubo.model * vec4(inPos.xyz, 1.0);
	gl_Position = outPos;		
}
