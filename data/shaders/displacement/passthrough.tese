#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (triangles) in;

layout (binding = 1) uniform UBO 
{
	mat4 projection;
	mat4 model;
	vec3 lightPos;	
	float tessAlpha;
	float tessStrength;	
} ubo; 

layout (location = 0) in vec3 inNormal[];
layout (location = 3) in vec2 inTexCoord[];
layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec2 outTexCoord;
layout (location = 2) out vec3 outEyesPos;
layout (location = 3) out vec3 outLightVec;

void main(void)
{
    gl_Position = (gl_TessCoord.x * gl_in[0].gl_Position) + (gl_TessCoord.y * gl_in[1].gl_Position) + (gl_TessCoord.z * gl_in[2].gl_Position);
	outNormal = gl_TessCoord.x * inNormal[0] + gl_TessCoord.y * inNormal[1] + gl_TessCoord.z * inNormal[2];
	outTexCoord  = gl_TessCoord.x * inTexCoord[0] + gl_TessCoord.y * inTexCoord[1] + gl_TessCoord.z * inTexCoord[2];	
	gl_Position.xyz += normalize(outNormal);
	outEyesPos = (gl_Position).xyz;
	outLightVec = normalize(ubo.lightPos - outEyesPos);	
	gl_Position = ubo.projection * ubo.model * gl_Position;
	
}