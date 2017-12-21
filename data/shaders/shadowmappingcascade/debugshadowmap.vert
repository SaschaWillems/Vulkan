#version 450

layout(push_constant) uniform PushConsts {
	vec4 position;
	uint cascadeIndex;
} pushConsts;

layout (location = 0) out vec2 outUV;
layout (location = 1) out uint outCascadeIndex;

out gl_PerVertex {
	vec4 gl_Position;   
};

void main() 
{
	outUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	outCascadeIndex = pushConsts.cascadeIndex;
	gl_Position = vec4(outUV * 2.0f - 1.0f, 0.0f, 1.0f);
}
