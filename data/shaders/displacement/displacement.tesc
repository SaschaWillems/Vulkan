#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 0) uniform UBO 
{
	float tessLevel;
} ubo; 
 
layout (vertices = 3) out;
 
layout (location = 0) in vec3 inNormal[];
layout (location = 1) in vec2 inUV[];
 
layout (location = 0) out vec3 outNormal[3];
layout (location = 1) out vec2 outUV[3];
 
void main()
{
	if (gl_InvocationID == 0)
	{
		gl_TessLevelInner[0] = ubo.tessLevel;
		gl_TessLevelOuter[0] = ubo.tessLevel;
		gl_TessLevelOuter[1] = ubo.tessLevel;
		gl_TessLevelOuter[2] = ubo.tessLevel;		
	}

	gl_out[gl_InvocationID].gl_Position =  gl_in[gl_InvocationID].gl_Position;
	outNormal[gl_InvocationID] = inNormal[gl_InvocationID];
	outUV[gl_InvocationID] = inUV[gl_InvocationID];
} 
