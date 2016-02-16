#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec3 inEyePos;
layout (location = 3) in vec3 inLightVec;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	vec3 Eye = normalize(-inEyePos);
	vec3 Reflected = normalize(reflect(-inLightVec, inNormal)); 

	vec4 IAmbient = vec4(0.1, 0.1, 0.1, 1.0);
	vec4 IDiffuse = vec4(max(dot(inNormal, inLightVec), 0.0));
	float specular = 0.75;
	vec4 ISpecular = vec4(0.0);
	if (dot(inEyePos, inNormal) < 0.0)
	{
		ISpecular = vec4(0.5, 0.5, 0.5, 1.0) * pow(max(dot(Reflected, Eye), 0.0), 16.0) * specular; 
	}

	outFragColor = vec4((IAmbient + IDiffuse) * vec4(inColor, 1.0) + ISpecular);
  
}