#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec4 inPos;
layout (location = 1) in vec4 inColor;

layout (location = 0) out vec4 outColor;

void main () 
{
  gl_PointSize = 32.0;
  outColor = inColor;
  gl_Position = vec4(inPos.xyz, 1.0);
}