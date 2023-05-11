// Copyright 2020 Google LLC

struct VSInput
{
[[vk::location(0)]] float3 Pos : POSITION0;
[[vk::location(1)]] float2 UV : TEXCOORD0;
};

struct Instance
{
	float4x4 model;
	float4 arrayIndex;
};

struct UBO
{
	float4x4 projection;
	float4x4 view;
	Instance instance[8];
};

cbuffer ubo : register(b0) { UBO ubo; }

struct VSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float3 UV : TEXCOORD0;
};

VSOutput main(VSInput input, uint InstanceIndex : SV_InstanceID)
{
	VSOutput output = (VSOutput)0;
	output.UV = float3(input.UV, ubo.instance[InstanceIndex].arrayIndex.x);
	float4x4 modelView = mul(ubo.view, ubo.instance[InstanceIndex].model);
	output.Pos = mul(ubo.projection, mul(modelView, float4(input.Pos, 1.0)));
	return output;
}
