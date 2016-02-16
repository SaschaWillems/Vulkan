#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec2 vTexcoord;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vColor;
layout (location = 3) in vec3 vEyePos;
layout (location = 4) in vec3 vLightVec;

layout (location = 0) out vec4 outFragColor;

void main() 
{
  vec3 Eye = normalize(-vEyePos);
  vec3 Reflected = normalize(reflect(-vLightVec, vNormal)); 
 
  vec4 IAmbient = vec4(0.2, 0.2, 0.2, 1.0);
  vec4 IDiffuse = vec4(0.5, 0.5, 0.5, 0.5) * max(dot(vNormal, vLightVec), 0.0);
  float specular = 0.75;
  vec4 ISpecular = vec4(0.5, 0.5, 0.5, 1.0) * pow(max(dot(Reflected, Eye), 0.0), 0.8) * specular; 
 
  outFragColor = vec4((IAmbient + IDiffuse) * vec4(vColor, 1.0) + ISpecular);
}