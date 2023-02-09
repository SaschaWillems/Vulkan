// Copyright 2020 Google LLC

struct VSInput
{
[[vk::location(0)]] float3 Pos : POSITION0;
[[vk::location(1)]] float3 Color : COLOR0;
[[vk::location(2)]] float3 Normal : NORMAL0;
};

struct UBO
{
	float4x4 projection;
	float4x4 view;
	float4x4 model;
	float4 lightPos;
};

cbuffer ubo : register(b0) { UBO ubo; }

struct VSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float3 Normal : NORMAL0;
[[vk::location(1)]] float3 Color : COLOR0;
[[vk::location(2)]] float3 EyePos : POSITION0;
[[vk::location(3)]] float3 LightVec : TEXCOORD2;
[[vk::location(4)]] float3 WorldPos : POSITION1;
[[vk::location(5)]] float3 LightPos : POSITION2;
};

VSOutput main(VSInput input)
{
	VSOutput output = (VSOutput)0;
	output.Color = input.Color;
	output.Normal = input.Normal;

	output.Pos = mul(ubo.projection, mul(ubo.view, mul(ubo.model, float4(input.Pos.xyz, 1.0))));
	output.EyePos = mul(ubo.model, float4(input.Pos, 1.0f)).xyz;
	output.LightVec = normalize(ubo.lightPos.xyz - input.Pos.xyz);
	output.WorldPos = input.Pos;

	output.LightPos = ubo.lightPos.xyz;
	return output;
}

