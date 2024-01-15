#version 450

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 modelview;
	vec4 lightPos;
	float tessAlpha;
	float tessStrength;
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
