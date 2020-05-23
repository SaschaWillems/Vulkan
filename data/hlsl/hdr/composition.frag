// Copyright 2020 Google LLC

Texture2D textureColor0 : register(t0);
SamplerState samplerColor0 : register(s0);
Texture2D textureColor1 : register(t1);
SamplerState samplerColor1 : register(s1);

float4 main([[vk::location(0)]] float2 inUV : TEXCOORD0) : SV_TARGET
{
	return textureColor0.Sample(samplerColor0, inUV);
}