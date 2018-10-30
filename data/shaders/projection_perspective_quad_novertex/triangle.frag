#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inColor;

layout (location = 0) out vec4 outFragColor;

void main() 
{
  //outFragColor = vec4(inColor, 1.0);
  float scale = 1.0f;
  outFragColor = texture(samplerColor, vec2(inUV.s*scale, scale*(1.0 - inUV.t)));
}
