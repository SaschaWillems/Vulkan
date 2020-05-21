// Copyright 2020 Google LLC

struct VSInput
{
[[vk::location(0)]] float3 Pos : POSITION0;
[[vk::location(1)]] float2 UV : TEXCOORD0;
};

struct UBO
{
	float4x4 projection;
	float4x4 view;
	float4x4 model;
};

cbuffer ubo : register(b0) { UBO ubo; }

struct VSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float2 UV : TEXCOORD0;
[[vk::location(1)]] float4 ProjCoord : POSITION0;
};

VSOutput main(VSInput input)
{
	VSOutput output = (VSOutput)0;
	output.UV = input.UV;
	output.ProjCoord = mul(ubo.projection, mul(ubo.view, mul(ubo.model, float4(input.Pos.xyz, 1.0))));
	output.Pos = output.ProjCoord;
	return output;
}
