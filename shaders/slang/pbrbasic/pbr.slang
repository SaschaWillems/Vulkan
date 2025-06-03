/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSInput
{
    float3 Pos;
    float3 Normal;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float3 WorldPos;
    float3 Normal;
};

struct UBO
{
    float4x4 projection;
    float4x4 model;
    float4x4 view;
    float3 camPos;
};
ConstantBuffer<UBO> ubo;

struct UBOParams {
	float4 lights[4];
};
ConstantBuffer<UBOParams> uboParams;

struct Material {
	[[vk::offset(12)]] float roughness;
	[[vk::offset(16)]] float metallic;
	[[vk::offset(20)]] float r;
	[[vk::offset(24)]] float g;
	[[vk::offset(28)]] float b;
};
[[vk::push_constant]] Material material;

static const float PI = 3.14159265359;

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
float3 F_Schlick(float cosTheta, Material material)
{
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), float3(material.r, material.g, material.b), material.metallic); // * material.specular
	float3 F = F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
	return F;
}

// Specular BRDF composition --------------------------------------------

float3 BRDF(float3 L, float3 V, float3 N, Material material)
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
        float rroughness = max(0.05, material.roughness);
        // D = Normal distribution (Distribution of the microfacets)
        float D = D_GGX(dotNH, material.roughness);
		// G = Geometric shadowing term (Microfacets shadowing)
		float G = G_SchlicksmithGGX(dotNL, dotNV, rroughness);
        // F = Fresnel factor (Reflectance depending on angle of incidence)
        float3 F = F_Schlick(dotNV, material);

		float3 spec = D * F * G / (4.0 * dotNL * dotNV);

		color += spec * dotNL * lightColor;
	}

	return color;
}

[shader("vertex")]
VSOutput vertexMain(VSInput input, uniform float3 objPos)
{
    VSOutput output;
    float3 locPos = mul(ubo.model, float4(input.Pos, 1.0)).xyz;
    output.WorldPos = locPos + objPos;
    output.Normal = mul((float3x3)ubo.model, input.Normal);
    output.Pos = mul(ubo.projection, mul(ubo.view, float4(output.WorldPos, 1.0)));
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
	float3 N = normalize(input.Normal);
	float3 V = normalize(ubo.camPos - input.WorldPos);

	// Specular contribution
	float3 Lo = float3(0.0, 0.0, 0.0);
	for (int i = 0; i < 4; i++) {
		float3 L = normalize(uboParams.lights[i].xyz - input.WorldPos);
		Lo += BRDF(L, V, N, material);
	};

	// Combine with ambient
	float3 color = float3(material.r, material.g, material.b) * 0.02;
	color += Lo;

	// Gamma correct
	color = pow(color, float3(0.4545, 0.4545, 0.4545));

	return float4(color, 1.0);
}