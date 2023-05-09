// Copyright 2020 Google LLC

struct VSInput
{
[[vk::location(0)]] float3 Pos : POSITION0;
[[vk::location(1)]] float3 Normal : NORMAL0;
[[vk::location(2)]] float3 Color : COLOR0;
};

struct UBO
{
	float4x4 projection;
	float4x4 model;
	float4x4 view;
};

cbuffer ubo : register(b0) { UBO ubo; }

struct PushConsts {
	float4 color;
	float4 position;
};
[[vk::push_constant]] PushConsts pushConsts;

struct VSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float3 Color : COLOR0;
};

VSOutput main(VSInput input)
{
	VSOutput output = (VSOutput)0;
	output.Color = input.Color * pushConsts.color.rgb;
	
	float3 locPos = float3(mul(ubo.model, float4(input.Pos.xyz, 1.0)).xyz);
	float3 worldPos = locPos + pushConsts.position.xyz;
	output.Pos = mul(ubo.projection, mul(ubo.view, float4(worldPos.xyz, 1.0)));
	
	return output;
}