// Copyright 2020 Google LLC

struct VSInput
{
[[vk::location(0)]] float4 Pos : POSITION0;
[[vk::location(1)]] float2 UV : TEXCOORD0;
[[vk::location(2)]] float3 Color : COLOR0;
[[vk::location(3)]] float3 Normal : NORMAL0;
};

struct VSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float3 Normal : NORMAL0;
[[vk::location(1)]] float2 UV : TEXCOORD0;
[[vk::location(2)]] float3 Color : COLOR0;
[[vk::location(3)]] float3 ViewVec : TEXCOORD1;
[[vk::location(4)]] float3 LightVec : TEXCOORD2;
};

cbuffer UBO : register(b0)
{
	float4x4 projection;
	float4x4 view;
	float4x4 model;
};

VSOutput main(VSInput input)
{
	VSOutput output = (VSOutput)0;
	output.Normal = input.Normal;
	output.Color = input.Color;
	output.UV = input.UV;
	output.Pos = mul(projection, mul(view, mul(model, input.Pos)));

	float3 lightPos = float3(-5.0, -5.0, 0.0);
	float4 pos = mul(view, mul(model, input.Pos));
	output.Normal = mul((float4x3)mul(view, model), input.Normal).xyz;
	output.LightVec = lightPos - pos.xyz;
	output.ViewVec = -pos.xyz;
	return output;
}
