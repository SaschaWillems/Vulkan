// Copyright 2020 Google LLC

Texture2D textureColorMap : register(t0);
SamplerState samplerColorMap : register(s0);
Texture2D textureGradientRamp : register(t1);
SamplerState samplerGradientRamp : register(s1);

struct VSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float4 Color : COLOR0;
[[vk::location(1)]] float GradientPos : POSITION0;
[[vk::location(2)]] float2 CenterPos : POSITION1;
[[vk::location(3)]] float PointSize : TEXCOORD0;
};

float4 main (VSOutput input) : SV_TARGET
{
	float3 color = textureGradientRamp.Sample(samplerGradientRamp, float2(input.GradientPos, 0.0)).rgb;
	float2 PointCoord = (input.Pos.xy - input.CenterPos.xy) / input.PointSize + 0.5;
	return float4(textureColorMap.Sample(samplerColorMap, PointCoord).rgb * color, 1.0);
}
