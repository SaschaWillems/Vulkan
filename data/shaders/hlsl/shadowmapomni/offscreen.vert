// Copyright 2020 Google LLC

struct VSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float4 WorldPos : POSITION0;
[[vk::location(1)]] float3 LightPos : POSITION1;
};

struct UBO
{
	float4x4 projection;
	float4x4 view;
	float4x4 model;
	float4 lightPos;
};

cbuffer ubo : register(b0) { UBO ubo; }

struct PushConsts
{
	float4x4 view;
};
[[vk::push_constant]] PushConsts pushConsts;

VSOutput main([[vk::location(0)]] float3 Pos : POSITION0)
{
	VSOutput output = (VSOutput)0;
	output.Pos = mul(ubo.projection, mul(pushConsts.view, mul(ubo.model, float4(Pos, 1.0))));

	output.WorldPos = float4(Pos, 1.0);
	output.LightPos = ubo.lightPos.xyz;
	return output;
}