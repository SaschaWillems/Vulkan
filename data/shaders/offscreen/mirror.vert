#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUV;
layout (location = 3) in vec3 inNormal;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 model;
} ubo;

layout (location = 0) out vec2 outUV;
layout (location = 1) out vec4 outPos;
layout (location = 2) out vec3 outNormal;

mat3 mat3_emu(mat4 m4) {
  return mat3(
      m4[0][0], m4[0][1], m4[0][2],
      m4[1][0], m4[1][1], m4[1][2],
      m4[2][0], m4[2][1], m4[2][2]);
}

void main() 
{
    mat3 rotation = mat3_emu(ubo.model);
	outNormal = rotation * inNormal;
	outUV = inUV;
	outPos = ubo.projection * ubo.model * vec4(inPos.xyz, 1.0);
	gl_Position = outPos;		
}
