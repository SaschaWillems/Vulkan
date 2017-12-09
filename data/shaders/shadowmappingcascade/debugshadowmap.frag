#version 450

layout (binding = 1) uniform sampler2DArray shadowMap;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;

layout(push_constant) uniform PushConsts {
	uint cascadeIndex;
} pushConsts;

void main() 
{
	float depth = texture(shadowMap, vec3(inUV, float(pushConsts.cascadeIndex))).r;
	outFragColor = vec4(vec3((depth)), 1.0);
}