#version 420

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#define LIGHT_COUNT 3

layout (triangles, invocations = LIGHT_COUNT) in;
layout (triangle_strip, max_vertices = 3) out;

layout (binding = 0) uniform UBO 
{
	mat4 mvp[LIGHT_COUNT];
	vec4 instancePos[3];
} ubo;

layout (location = 0) in int inInstanceIndex[];

out gl_PerVertex
{
	vec4 gl_Position;
};

void main() 
{
	vec4 instancedPos = ubo.instancePos[inInstanceIndex[0]]; 
	for (int i = 0; i < gl_in.length(); i++)
	{
		gl_Layer = gl_InvocationID;
		vec4 tmpPos = gl_in[i].gl_Position + instancedPos;
		gl_Position = ubo.mvp[gl_InvocationID] * tmpPos;
		EmitVertex();
	}
	EndPrimitive();
}