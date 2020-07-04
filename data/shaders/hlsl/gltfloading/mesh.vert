// Copyright 2020 Google LLC

struct VSInput
{
[[vk::location(0)]] float3 Pos : POSITION0;
[[vk::location(1)]] float3 Normal : NORMAL0;
[[vk::location(2)]] float2 UV : TEXCOORD0;
[[vk::location(3)]] float3 Color : COLOR0;
};

struct UBO
{
	float4x4 projection;
	float4x4 view;
	float4 lightPos;
};

cbuffer ubo : register(b0) { UBO ubo; }

struct PushConsts {
	float4x4 model;
};
[[vk::push_constant]] PushConsts primitive;

struct VSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float3 Normal : NORMAL0;
[[vk::location(1)]] float3 Color : COLOR0;
[[vk::location(2)]] float2 UV : TEXCOORD0;
[[vk::location(3)]] float3 ViewVec : TEXCOORD1;
[[vk::location(4)]] float3 LightVec : TEXCOORD2;
};

VSOutput main(VSInput input)
{
	VSOutput output = (VSOutput)0;
	output.Normal = input.Normal;
	output.Color = input.Color;
	output.UV = input.UV;
	output.Pos = mul(ubo.projection, mul(ubo.view, mul(primitive.model, float4(input.Pos.xyz, 1.0))));

	float4 pos = mul(ubo.view, float4(input.Pos, 1.0));
	output.Normal = mul((float3x3)ubo.view, input.Normal);
	float3 lPos = mul((float3x3)ubo.view, ubo.lightPos.xyz);
	output.LightVec = lPos - pos.xyz;
	output.ViewVec = -pos.xyz;
	return output;
}