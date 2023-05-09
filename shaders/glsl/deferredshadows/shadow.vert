#version 450

layout (location = 0) in vec4 inPos;

layout (location = 0) out int outInstanceIndex;

void main()
{
	outInstanceIndex = gl_InstanceIndex;
	gl_Position = inPos;
}