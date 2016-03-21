#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec4 inPos;
layout (location = 1) in vec4 inColor;
layout (location = 2) in float inAlpha;
layout (location = 3) in float inSize;
layout (location = 4) in float inRotation;
layout (location = 5) in int inType;

layout (location = 0) out vec4 outColor;
layout (location = 1) out float outPointSize;
layout (location = 2) out float outAlpha;
layout (location = 3) out flat int outType;
layout (location = 4) out float outRotation;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 modelview;
	vec2 viewportDim;
	float pointSize;
} ubo;

void main () 
{
	gl_PointSize = ubo.pointSize;
	outColor = inColor;
	outAlpha = inAlpha;
	outType = inType;
	outRotation = inRotation;
	  
	vec4 eyePos = ubo.modelview * vec4(inPos.xyz, 1.0);
	vec4 projVoxel = ubo.projection * vec4(ubo.pointSize, ubo.pointSize, eyePos.z, eyePos.w);
	vec2 projSize = ubo.viewportDim * projVoxel.xy / projVoxel.w;

	outPointSize = inSize * 0.25 * (projSize.x + projSize.y);
	
	gl_PointSize = outPointSize;
		
	gl_Position = ubo.projection * ubo.modelview * vec4(inPos.xyz, 1.0);	
	
	float pointDist = (gl_Position.w == 0.0) ? 0.00001 : gl_Position.w;
	float viewportAR = ubo.viewportDim.x / ubo.viewportDim.y;
	
	gl_PointSize = (((inSize * 2048.0 * viewportAR) / pointDist) * viewportAR);	
	
}