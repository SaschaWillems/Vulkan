// Copyright 2020 Google LLC

struct VSInput
{
[[vk::location(0)]] float3 Pos : POSITION0;
[[vk::location(1)]] float3 Normal : NORMAL0;
[[vk::location(2)]] float2 UV : TEXCOORD0;
};

struct VSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float2 UV : TEXCOORD0;
};

struct UBO
{
	float4x4 mvp;
};
cbuffer ubo : register(b0) { UBO ubo; };

VSOutput main(VSInput input)
{
	VSOutput output = (VSOutput)0;
	output.Pos = mul(ubo.mvp, float4(input.Pos, 1.0));
	output.UV = input.UV;
	return output;
}
