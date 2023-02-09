// Copyright 2020 Google LLC

struct VSInput
{
[[vk::location(0)]] float4 Pos : POSITION0;
[[vk::location(2)]] float2 UV : TEXCOORD0;
};

struct UBO
{
	float4x4 projection;
	float4x4 modelview;
};

cbuffer ubo : register(b0) { UBO ubo; }

struct VSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float2 UV : TEXCOORD0;
};

VSOutput main(VSInput input)
{
	VSOutput output = (VSOutput)0;
	output.UV = input.UV;
	// Skysphere always at center, only use rotation part of modelview matrix
	output.Pos = mul(ubo.projection, float4(mul((float3x3)ubo.modelview, input.Pos.xyz), 1));
	return output;
}
