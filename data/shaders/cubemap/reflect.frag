#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 1) uniform samplerCube samplerColor;

layout (location = 0) in vec3 inEyePos;
layout (location = 1) in vec3 inEyeNormal;
layout (location = 2) in mat4 inView;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	vec3 I = normalize (inEyePos);
	vec3 R = reflect (I, normalize(inEyeNormal));

	R = vec3(inverse (inView) * vec4(R, 0.0));
	
	outFragColor = texture(samplerColor, R);
}