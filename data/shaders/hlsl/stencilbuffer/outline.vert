// Copyright 2020 Google LLC

struct VSInput
{
[[vk::location(0)]] float4 Pos : POSITION0;
[[vk::location(2)]] float3 Normal : NORMAL0;
};

struct UBO
{
	float4x4 projection;
	float4x4 model;
	float4 lightPos;
	float outlineWidth;
};

cbuffer ubo : register(b0) { UBO ubo; }

float4 main(VSInput input) : SV_POSITION
{
	// Extrude along normal
	float4 pos = float4(input.Pos.xyz + input.Normal * ubo.outlineWidth, input.Pos.w);
	return mul(ubo.projection, mul(ubo.model, pos));
}
