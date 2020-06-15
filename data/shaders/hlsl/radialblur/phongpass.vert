// Copyright 2020 Google LLC

struct VSInput
{
[[vk::location(0)]] float4 Pos : POSITION0;
[[vk::location(2)]] float3 Color : COLOR0;
[[vk::location(3)]] float3 Normal : NORMAL0;
};

struct UBO
{
	float4x4 projection;
	float4x4 model;
	float gradientPos;
};

cbuffer ubo : register(b0) { UBO ubo; }

struct VSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float3 Normal : NORMAL0;
[[vk::location(1)]] float3 Color : COLOR0;
[[vk::location(2)]] float3 EyePos : POSITION0;
[[vk::location(3)]] float3 LightVec : TEXCOORD2;
[[vk::location(4)]] float2 UV : TEXCOORD0;
};

VSOutput main(VSInput input)
{
	VSOutput output = (VSOutput)0;
	output.Normal = input.Normal;
	output.Color = input.Color;
	output.UV = float2(ubo.gradientPos, 0.0);
	output.Pos = mul(ubo.projection, mul(ubo.model, input.Pos));
	output.EyePos = mul(ubo.model, input.Pos).xyz;
	float4 lightPos = float4(0.0, 0.0, -5.0, 1.0);// * ubo.model;
	output.LightVec = normalize(lightPos.xyz - input.Pos.xyz);
	return output;
}
