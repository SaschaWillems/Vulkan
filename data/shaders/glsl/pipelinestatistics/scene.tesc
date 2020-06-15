#version 450

layout (vertices = 3) out;

layout (location = 0) in vec3 inNormal[];
layout (location = 1) in vec3 inColor[];
layout (location = 2) in vec3 inViewVec[];
layout (location = 3) in vec3 inLightVec[];

layout (location = 0) out vec3 outNormal[3];
layout (location = 1) out vec3 outColor[3];
layout (location = 2) out vec3 outViewVec[3];
layout (location = 3) out vec3 outLightVec[3];

void main(void)
{
    if (gl_InvocationID == 0)
    {
        gl_TessLevelInner[0] = 2.0;
        gl_TessLevelOuter[0] = 1.0;
        gl_TessLevelOuter[1] = 1.0;
        gl_TessLevelOuter[2] = 1.0;
    }

    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
	outNormal[gl_InvocationID] = inNormal[gl_InvocationID];
	outColor[gl_InvocationID] = inColor[gl_InvocationID];	
	outViewVec[gl_InvocationID] = inViewVec[gl_InvocationID];	
	outLightVec[gl_InvocationID] = inLightVec[gl_InvocationID];	
}