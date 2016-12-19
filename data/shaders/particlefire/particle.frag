#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 1) uniform sampler2D samplerSmoke;
layout (binding = 2) uniform sampler2D samplerFire;

layout (location = 0) in vec4 inColor;
layout (location = 1) in float inAlpha;
layout (location = 2) in flat int inType;
layout (location = 3) in float inRotation;


layout (location = 0) out vec4 outFragColor;

void main () 
{
	vec4 color;
	float alpha = (inAlpha <= 1.0) ? inAlpha : 2.0 - inAlpha;
	
	// Rotate texture coordinates
	// Rotate UV	
	float rotCenter = 0.5;
	float rotCos = cos(inRotation);
	float rotSin = sin(inRotation);
	vec2 rotUV = vec2(
		rotCos * (gl_PointCoord.x - rotCenter) + rotSin * (gl_PointCoord.y - rotCenter) + rotCenter,
		rotCos * (gl_PointCoord.y - rotCenter) - rotSin * (gl_PointCoord.x - rotCenter) + rotCenter);

	
	if (inType == 0) 
	{
		// Flame
		color = texture(samplerFire, rotUV);
		outFragColor.a = 0.0;
	}
	else
	{
		// Smoke
		color = texture(samplerSmoke, rotUV);
		outFragColor.a = color.a * alpha;
	}
	
	outFragColor.rgb = color.rgb * inColor.rgb * alpha;	
}
