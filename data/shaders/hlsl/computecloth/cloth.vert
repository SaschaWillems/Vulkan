// Copyright 2020 Google LLC

struct VSInput
{
[[vk::location(0)]] float3 Pos : POSITION0;
[[vk::location(1)]] float2 UV : TEXCOORD0;
[[vk::location(2)]] float3 Normal : NORMAL0;
};

struct VSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float2 UV : TEXCOORD0;
[[vk::location(1)]] float3 Normal : NORMAL0;
[[vk::location(2)]] float3 ViewVec : TEXCOORD1;
[[vk::location(3)]] float3 LightVec : TEXCOORD2;
};

struct UBO
{
	float4x4 projection;
	float4x4 modelview;
	float4 lightPos;
};

cbuffer ubo : register(b0)
{
	UBO ubo;
};

VSOutput main (VSInput input)
{
	VSOutput output = (VSOutput)0;
	output.UV = input.UV;
	output.Normal = input.Normal.xyz;
	float4 eyePos = mul(ubo.modelview, float4(input.Pos.x, input.Pos.y, input.Pos.z, 1.0));
	output.Pos = mul(ubo.projection, eyePos);
	float4 pos = float4(input.Pos, 1.0);
	float3 lPos = ubo.lightPos.xyz;
	output.LightVec = lPos - pos.xyz;
	output.ViewVec = -pos.xyz;
	return output;
}