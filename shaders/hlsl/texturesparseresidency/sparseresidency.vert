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
	float4x4 model;
	float4 viewPos;
	float lodBias;
};

cbuffer ubo : register(b0) { UBO ubo; }

struct VSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float2 UV : TEXCOORD0;
[[vk::location(1)]] float LodBias : TEXCOORD3;
[[vk::location(2)]] float3 Normal : NORMAL0;
[[vk::location(3)]] float3 ViewVec : TEXCOORD1;
[[vk::location(4)]] float3 LightVec : TEXCOORD2;
};

VSOutput main(VSInput input)
{
	VSOutput output = (VSOutput)0;
	output.UV = input.UV;
	output.LodBias = ubo.lodBias;
	output.Normal = input.Normal;

	float3 worldPos = mul(ubo.model, float4(input.Pos, 1.0)).xyz;

	output.Pos = mul(ubo.projection, mul(ubo.model, float4(input.Pos.xyz, 1.0)));

	float3 lightPos = float3(0.0, 50.0f, 0.0f);
	output.LightVec = lightPos - input.Pos.xyz;
	output.ViewVec = ubo.viewPos.xyz - worldPos.xyz;
	return output;
}
