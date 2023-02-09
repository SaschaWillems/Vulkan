// Copyright 2020 Google LLC

Texture2D textureColorMap : register(t1);
SamplerState samplerColorMap : register(s1);

struct VSOutput
{
[[vk::location(0)]] float3 Normal : NORMAL0;
[[vk::location(1)]] float3 Color : COLOR0;
[[vk::location(2)]] float2 UV : TEXCOORD0;
};

float4 main(VSOutput input) : SV_TARGET
{
	return textureColorMap.Sample(samplerColorMap, input.UV) * float4(input.Color, 1.0);
}