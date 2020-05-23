// Copyright 2020 Google LLC

struct VSInput
{
[[vk::location(0)]] float3 Pos : POSITION0;
[[vk::location(1)]] float3 Normal : NORMAL0;
};

struct UBO
{
	float4x4 projection;
	float4x4 model;
	float4x4 view;
	float3 camPos;
};
cbuffer ubo : register(b0) { UBO ubo; };

struct VSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float3 WorldPos : POSITION0;
[[vk::location(1)]] float3 Normal : NORMAL0;
};

struct PushConsts {
	float3 objPos;
};
[[vk::push_constant]] PushConsts pushConsts;

VSOutput main(VSInput input)
{
	VSOutput output = (VSOutput)0;
	float3 locPos = mul(ubo.model, float4(input.Pos, 1.0)).xyz;
	output.WorldPos = locPos + pushConsts.objPos;
	output.Normal = mul((float4x3)ubo.model, input.Normal).xyz;
	output.Pos =  mul(ubo.projection, mul(ubo.view, float4(output.WorldPos, 1.0)));
	return output;
}
