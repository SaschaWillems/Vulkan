// Copyright 2020 Google LLC

struct VSInput
{
[[vk::location(0)]] float4 Pos : POSITION0;
[[vk::location(1)]] float2 UV : TEXCOORD0;
[[vk::location(2)]] float3 Color : COLOR0;
[[vk::location(3)]] float3 Normal : NORMAL0;
[[vk::location(4)]] float3 Tangent : TEXCOORD1;
};

struct UBO
{
	float4x4 projection;
	float4x4 model;
	float4x4 view;
	float4 instancePos[3];
};

cbuffer ubo : register(b0) { UBO ubo; }

struct VSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float3 Normal : NORMAL0;
[[vk::location(1)]] float2 UV : TEXCOORD0;
[[vk::location(2)]] float3 Color : COLOR0;
[[vk::location(3)]] float3 WorldPos : POSITION0;
[[vk::location(4)]] float3 Tangent : TEXCOORD1;
};

VSOutput main(VSInput input, uint InstanceIndex : SV_InstanceID)
{
	VSOutput output = (VSOutput)0;
	float4 tmpPos = input.Pos + ubo.instancePos[InstanceIndex];

	output.Pos = mul(ubo.projection, mul(ubo.view, mul(ubo.model, tmpPos)));

	output.UV = input.UV;

	// Vertex position in world space
	output.WorldPos = mul(ubo.model, tmpPos).xyz;

	// Normal in world space
	output.Normal = normalize(input.Normal);
	output.Tangent = normalize(input.Tangent);

	// Currently just vertex color
	output.Color = input.Color;
	return output;
}
