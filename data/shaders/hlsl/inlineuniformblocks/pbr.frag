// Copyright 2020 Google LLC

struct VSOutput
{
[[vk::location(0)]] float3 WorldPos : POSITION0;
[[vk::location(1)]] float3 Normal : NORMAL0;
};

struct UBO
{
	float4x4 projection;
	float4x4 model;
	float4x4 view;
	float3 camPos;
};
cbuffer ubo : register(b0) { UBO ubo; };

// Inline uniform block
struct UniformInline {
	float roughness;
	float metallic;
	float r;
	float g;
	float b;
	float ambient;
};
cbuffer material : register(b0, space1) { UniformInline material; };

#define PI 3.14159265359

float3 materialcolor()
{
	return float3(material.r, material.g, material.b);
}

// Normal Distribution function --------------------------------------
float D_GGX(float dotNH, float roughness)
{
	float alpha = roughness * roughness;
	float alpha2 = alpha * alpha;
	float denom = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
	return (alpha2)/(PI * denom*denom);
}

// Geometric Shadowing function --------------------------------------
float G_SchlicksmithGGX(float dotNL, float dotNV, float roughness)
{
	float r = (roughness + 1.0);
	float k = (r*r) / 8.0;
	float GL = dotNL / (dotNL * (1.0 - k) + k);
	float GV = dotNV / (dotNV * (1.0 - k) + k);
	return GL * GV;
}

// Fresnel function ----------------------------------------------------
float3 F_Schlick(float cosTheta, float metallic)
{
	float3 F0 = lerp(float3(0.04, 0.04, 0.04), materialcolor(), metallic); // * material.specular
	float3 F = F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
	return F;
}

// Specular BRDF composition --------------------------------------------

float3 BRDF(float3 L, float3 V, float3 N, float metallic, float roughness)
{
	// Precalculate vectors and dot products
	float3 H = normalize (V + L);
	float dotNV = clamp(dot(N, V), 0.0, 1.0);
	float dotNL = clamp(dot(N, L), 0.0, 1.0);
	float dotLH = clamp(dot(L, H), 0.0, 1.0);
	float dotNH = clamp(dot(N, H), 0.0, 1.0);

	// Light color fixed
	float3 lightColor = float3(1.0, 1.0, 1.0);

	float3 color = float3(0.0, 0.0, 0.0);

	if (dotNL > 0.0)
	{
		float rroughness = max(0.05, roughness);
		// D = Normal distribution (Distribution of the microfacets)
		float D = D_GGX(dotNH, rroughness);
		// G = Geometric shadowing term (Microfacets shadowing)
		float G = G_SchlicksmithGGX(dotNL, dotNV, rroughness);
		// F = Fresnel factor (Reflectance depending on angle of incidence)
		float3 F = F_Schlick(dotNV, metallic);

		float3 spec = D * F * G / (4.0 * dotNL * dotNV);

		color += spec * dotNL * lightColor;
	}

	return color;
}

// ----------------------------------------------------------------------------
float4 main(VSOutput input) : SV_TARGET
{
	float3 N = normalize(input.Normal);
	float3 V = normalize(ubo.camPos - input.WorldPos);

	float roughness = material.roughness;

	// Specular contribution
	float3 lightPos = float3(0.0f, 0.0f, 10.0f);
	float3 Lo = float3(0.0, 0.0, 0.0);
	float3 L = normalize(lightPos.xyz - input.WorldPos);
	Lo += BRDF(L, V, N, material.metallic, roughness);

	// Combine with ambient
	float3 color = materialcolor() * material.ambient;
	color += Lo;

	// Gamma correct
	color = pow(color, float3(0.4545, 0.4545, 0.4545));

	return float4(color, 1.0);
}