// Copyright 2020 Google LLC

Texture2DArray shadowMapTexture : register(t1);
SamplerState shadowMapSampler : register(s1);

struct VSOutput
{
[[vk::location(0)]] float2 UV : TEXCOORD0;
[[vk::location(1)]] uint CascadeIndex : TEXCOORD1;
};

float4 main(VSOutput input) : SV_TARGET
{
	float depth = shadowMapTexture.Sample(shadowMapSampler, float3(input.UV, float(input.CascadeIndex))).r;
	return float4(depth.xxx, 1.0);
}