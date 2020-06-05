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
	float4x4 normal;
	float4x4 view;
	float3 lightpos;
};

cbuffer ubo : register(b0) { UBO ubo; }

struct VSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float3 Normal : NORMAL0;
[[vk::location(1)]] float3 Color : COLOR0;
[[vk::location(2)]] float3 EyePos : POSITION0;
[[vk::location(3)]] float3 LightVec : TEXCOORD2;
};

VSOutput main(VSInput input)
{
	VSOutput output = (VSOutput)0;
	output.Normal = normalize(mul((float4x3)ubo.normal, input.Normal).xyz);
	output.Color = input.Color;
	float4x4 modelView = mul(ubo.view, ubo.model);
	float4 pos = mul(modelView, input.Pos);
	output.EyePos = mul(modelView, pos).xyz;
	float4 lightPos = mul(float4(ubo.lightpos, 1.0), modelView);
	output.LightVec = normalize(lightPos.xyz - output.EyePos);
	output.Pos = mul(ubo.projection, pos);
	return output;
}