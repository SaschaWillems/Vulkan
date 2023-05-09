// Copyright 2020 Google LLC

struct VSInput
{
[[vk::location(0)]] float4 Pos : POSITION0;
[[vk::location(1)]] float3 Normal : NORMAL0;
[[vk::location(2)]] float2 TexCoord : TEXCOORD0;
[[vk::location(3)]] float3 Color : COLOR0;
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
[[vk::location(0)]] float2 UV : TEXCOORD0;
[[vk::location(1)]] float3 Normal : NORMAL0;
[[vk::location(2)]] float3 Color : COLOR0;
[[vk::location(3)]] float3 EyePos : POSITION0;
[[vk::location(4)]] float3 LightVec : TEXCOORD2;
};

VSOutput main(VSInput input)
{
	VSOutput output = (VSOutput)0;
	float4x4 modelView = mul(ubo.view, ubo.model);
	float4 pos = mul(modelView, input.Pos);
	output.UV = input.TexCoord.xy;
	output.Normal = normalize(mul((float3x3)ubo.normal, input.Normal));
	output.Color = input.Color;
	output.Pos = mul(ubo.projection, pos);
	output.EyePos = mul(modelView, pos).xyz;
	float4 lightPos = mul(modelView, float4(1.0, 2.0, 0.0, 1.0));
	output.LightVec = normalize(lightPos.xyz - output.EyePos);
	return output;
}
