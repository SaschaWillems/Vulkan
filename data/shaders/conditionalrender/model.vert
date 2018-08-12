#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inColor;

layout (set = 0, binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 model;
} ubo;

layout(push_constant) uniform PushBlock {
	vec4 offset;
	vec4 color;
} pushBlock;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec3 outViewVec;
layout (location = 3) out vec3 outLightVec;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main() 
{
	outNormal = inNormal;
	outColor = inColor * pushBlock.color.rgb;
	vec4 pos = vec4(inPos + pushBlock.offset.xyz, 1.0);
	gl_Position = ubo.projection * ubo.model * pos;

	outNormal = mat3(ubo.model) * inNormal;

	vec4 localpos = ubo.model * pos;
	vec3 lightPos = vec3(1.0f, -1.0f, 1.0f);
	outLightVec = lightPos.xyz - localpos.xyz;
	outViewVec = -localpos.xyz;		
}
