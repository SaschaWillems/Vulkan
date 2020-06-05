#version 450

layout (location = 0) in vec2 inUV;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec3 inEyePos;
layout (location = 4) in vec3 inLightVec;

layout (location = 0) out vec4 outFragColor;

void main() 
{
  vec3 Eye = normalize(-inEyePos);
  vec3 Reflected = normalize(reflect(-inLightVec, inNormal));

  vec4 diff = vec4(inColor, 1.0) * max(dot(inNormal, inLightVec), 0.0);
  float shininess = 0.0;
  vec4 spec = vec4(1.0, 1.0, 1.0, 1.0) * pow(max(dot(Reflected, Eye), 0.0), 2.5) * shininess;

  outFragColor = diff + spec;
  outFragColor.a = 1.0; 
}