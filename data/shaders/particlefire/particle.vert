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
layout (location = 1) out float outAlpha;
layout (location = 2) out flat int outType;
layout (location = 3) out float outRotation;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 modelview;
	vec2 viewportDim;
	float pointSize;
} ubo;

out gl_PerVertex
{
	vec4 gl_Position;
	float gl_PointSize;
};

void main () 
{
	outColor = inColor;
	outAlpha = inAlpha;
	outType = inType;
	outRotation = inRotation;
	  
	gl_Position = ubo.projection * ubo.modelview * vec4(inPos.xyz, 1.0);	
	
	// Base size of the point sprites
	float spriteSize = 8.0 * inSize;

	// Scale particle size depending on camera projection
	vec4 eyePos = ubo.modelview * vec4(inPos.xyz, 1.0);
	vec4 projectedCorner = ubo.projection * vec4(0.5 * spriteSize, 0.5 * spriteSize, eyePos.z, eyePos.w);
	gl_PointSize = ubo.viewportDim.x * projectedCorner.x / projectedCorner.w;
	
}