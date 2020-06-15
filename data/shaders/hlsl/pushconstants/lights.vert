// Copyright 2020 Google LLC

struct VSInput
{
[[vk::location(0)]] float3 Pos : POSITION0;
[[vk::location(1)]] float3 Normal : NORMAL0;
[[vk::location(3)]] float3 Color : COLOR0;
};

#define lightCount 6

struct UBO
{
	float4x4 projection;
	float4x4 model;
	float4 lightColor[lightCount];
};

cbuffer ubo : register(b0) { UBO ubo; }

struct PushConsts {
	float4 lightPos[lightCount];
};
[[vk::push_constant]] PushConsts pushConsts;

struct VSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float3 Normal : NORMAL0;
[[vk::location(2)]] float3 Color : COLOR0;
[[vk::location(3)]] float4 LightVec[lightCount] : TEXCOORD1;
};

VSOutput main(VSInput input)
{
	VSOutput output = (VSOutput)0;
	output.Normal = input.Normal;
	output.Color = input.Color;

	output.Pos = mul(ubo.projection, mul(ubo.model, float4(input.Pos.xyz, 1.0)));

	for (int i = 0; i < lightCount; ++i)
	{
		float4 worldPos = mul(ubo.model, float4(input.Pos.xyz, 1.0));
		output.LightVec[i].xyz = pushConsts.lightPos[i].xyz - input.Pos.xyz;
		// Store light radius in w
		output.LightVec[i].w = pushConsts.lightPos[i].w;
	}
	return output;
}