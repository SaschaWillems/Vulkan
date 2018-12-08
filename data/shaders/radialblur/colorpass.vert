#version 450

layout (location = 0) in vec3 inPos;
layout (location = 2) in vec3 inColor;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 model;
	float gradientPos;
} ubo;

layout (location = 0) out vec3 outColor;
layout (location = 1) out vec2 outUV;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main() 
{
	outColor = inColor;
	outUV = vec2(ubo.gradientPos, 0.0f);
	gl_Position = ubo.projection * ubo.model * vec4(inPos, 1.0);
}
