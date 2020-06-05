#version 450 core

layout (location = 0) in vec2 inPos;
layout (location = 1) in vec2 inUV;

layout (location = 0) out vec2 outUV;

out gl_PerVertex 
{
    vec4 gl_Position;   
};

void main(void)
{
	gl_Position = vec4(inPos, 0.0, 1.0);
	outUV = inUV;
}
