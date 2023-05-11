// Copyright 2020 Google LLC

struct UBO
{
	float4x4 projection;
	float4x4 modelview;
	float4 lightPos;
};

cbuffer ubo : register(b0) { UBO ubo; }

struct VSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float3 Color : COLOR0;
};

VSOutput main([[vk::location(0)]] float3 Pos : POSITION0)
{
	VSOutput output = (VSOutput)0;
	output.Pos = mul(ubo.projection, mul(ubo.modelview, float4(Pos.xyz, 1.0)));
	return output;
}