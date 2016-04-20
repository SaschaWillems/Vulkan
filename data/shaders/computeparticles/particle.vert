#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec4 inPos;

layout (location = 0) out vec4 outColor;

void main ()
{
  gl_PointSize = 1.0;
  outColor = vec4(0.04);
  gl_Position = vec4(inPos.xy, 1.0, 1.0);
}
