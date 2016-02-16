#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 1) uniform sampler2D sColorMap;
layout (binding = 2) uniform sampler2D sNormalHeightMap;

layout (binding = 3) uniform UBO 
{
	float scale;
	float bias;
	float lightRadius;	
	int usePom;
	int displayNormalMap;
} ubo;

layout (location = 0) in vec2 inUV;
layout (location = 1) in vec3 inLightVec;
layout (location = 2) in vec3 inLightVecB;
layout (location = 3) in vec3 inSpecular;
layout (location = 4) in vec3 inEyeVec;
layout (location = 5) in vec3 inLightDir;
layout (location = 6) in vec3 inViewVec;

layout (location = 0) out vec4 outFragColor;

void main(void) 
{
	vec3 specularColor = vec3(0.0, 0.0, 0.0);

	float invRadius = 1.0/ubo.lightRadius;
	float ambient = 0.5;

	vec3 rgb, normal;

	rgb = (ubo.displayNormalMap == 0) ? texture(sColorMap, inUV).rgb : texture(sNormalHeightMap, inUV).rgb;
	normal = normalize((texture(sNormalHeightMap, inUV).rgb - 0.5) * 2.0);

	float distSqr = dot(inLightVecB, inLightVecB);
	vec3 lVec = inLightVecB * inversesqrt(distSqr);

	vec3  nvViewVec = normalize(inViewVec);
	float specular = pow(clamp(dot(reflect(-nvViewVec, normal), lVec), 0.0, 1.0), 4.0);

	float atten = clamp(1.0 - invRadius * sqrt(distSqr), 0.0, 1.0);
	float diffuse = clamp(dot(lVec, normal), 0.0, 1.0);

	outFragColor = vec4((rgb * ambient + (diffuse * rgb + 0.5 * specular * specularColor.rgb)) * atten, 1.0);    
}
