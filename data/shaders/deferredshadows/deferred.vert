#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUV;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 modelview;
	mat4 lightMVP;
} ubo;

layout (location = 0) out vec2 outUV;
//layout (location = 1) out vec4 outShadowCoord;

out gl_PerVertex
{
	vec4 gl_Position;
};

/*
const mat4 biasMat = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 );
	*/
void main() 
{
	outUV = inUV;
	gl_Position = ubo.projection * ubo.modelview * vec4(inPos.xyz, 1.0);
	//outShadowCoord = (biasMat * ubo.lightMVP * ubo.modelview) * vec4(inPos, 1.0);	
}
