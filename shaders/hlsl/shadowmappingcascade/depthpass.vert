// Copyright 2020 Google LLC

struct VSInput
{
[[vk::location(0)]] float3 Pos : POSITION0;
[[vk::location(1)]] float2 UV : TEXCOORD0;
};
// todo: pass via specialization constant
#define SHADOW_MAP_CASCADE_COUNT 4

struct PushConsts {
	float4 position;
	uint cascadeIndex;
};
[[vk::push_constant]] PushConsts pushConsts;

struct UBO  {
	float4x4 cascadeViewProjMat[SHADOW_MAP_CASCADE_COUNT];
};

cbuffer ubo : register(b0) { UBO ubo; }

struct VSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float2 UV : TEXCOORD0;
};

VSOutput main(VSInput input)
{
	VSOutput output = (VSOutput)0;
	output.UV = input.UV;
	float3 pos = input.Pos + pushConsts.position.xyz;
	output.Pos = mul(ubo.cascadeViewProjMat[pushConsts.cascadeIndex], float4(pos, 1.0));
	return output;
}