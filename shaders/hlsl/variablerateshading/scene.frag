// Copyright 2020 Sascha Willems

Texture2D textureColorMap : register(t0, space1);
SamplerState samplerColorMap : register(s0, space1);
Texture2D textureNormalMap : register(t1, space1);
SamplerState samplerNormalMap : register(s1, space1);

struct UBO
{
	float4x4 projection;
	float4x4 view;
	float4x4 model;
	float4 lightPos;
	float4 viewPos;
	int colorShadingRates;
};
cbuffer ubo : register(b0) { UBO ubo; };

[[vk::constant_id(0)]] const bool ALPHA_MASK = false;
[[vk::constant_id(1)]] const float ALPHA_MASK_CUTOFF = 0.0;

struct VSOutput
{
[[vk::location(0)]] float3 Normal : NORMAL0;
[[vk::location(1)]] float3 Color : COLOR0;
[[vk::location(2)]] float2 UV : TEXCOORD0;
[[vk::location(3)]] float3 ViewVec : TEXCOORD1;
[[vk::location(4)]] float3 LightVec : TEXCOORD2;
[[vk::location(5)]] float4 Tangent : TEXCOORD3;
};

float4 main(VSOutput input, uint shadingRate : SV_ShadingRate) : SV_TARGET
{
	float4 color = textureColorMap.Sample(samplerColorMap, input.UV) * float4(input.Color, 1.0);

	if (ALPHA_MASK) {
		if (color.a < ALPHA_MASK_CUTOFF) {
			discard;
		}
	}

	float3 N = normalize(input.Normal);
	float3 T = normalize(input.Tangent.xyz);
	float3 B = cross(input.Normal, input.Tangent.xyz) * input.Tangent.w;
	float3x3 TBN = float3x3(T, B, N);
	N = mul(normalize(textureNormalMap.Sample(samplerNormalMap, input.UV).xyz * 2.0 - float3(1.0, 1.0, 1.0)), TBN);

	const float ambient = 0.1;
	float3 L = normalize(input.LightVec);
	float3 V = normalize(input.ViewVec);
	float3 R = reflect(-L, N);
	float3 diffuse = max(dot(N, L), ambient).rrr;
	float3 specular = pow(max(dot(R, V), 0.0), 32.0);
	color =  float4(diffuse * color.rgb + specular, color.a);

    const uint SHADING_RATE_PER_PIXEL = 0x0;
    const uint SHADING_RATE_PER_2X1_PIXELS = 6;
    const uint SHADING_RATE_PER_1X2_PIXELS = 7;
    const uint SHADING_RATE_PER_2X2_PIXELS = 8;
    const uint SHADING_RATE_PER_4X2_PIXELS = 9;
    const uint SHADING_RATE_PER_2X4_PIXELS = 10;

	if (ubo.colorShadingRates == 1) {
		switch(shadingRate) {
			case SHADING_RATE_PER_PIXEL:
				return color * float4(0.0, 0.8, 0.4, 1.0);
			case SHADING_RATE_PER_2X1_PIXELS:
				return color * float4(0.2, 0.6, 1.0, 1.0);
			case SHADING_RATE_PER_1X2_PIXELS:
				return color * float4(0.0, 0.4, 0.8, 1.0);
			case SHADING_RATE_PER_2X2_PIXELS:
				return color * float4(1.0, 1.0, 0.2, 1.0);
			case SHADING_RATE_PER_4X2_PIXELS:
				return color * float4(0.8, 0.8, 0.0, 1.0);
			case SHADING_RATE_PER_2X4_PIXELS:
				return color * float4(1.0, 0.4, 0.2, 1.0);
		default:
			return color * float4(0.8, 0.0, 0.0, 1.0);
		}
	}

	return color;
}