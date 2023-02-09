#version 450

#extension GL_NV_shading_rate_image : require

layout (set = 1, binding = 0) uniform sampler2D samplerColorMap;
layout (set = 1, binding = 1) uniform sampler2D samplerNormalMap;

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inViewVec;
layout (location = 4) in vec3 inLightVec;
layout (location = 5) in vec4 inTangent;

layout (set = 0, binding = 0) uniform UBOScene 
{
	mat4 projection;
	mat4 view;
	mat4 model;
	vec4 lightPos;
	vec4 viewPos;
	int colorShadingRates;
} uboScene;

layout (location = 0) out vec4 outFragColor;

layout (constant_id = 0) const bool ALPHA_MASK = false;
layout (constant_id = 1) const float ALPHA_MASK_CUTOFF = 0.0f;

void main() 
{
	vec4 color = texture(samplerColorMap, inUV) * vec4(inColor, 1.0);

	if (ALPHA_MASK) {
		if (color.a < ALPHA_MASK_CUTOFF) {
			discard;
		}
	}

	vec3 N = normalize(inNormal);
	vec3 T = normalize(inTangent.xyz);
	vec3 B = cross(inNormal, inTangent.xyz) * inTangent.w;
	mat3 TBN = mat3(T, B, N);
	N = TBN * normalize(texture(samplerNormalMap, inUV).xyz * 2.0 - vec3(1.0));

	const float ambient = 0.25;
	vec3 L = normalize(inLightVec);
	vec3 V = normalize(inViewVec);
	vec3 R = reflect(-L, N);
	vec3 diffuse = max(dot(N, L), ambient).rrr;
	float specular = pow(max(dot(R, V), 0.0), 32.0);
	outFragColor = vec4(diffuse * color.rgb + specular, color.a);

	if (uboScene.colorShadingRates == 1) {
		if (gl_FragmentSizeNV.x == 1 && gl_FragmentSizeNV.y == 1) {
			outFragColor.rgb *= vec3(0.0, 0.8, 0.4);
			return;
		}
		if (gl_FragmentSizeNV.x == 2 && gl_FragmentSizeNV.y == 1) {
			outFragColor.rgb *= vec3(0.2, 0.6, 1.0);
			return;
		}
		if (gl_FragmentSizeNV.x == 1 && gl_FragmentSizeNV.y == 2) {
			outFragColor.rgb *= vec3(0.0, 0.4, 0.8);
			return;
		}
		if (gl_FragmentSizeNV.x == 2 && gl_FragmentSizeNV.y == 2) {
			outFragColor.rgb *= vec3(1.0, 1.0, 0.2);
			return;
		}
		if (gl_FragmentSizeNV.x == 4 && gl_FragmentSizeNV.y == 2) {
			outFragColor.rgb *= vec3(0.8, 0.8, 0.0);
			return;
		}
		if (gl_FragmentSizeNV.x == 2 && gl_FragmentSizeNV.y == 4) {
			outFragColor.rgb *= vec3(1.0, 0.4, 0.2);
			return;
		}
		if (gl_FragmentSizeNV.x == 4 && gl_FragmentSizeNV.y == 4) {
			outFragColor.rgb *= vec3(0.8, 0.0, 0.0);
			return;
		}
	}
}