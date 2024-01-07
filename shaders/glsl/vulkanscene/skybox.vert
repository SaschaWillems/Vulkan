#version 450

layout (location = 0) in vec3 inPos;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 model;
	mat4 normal;
	mat4 view;
} ubo;

layout (location = 0) out vec3 outUVW;

void main() 
{
	outUVW = inPos;
	// Remove translation from view matrix
	mat4 viewMat = mat4(mat3(ubo.view));
	gl_Position = ubo.projection * viewMat * ubo.model * vec4(inPos.xyz, 1.0);
}
