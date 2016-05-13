#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) out vec2 outUV;

out gl_PerVertex 
{
    vec4 gl_Position;   
};

void main() 
{
	outUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	gl_Position = vec4(outUV * vec2(2.0f, 2.0f) + vec2(-1.0f, -1.0f), 0.0f, 1.0f);
}