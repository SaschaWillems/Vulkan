// Copyright 2020 Google LLC

Texture2D textureColorMap : register(t1);
SamplerState samplerColorMap : register(s1);

float4 main([[vk::location(0)]] float2 inUV : TEXCOODR0) : SV_TARGET
{
	float4 color = textureColorMap.Sample(samplerColorMap, inUV);
	return float4(color.rgb, 1.0);
}
