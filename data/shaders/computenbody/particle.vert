#version 450

layout (location = 0) in vec4 inPos;
layout (location = 1) in vec4 inVel;

layout (location = 0) out float outGradientPos;

layout (binding = 2) uniform UBO 
{
	mat4 projection;
	mat4 modelview;
	vec2 screendim;
} ubo;

out gl_PerVertex
{
	vec4 gl_Position;
	float gl_PointSize;
};

void main () 
{
	const float spriteSize = 0.005 * inPos.w; // Point size influenced by mass (stored in inPos.w);

	vec4 eyePos = ubo.modelview * vec4(inPos.x, inPos.y, inPos.z, 1.0); 
	vec4 projectedCorner = ubo.projection * vec4(0.5 * spriteSize, 0.5 * spriteSize, eyePos.z, eyePos.w);
	gl_PointSize = clamp(ubo.screendim.x * projectedCorner.x / projectedCorner.w, 1.0, 128.0);
	
	gl_Position = ubo.projection * eyePos;

	outGradientPos = inVel.w;
}