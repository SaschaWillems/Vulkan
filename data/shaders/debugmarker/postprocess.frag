#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 1) uniform sampler2D samplerColor;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	// Single pass gauss blur

	const vec2 texOffset = vec2(0.01, 0.01);

	vec2 tc0 = inUV + vec2(-texOffset.s, -texOffset.t);
	vec2 tc1 = inUV + vec2(         0.0, -texOffset.t);
	vec2 tc2 = inUV + vec2(+texOffset.s, -texOffset.t);
	vec2 tc3 = inUV + vec2(-texOffset.s,          0.0);
	vec2 tc4 = inUV + vec2(         0.0,          0.0);
	vec2 tc5 = inUV + vec2(+texOffset.s,          0.0);
	vec2 tc6 = inUV + vec2(-texOffset.s, +texOffset.t);
	vec2 tc7 = inUV + vec2(         0.0, +texOffset.t);
	vec2 tc8 = inUV + vec2(+texOffset.s, +texOffset.t);

	vec4 col0 = texture(samplerColor, tc0);
	vec4 col1 = texture(samplerColor, tc1);
	vec4 col2 = texture(samplerColor, tc2);
	vec4 col3 = texture(samplerColor, tc3);
	vec4 col4 = texture(samplerColor, tc4);
	vec4 col5 = texture(samplerColor, tc5);
	vec4 col6 = texture(samplerColor, tc6);
	vec4 col7 = texture(samplerColor, tc7);
	vec4 col8 = texture(samplerColor, tc8);

	vec4 sum = (1.0 * col0 + 2.0 * col1 + 1.0 * col2 + 
			  2.0 * col3 + 4.0 * col4 + 2.0 * col5 +
			  1.0 * col6 + 2.0 * col7 + 1.0 * col8) / 16.0; 
	outFragColor = vec4(sum.rgb, 1.0);
}