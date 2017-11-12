#version 450

layout (triangles) in;

layout (location = 0) in vec3 inNormal[];
layout (location = 1) in vec3 inColor[];
layout (location = 2) in vec3 inViewVec[];
layout (location = 3) in vec3 inLightVec[];

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec3 outViewVec;
layout (location = 3) out vec3 outLightVec;

void main(void)
{
    gl_Position = (gl_TessCoord.x * gl_in[2].gl_Position) +
                  (gl_TessCoord.y * gl_in[1].gl_Position) +
                  (gl_TessCoord.z * gl_in[0].gl_Position);
	outNormal = gl_TessCoord.x*inNormal[2] + gl_TessCoord.y*inNormal[1] + gl_TessCoord.z*inNormal[0];
	outViewVec = gl_TessCoord.x*inViewVec[2] + gl_TessCoord.y*inViewVec[1] + gl_TessCoord.z*inViewVec[0];
	outLightVec = gl_TessCoord.x*inLightVec[2] + gl_TessCoord.y*inLightVec[1] + gl_TessCoord.z*inLightVec[0];
	outColor = inColor[0];
}