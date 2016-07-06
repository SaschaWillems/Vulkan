#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 1) uniform sampler2D samplerColor;
layout (binding = 2) uniform sampler2D samplerColorMap;

layout (location = 0) in vec2 inUV;
layout (location = 1) in vec4 inPos;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	vec4 tmp = vec4(1.0 / inPos.w);
	vec4 projCoord = inPos * tmp;

	// Scale and bias
	projCoord += vec4(1.0);
	projCoord *= vec4(0.5);
		
	// Slow single pass blur
	// For demonstration purposes only
	const float blurSize = 1.0 / 512.0;	

	vec4 color = texture(samplerColorMap, inUV);
	outFragColor = color * 0.25;

	if (gl_FrontFacing) 
	{
		// Only render mirrored scene on front facing (upper) side of mirror surface
		vec4 reflection = vec4(0.0);
		for (int x = -3; x <= 3; x++)
		{
			for (int y = -3; y <= 3; y++)
			{
				reflection += texture(samplerColor, vec2(projCoord.s + x * blurSize, projCoord.t + y * blurSize)) / 49.0;
			}
		}
		outFragColor += reflection * 1.5 * (color.r);
	};
}