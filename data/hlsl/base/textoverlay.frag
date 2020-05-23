// Copyright 2020 Google LLC

Texture2D textureFont : register(t0);
SamplerState samplerFont : register(s0);

float4 main([[vk::location(0)]]float2 inUV : TEXCOORD0) : SV_TARGET
{
	float color = textureFont.Sample(samplerFont, inUV).r;
	return float4(color.xxx, 1.0);
}
