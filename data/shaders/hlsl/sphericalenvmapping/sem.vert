// Copyright 2020 Google LLC

struct VSInput
{
[[vk::location(0)]] float4 Pos : POSITION0;
[[vk::location(1)]] float3 Normal : NORMAL0;
[[vk::location(2)]] float3 Color : COLOR0;
};

struct UBO
{
	float4x4 projection;
	float4x4 model;
	float4x4 normal;
	float4x4 view;
	int texIndex;
};

cbuffer ubo : register(b0) { UBO ubo; }

struct VSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float3 Color : COLOR0;
[[vk::location(1)]] float3 EyePos : POSITION0;
[[vk::location(2)]] float3 Normal : NORMAL0;
[[vk::location(3)]] int TexIndex : TEXCOORD1;
};

VSOutput main(VSInput input)
{
	VSOutput output = (VSOutput)0;
	output.Color = input.Color;
	float4x4 modelView = mul(ubo.view, ubo.model);
	output.EyePos = normalize( mul(modelView, input.Pos ).xyz );
	output.TexIndex = ubo.texIndex;
	output.Normal = normalize( mul((float3x3)ubo.normal, input.Normal) );
	float3 r = reflect( output.EyePos, output.Normal );
	float m = 2.0 * sqrt( pow(r.x, 2.0) + pow(r.y, 2.0) + pow(r.z + 1.0, 2.0));
	output.Pos = mul(ubo.projection, mul(modelView, input.Pos));
	return output;
}
