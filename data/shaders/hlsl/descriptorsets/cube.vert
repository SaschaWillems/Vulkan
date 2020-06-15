// Copyright 2020 Google LLC

struct VSInput
{
[[vk::location(0)]] float3 Pos : POSITION0;
[[vk::location(1)]] float3 Normal : NORMAL0;
[[vk::location(2)]] float2 UV : TEXCOORD0;
[[vk::location(3)]] float3 Color : COLOR0;
};

struct UBOMatrices {
	float4x4 projection;
	float4x4 view;
	float4x4 model;
};

cbuffer uboMatrices : register(b0) { UBOMatrices uboMatrices; };

struct VSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float3 Normal : NORMAL0;
[[vk::location(1)]] float3 Color : COLOR0;
[[vk::location(2)]] float2 UV : TEXCOORD0;
};

VSOutput main(VSInput input)
{
	VSOutput output = (VSOutput)0;
	output.Normal = input.Normal;
	output.Color = input.Color;
	output.UV = input.UV;
	output.Pos = mul(uboMatrices.projection, mul(uboMatrices.view, mul(uboMatrices.model, float4(input.Pos.xyz, 1.0))));
	return output;
}