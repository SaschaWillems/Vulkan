#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec2 inPos;
layout (location = 1) in vec4 inGradientPos;

layout (location = 0) out vec4 outColor;
layout (location = 1) out float outGradientPos;

out gl_PerVertex
{
	vec4 gl_Position;
	float gl_PointSize;
};

void main () 
{
  gl_PointSize = 8.0;
  outColor = vec4(0.035);
  outGradientPos = inGradientPos.x;
  gl_Position = vec4(inPos.xy, 1.0, 1.0);
}