// Copyright 2020 Google LLC

Texture2D textureColor : register(t2);
SamplerState samplerColor : register(s2);

struct VSOutput
{
[[vk::location(0)]] float2 UV : TEXCOORD0;
};

float4 main(VSOutput input) : SV_TARGET
{
	float4 color = textureColor.Sample(samplerColor, input.UV);
	return float4(color.rgb, 1.0);
}