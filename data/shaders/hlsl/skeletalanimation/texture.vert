// Copyright 2020 Google LLC

struct VSInput
{
[[vk::location(0)]] float3 Pos : POSITION0;
[[vk::location(1)]] float3 Normal : NORMAL0;
[[vk::location(2)]] float2 UV : TEXCOORD0;
};

struct UBO
{
	float4x4 projection;
	float4x4 view;
	float4x4 model;
	float4 lightPos;
	float4 viewPos;
	float2 uvOffset;
};

cbuffer ubo : register(b0) { UBO ubo; }

struct VSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float2 UV : TEXCOORD0;
[[vk::location(1)]] float3 Normal : NORMAL0;
[[vk::location(2)]] float3 ViewVec : TEXCOORD1;
[[vk::location(3)]] float3 LightVec : TEXCOORD2;
};

VSOutput main(VSInput input)
{
	VSOutput output = (VSOutput)0;
	output.UV = input.UV + ubo.uvOffset;
	float4 pos = float4(input.Pos, 1.0);
	output.Pos = mul(ubo.projection, mul(ubo.view, mul(ubo.model, pos)));

	output.Normal = input.Normal;
	output.LightVec = ubo.lightPos.xyz - pos.xyz;
	output.ViewVec = ubo.viewPos.xyz - pos.xyz;
	return output;
}
