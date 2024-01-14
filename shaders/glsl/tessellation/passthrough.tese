#version 450

layout (triangles, fractional_odd_spacing, cw) in;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 model;
	float tessAlpha;
	float tessLevel;
} ubo; 

layout (location = 0) in vec3 inNormal[];
layout (location = 1) in vec2 inUV[];

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec2 outUV;

void main(void)
{
    gl_Position = (gl_TessCoord.x * gl_in[0].gl_Position) +
                  (gl_TessCoord.y * gl_in[1].gl_Position) +
                  (gl_TessCoord.z * gl_in[2].gl_Position);
	gl_Position = ubo.projection * ubo.model * gl_Position;
	
	outNormal = gl_TessCoord.x*inNormal[0] + gl_TessCoord.y*inNormal[1] + gl_TessCoord.z*inNormal[2];
	outUV = gl_TessCoord.x*inUV[0] + gl_TessCoord.y*inUV[1] + gl_TessCoord.z*inUV[2];
}