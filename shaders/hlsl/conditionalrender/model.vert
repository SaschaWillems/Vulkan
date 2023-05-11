// Copyright 2020 Google LLC

struct VSInput
{
[[vk::location(0)]] float3 Pos : POSITION0;
[[vk::location(1)]] float3 Normal : NORMAL0;
[[vk::location(2)]] float3 Color : COLOR0;
};

struct UBO
{
	float4x4 projection;
	float4x4 view;
	float4x4 model;
};

cbuffer ubo : register(b0) { UBO ubo; }

struct Node
{
	float4x4 transform;
};

cbuffer NodeBuf : register(b0, space1) { Node node; }

struct PushConstant
{
	float4 baseColorFactor;
};

[[vk::push_constant]] PushConstant material;

struct VSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float3 Normal : NORMAL0;
[[vk::location(1)]] float3 Color : COLOR0;
[[vk::location(2)]] float3 ViewVec : TEXCOORD1;
[[vk::location(3)]] float3 LightVec : TEXCOORD2;
};

VSOutput main(VSInput input)
{
	VSOutput output = (VSOutput)0;
	output.Normal = input.Normal;
	output.Color = material.baseColorFactor.rgb;
	float4 pos = float4(input.Pos, 1.0);
	output.Pos = mul(ubo.projection, mul(ubo.view, mul(ubo.model, mul(node.transform, pos))));

	output.Normal = mul((float4x3)mul(ubo.view, mul(ubo.model, node.transform)), input.Normal).xyz;

	float4 localpos = mul(ubo.view, mul(ubo.model, mul(node.transform, pos)));
	float3 lightPos = float3(10.0f, -10.0f, 10.0f);
	output.LightVec = lightPos.xyz - localpos.xyz;
	output.ViewVec = -localpos.xyz;
	return output;
}
