// Copyright 2020 Google LLC

Texture2D textureGradientRamp : register(t1);
SamplerState samplerGradientRamp : register(s1);

struct VSOutput
{
[[vk::location(0)]] float3 Color : COLOR0;
[[vk::location(1)]] float2 UV : TEXCOORD0;
};

float4 main(VSOutput input) : SV_TARGET
{
	// Use max. color channel value to detect bright glow emitters
	if ((input.Color.r >= 0.9) || (input.Color.g >= 0.9) || (input.Color.b >= 0.9))
	{
		return float4(textureGradientRamp.Sample(samplerGradientRamp, input.UV).rgb, 1);
	}
	else
	{
		return float4(input.Color, 1);
	}
}