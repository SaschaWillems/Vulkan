// Copyright 2021 Sascha Willems

struct VSInput
{
[[vk::location(0)]] float3 Pos : POSITION0;
[[vk::location(1)]] float2 UV : TEXCOORD0;
[[vk::location(2)]] int TextureIndex : TEXTUREINDEX0;
};

struct Matrices {
	float4x4 projection;
	float4x4 view;
	float4x4 model;
};

cbuffer matrices : register(b0) { Matrices matrices; };

struct VSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] int TextureIndex : TEXTUREINDEX0;
[[vk::location(1)]] float2 UV : TEXCOORD0;
};

VSOutput main(VSInput input)
{
	VSOutput output = (VSOutput)0;
	output.UV = input.UV;
	output.TextureIndex = input.TextureIndex;
	output.Pos = mul(matrices.projection, mul(matrices.view, mul(matrices.model, float4(input.Pos.xyz, 1.0))));
	return output;
}