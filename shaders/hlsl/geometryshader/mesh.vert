// Copyright 2020 Google LLC

struct VSInput
{
[[vk::location(0)]] float4 Pos : POSITION0;
[[vk::location(1)]] float3 Normal : NORMAL0;
[[vk::location(2)]] float3 Color : COLOR0;
};

struct UBO
{
	float4x4 projection;
	float4x4 model;
};
cbuffer ubo : register(b0) { UBO ubo; };

struct VSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float3 Normal : NORMAL0;
[[vk::location(1)]] float3 Color : COLOR0;
[[vk::location(2)]] float3 ViewVec : TEXCOORD1;
[[vk::location(3)]] float3 LightVec : TEXCOORD2;
};

VSOutput main(VSInput input)
{
	VSOutput output = (VSOutput)0;
	output.Normal = input.Normal;
	output.Color = input.Color;
	output.Pos = mul(ubo.projection, mul(ubo.model, input.Pos));

	float4 pos = mul(ubo.model, float4(input.Pos.xyz, 1.0));
	output.Normal = mul((float4x3)ubo.model, input.Normal).xyz;

	float3 lightPos = float3(1.0f, -1.0f, 1.0f);
	output.LightVec = lightPos.xyz - pos.xyz;
	output.ViewVec = -pos.xyz;
	return output;
}
