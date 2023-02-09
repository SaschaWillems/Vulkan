// Copyright 2020 Google LLC

struct VSInput
{
[[vk::location(0)]] float3 Pos : POSITION0;
[[vk::location(1)]] float3 Color : COLOR0;
};

struct UboView
{
	float4x4 projection;
	float4x4 view;
};
cbuffer uboView : register(b0) { UboView uboView; };

struct UboInstance
{
	float4x4 model;
};
cbuffer uboInstance : register(b1) { UboInstance uboInstance; };

struct VSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float3 Color : COLOR0;
};

VSOutput main(VSInput input)
{
	VSOutput output = (VSOutput)0;
	output.Color = input.Color;
	float4x4 modelView = mul(uboView.view, uboInstance.model);
	float3 worldPos = mul(modelView, float4(input.Pos, 1.0)).xyz;
	output.Pos = mul(uboView.projection, mul(modelView, float4(input.Pos.xyz, 1.0)));
	return output;
}
