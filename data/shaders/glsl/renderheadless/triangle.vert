#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inColor;

layout (location = 0) out vec3 outColor;

out gl_PerVertex {
	vec4 gl_Position;   
};

layout(push_constant) uniform PushConsts {
	mat4 mvp;
} pushConsts;

void main() 
{
	outColor = inColor;
	gl_Position = pushConsts.mvp * vec4(inPos.xyz, 1.0);
}
