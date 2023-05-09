// Copyright 2020 Google LLC

struct VSInput
{
[[vk::location(0)]] float3 Pos : POSITION0;
[[vk::location(1)]] float3 Normal : NORMAL0;
[[vk::location(2)]] float2 UV : TEXCOORD0;
[[vk::location(3)]] float3 Color : COLOR0;
};

struct UBOScene {
	float4x4 projection;
	float4x4 view;
};
cbuffer uboCamera : register(b0) { UBOScene uboCamera; };

struct UBOModel {
	float4x4 local;
};
cbuffer uboModel : register(b1) { UBOModel uboModel; };

struct VSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float3 Normal : NORMAL0;
[[vk::location(1)]] float3 Color : COLOR0;
[[vk::location(2)]] float2 UV : TEXCOORD0;
};

VSOutput main(VSInput input)
{
	VSOutput output = (VSOutput)0;
	output.Normal = input.Normal;
	output.Color = input.Color;
	output.UV = input.UV;
	output.Pos = mul(uboCamera.projection, mul(uboCamera.view, mul(uboModel.local, float4(input.Pos.xyz, 1.0))));
	return output;
}