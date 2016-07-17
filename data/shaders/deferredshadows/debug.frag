#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 1) uniform sampler2D samplerPosition;
layout (binding = 2) uniform sampler2D samplerNormal;
layout (binding = 3) uniform sampler2D samplerAlbedo;
layout (binding = 5) uniform sampler2DArray samplerDepth;

layout (location = 0) in vec3 inUV;

layout (location = 0) out vec4 outFragColor;

float LinearizeDepth(float depth)
{
  float n = 0.1; // camera z near
  float f = 64.0; // camera z far
  float z = depth;
  return (2.0 * n) / (f + n - z * (f - n));	
}

void main() 
{
	// Display depth from light's point-of-view 
	// inUV.w = number of light source
	float depth = texture(samplerDepth, vec3(inUV)).r;
	outFragColor = vec4(vec3(1.0 - LinearizeDepth(depth)), 0.0);
}