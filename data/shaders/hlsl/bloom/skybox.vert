// Copyright 2020 Google LLC

cbuffer UBO : register(b0)
{
	float4x4 projection;
	float4x4 view;
	float4x4 model;
};

struct VSOutput
{
	float4 Pos : SV_POSITION;
	[[vk::location(0)]] float3 UVW : NORMAL0;
};

VSOutput main([[vk::location(0)]] float3 inPos : POSITION0)
{
	VSOutput output = (VSOutput)0;
	output.UVW = inPos;
	output.Pos = mul(projection, mul(view, mul(model, float4(inPos.xyz, 1.0))));
	return output;
}
