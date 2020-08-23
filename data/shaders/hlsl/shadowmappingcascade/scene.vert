// Copyright 2020 Google LLC

struct VSInput
{
[[vk::location(0)]] float3 Pos : POSITION0;
[[vk::location(1)]] float2 UV : TEXCOORD0;
[[vk::location(2)]] float3 Color : COLOR0;
[[vk::location(3)]] float3 Normal : NORMAL0;
};

struct UBO  {
	float4x4 projection;
	float4x4 view;
	float4x4 model;
};

cbuffer ubo : register(b0) { UBO ubo; }

struct VSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float3 Normal : NORMAL0;
[[vk::location(1)]] float3 Color : COLOR0;
[[vk::location(2)]] float3 ViewPos : POSITION1;
[[vk::location(3)]] float3 WorldPos : POSITION0;
[[vk::location(4)]] float2 UV : TEXCOORD0;
};

struct PushConsts {
	float4 position;
	uint cascadeIndex;
};
[[vk::push_constant]] PushConsts pushConsts;

VSOutput main(VSInput input)
{
	VSOutput output = (VSOutput)0;
	output.Color = input.Color;
	output.Normal = input.Normal;
	output.UV = input.UV;
	float3 pos = input.Pos + pushConsts.position.xyz;
	output.WorldPos = pos;
	output.ViewPos = mul(ubo.view, float4(pos.xyz, 1.0)).xyz;
	output.Pos = mul(ubo.projection, mul(ubo.view, mul(ubo.model, float4(pos.xyz, 1.0))));
	return output;
}

