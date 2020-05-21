// Copyright 2020 Google LLC

struct VSInput
{
[[vk::location(0)]] float4 Pos : POSITION0;
[[vk::location(1)]] float3 Color : COLOR0;
[[vk::location(2)]] float3 Normal : NORMAL0;
};

struct UBO
{
	float4x4 projection;
	float4x4 model;
	float4x4 view;
};

cbuffer ubo : register(b0) { UBO ubo; }

struct VSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float3 Normal : NORMAL0;
[[vk::location(1)]] float3 Color : COLOR0;
[[vk::location(2)]] float3 WorldPos : POSITION0;
[[vk::location(3)]] float3 Tangent : TEXCOORD1;
};

VSOutput main(VSInput input)
{
	VSOutput output = (VSOutput)0;
	output.Pos = mul(ubo.projection, mul(ubo.view, mul(ubo.model, input.Pos)));

	// Vertex position in world space
	output.WorldPos = mul(ubo.model, input.Pos).xyz;
	// GL to Vulkan coord space
	output.WorldPos.y = -output.WorldPos.y;

	// Normal in world space
	output.Normal = mul((float3x3)ubo.model, normalize(input.Normal));

	// Currently just vertex color
	output.Color = input.Color;
	return output;
}
