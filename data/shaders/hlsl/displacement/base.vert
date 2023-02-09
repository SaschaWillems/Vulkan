// Copyright 2020 Google LLC

struct VSInput
{
[[vk::location(0)]] float3 Pos : POSITION0;
[[vk::location(1)]] float3 Normal : NORMAL0;
[[vk::location(2)]] float2 UV : TEXCOORD0;
};

struct VSOutput
{
[[vk::location(2)]]	float4 Pos : POSITION;
[[vk::location(0)]] float3 Normal : NORMAL0;
[[vk::location(1)]] float2 UV : TEXCOORD0;
};

VSOutput main(VSInput input)
{
	VSOutput output = (VSOutput)0;
	output.Pos = float4(input.Pos.xyz, 1.0);
    output.UV = input.UV;
	output.Normal = input.Normal;
	return output;
}