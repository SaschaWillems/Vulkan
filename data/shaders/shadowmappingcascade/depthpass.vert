#version 450

layout (location = 0) in vec3 inPos;

// todo: pass via specialization constant
#define SHADOW_MAP_CASCADE_COUNT 4

layout(push_constant) uniform PushConsts {
	uint cascadeIndex;
} pushConsts;

layout (binding = 0) uniform UBO {
	mat4[SHADOW_MAP_CASCADE_COUNT] cascadeViewProjMat;
} ubo;

out gl_PerVertex {
	vec4 gl_Position;   
};

void main()
{
	gl_Position =  ubo.cascadeViewProjMat[pushConsts.cascadeIndex] * vec4(inPos, 1.0);
}