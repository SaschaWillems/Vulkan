// Copyright 2020 Google LLC

Texture2D fontTexture : register(t0);
SamplerState fontSampler : register(s0);

struct VSOutput
{
	[[vk::location(0)]]float2 UV : TEXCOORD0;
	[[vk::location(1)]]float4 Color : COLOR0;
};

float4 main(VSOutput input) : SV_TARGET
{
	return input.Color * fontTexture.Sample(fontSampler, input.UV);
}