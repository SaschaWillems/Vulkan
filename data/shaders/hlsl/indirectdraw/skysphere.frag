// Copyright 2020 Google LLC

Texture2D textureColor : register(t2);
SamplerState samplerColor : register(s2);

float4 main([[vk::location(0)]] float2 inUV : TEXCOORD0) : SV_TARGET
{
	const float4 gradientStart = float4(0.93, 0.9, 0.81, 1.0);
	const float4 gradientEnd = float4(0.35, 0.5, 1.0, 1.0);
	return lerp(gradientStart, gradientEnd, min(0.5 - (inUV.y + 0.05), 0.5)/0.15 + 0.5);
}