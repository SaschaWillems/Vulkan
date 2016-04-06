#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 1) uniform sampler2DArray matCap;

layout (location = 0) in vec3 inColor;
layout (location = 1) in vec3 inEyePos;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in flat int inTexIndex;

layout (location = 0) out vec4 outFragColor;

void main()
{
	vec3 r = reflect( inEyePos, inNormal );
	vec3 r2 = vec3( r.x, r.y, r.z + 1.0 );
	float m = 2.0 * length( r2 );
	vec2 vN = r.xy / m + .5;
	outFragColor = vec4( texture( matCap, vec3(vN, inTexIndex)).rgb * (clamp(inColor.r * 2, 0.0, 1.0)), 1.0 );
}
