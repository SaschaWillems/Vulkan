// Copyright 2020 Google LLC

struct VSInput
{
	[[vk::location(0)]]float4 Pos : POSITION0;
	[[vk::location(1)]]float2 UV : TEXCOORD0;
	[[vk::location(2)]]float3 Color : COLOR0;
};

struct VSOutput
{
	float4 Pos : SV_POSITION;
	[[vk::location(0)]]float3 Color : COLOR0;
	[[vk::location(1)]]float2 UV : TEXCOORD0;
};

cbuffer UBO : register(b0)
{
	float4x4 projection;
	float4x4 view;
	float4x4 model;
};

VSOutput main(VSInput input)
{
	VSOutput output = (VSOutput)0;
	output.UV = input.UV;
	output.Color = input.Color;
	output.Pos = mul(projection, mul(view, mul(model, input.Pos)));
	return output;
}
