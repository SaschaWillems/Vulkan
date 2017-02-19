#version 450

layout (binding = 1) uniform samplerCube envmap;
layout (binding = 2) uniform samplerCube envmapibldiff;
layout (binding = 3) uniform samplerCube envmapiblrefl;

layout (location = 0) in vec3 inWorldPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 model;
	mat4 view;
	vec3 camPos;
} ubo;

layout (location = 0) out vec4 outColor;

layout(push_constant) uniform PushConsts {
	layout(offset = 12) float roughness;
	layout(offset = 16) float metallic;
	layout(offset = 20) float r;
	layout(offset = 24) float g;
	layout(offset = 28) float b;
} material;

const float PI = 3.14159265359;

//#define ROUGHNESS_PATTERN 1

// Fresnel ------------------------------------------------------------------------

float fresnelSchlick(float ct, float F0)
{
	return F0 + (1.0 - F0) * pow(1.0 - ct, 5.0);
}

// Normal distribution functions ---------------------------------------------------

float NDF_blinnPhong(float dotNH, float alphaSqr)
{
	return 1.0 / (PI * alphaSqr) * pow(dotNH, 2.0 / alphaSqr - 2.0);
}

float NDF_beckmann(float dotNH, float alphaSqr)
{
	float dotNH2 = dotNH * dotNH;
	return 1.0 / (PI * alphaSqr * dotNH2 * dotNH2) * exp((dotNH2 - 1.0) / (alphaSqr * dotNH2));
}

float NDF_GGX(float dotNH, float alphaSqr)
{
	return alphaSqr / (PI * pow(dotNH * dotNH * (alphaSqr - 1.0) + 1.0, 2.0));
}

// Geometry visibility functions ---------------------------------------------------

float GEOM_SchlickSmith(float dotNL, float dotNV, float alpha)
{
	//float k = alpha * sqrt(2.0 / PI);
	float k = pow(0.8 + 0.5 * alpha, 2.0) / 2.0;
	float GL = 1.0 / (dotNL * (1.0 - k) + k);
	float GV = 1.0 / (dotNV * (1.0 - k) + k);
	return GL * GV;

}

float PBR_Shade(vec3 N, vec3 V, vec3 L, float roughness, float F0) 
{
	float alpha = roughness * roughness;
	float alphaSqr = alpha * alpha;

	vec3 H = normalize (V + L);

	float dotNL = clamp(dot(N, L), 0.0, 1.0);
	float dotNV = clamp(dot(N, V), 0.0, 1.0);
	float dotNH = clamp(dot(N, H), 0.0, 1.0);
	float dotLH = clamp(dot(L, H), 0.0, 1.0);

	// Normal distribution
	float Di = NDF_GGX(dotNH, alphaSqr);
	// Fresnel
	float Fs = fresnelSchlick(dotNV, F0);
	// Visibility term 
	float Vs = GEOM_SchlickSmith(dotNL, dotNV, alpha);

	return dotNL * Di * Fs * Vs;
}

// ----------------------------------------------------------------------------
void main()
{		
	// Partially based on https://www.shadertoy.com/view/XsfXWX by Alexander Alekseev (https://github.com/tdmaav)

	// One fixed light source
	vec3 lightPos = vec3(-10.0f, -10.0f, 10.0f);
	// Take light color from environment map
	vec3 lightColor = texture(envmapiblrefl, vec3(0.5)).xyz;

	vec3 N = normalize(inNormal);
	vec3 V = normalize(ubo.camPos - inWorldPos);
	vec3 L = normalize(lightPos - inWorldPos);
	vec3 R = reflect(-V, N); 

	// Store material values for quick testing/changing inside the shader
	float roughness = material.roughness;
	float metallic = material.metallic;

	// Add striped pattern to roughness based on vertex position
#ifdef ROUGHNESS_PATTERN
	roughness = max(roughness, step(fract(inWorldPos.y * 2.02), 0.5));
#endif

	// Get IBL components from cube maps
	vec3 IBLdiffuse = texture(envmapibldiff, inNormal).rgb;
	vec3 IBLreflection = texture(envmapiblrefl, inNormal).rgb;

	// Fresnel part
	float fresnel = pow(max(1.0 - dot(N, V), 0.0), 1.5);    
		
	// Reflection part        

	// Select mip level based on roughness
	ivec2 dim = textureSize(envmap, 0);
	float nummips = log2(max(dim.s, dim.y));
	vec3 reflection = texture(envmap, R).xyz;
	reflection = textureLod(envmap, R, max(roughness * nummips, textureQueryLod(envmap, R).y)).rgb;
	reflection = mix(reflection, IBLreflection, (1.0-fresnel) * roughness);
	reflection = mix(reflection, IBLreflection, roughness);
		
	// Specular part
	// F0 based on metallic factor of material
	vec3 F0 = vec3(0.04); 
	F0 = mix(F0, lightColor, material.metallic);
	vec3 spec = lightColor * PBR_Shade(N, V, L, roughness, 0.2);
	reflection -= spec;
		
	// Diffuse part
	vec3 matColor = vec3(material.r, material.g, material.b);
	vec3 diffuse = mix(IBLdiffuse * matColor, reflection, fresnel);

	// Final output mixes based on material metalness
	outColor.rgb = mix(diffuse, reflection, metallic) + spec;
}