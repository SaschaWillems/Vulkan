// Copyright 2020 Google LLC

Texture2D textureColorMap : register(t0, space1);
SamplerState samplerColorMap : register(s0, space1);
Texture2D textureNormalMap : register(t1, space1);
SamplerState samplerNormalMap : register(s1, space1);

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

float4 main(VSOutput input) : SV_TARGET
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
	return float4(diffuse * color.rgb + specular, color.a);
}