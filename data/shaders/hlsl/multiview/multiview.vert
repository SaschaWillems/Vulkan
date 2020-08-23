// Copyright 2020 Google LLC

struct VSInput
{
[[vk::location(0)]] float3 Pos : POSITION0;
[[vk::location(1)]] float3 Normal : NORMAL0;
[[vk::location(2)]] float3 Color : COLOR0;
};

struct VSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float3 Normal : NORMAL0;
[[vk::location(1)]] float3 Color : COLOR0;
[[vk::location(2)]] float3 ViewVec : TEXCOORD1;
[[vk::location(3)]] float3 LightVec : TEXCOORD2;
};

struct UBO
{
	float4x4 projection[2];
	float4x4 modelview[2];
	float4 lightPos;
};

cbuffer ubo : register(b0) { UBO ubo; }

VSOutput main(VSInput input, uint ViewIndex : SV_ViewID)
{
	VSOutput output = (VSOutput)0;
	output.Color = input.Color;
	output.Normal = mul((float3x3)ubo.modelview[ViewIndex], input.Normal);

	float4 pos = float4(input.Pos.xyz, 1.0);
	float4 worldPos = mul(ubo.modelview[ViewIndex], pos);

	float3 lPos = mul(ubo.modelview[ViewIndex], ubo.lightPos).xyz;
	output.LightVec = lPos - worldPos.xyz;
	output.ViewVec = -worldPos.xyz;

	output.Pos = mul(ubo.projection[ViewIndex], worldPos);
	return output;
}
