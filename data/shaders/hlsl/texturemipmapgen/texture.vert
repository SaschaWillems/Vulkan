// Copyright 2020 Google LLC

struct VSInput
{
[[vk::location(0)]] float3 Pos : POSITION0;
[[vk::location(1)]] float2 UV : TEXCOORD0;
[[vk::location(2)]] float3 Normal : NORMAL0;
};

struct UBO
{
	float4x4 projection;
	float4x4 view;
	float4x4 model;
	float4 viewPos;
	float lodBias;
	int samplerIndex;
};

cbuffer ubo : register(b0) { UBO ubo; }

struct VSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float2 UV : TEXCOORD0;
[[vk::location(1)]] float LodBias : TEXCOORD3;
[[vk::location(2)]] int SamplerIndex : TEXCOORD4;
[[vk::location(3)]] float3 Normal : NORMAL0;
[[vk::location(4)]] float3 ViewVec : TEXCOORD1;
[[vk::location(5)]] float3 LightVec : TEXCOORD2;
};

VSOutput main(VSInput input)
{
	VSOutput output = (VSOutput)0;
	output.UV = input.UV * float2(2.0, 1.0);
	output.LodBias = ubo.lodBias;
	output.SamplerIndex = ubo.samplerIndex;

	float3 worldPos = mul(ubo.model, float4(input.Pos, 1.0)).xyz;

	output.Pos = mul(ubo.projection, mul(ubo.view, mul(ubo.model, float4(input.Pos.xyz, 1.0))));

	output.Normal = mul((float3x3)ubo.model, input.Normal);
	float3 lightPos = float3(-30.0, 0.0, 0.0);
	output.LightVec = worldPos - lightPos;
	output.ViewVec = ubo.viewPos.xyz - worldPos;
	return output;
}
