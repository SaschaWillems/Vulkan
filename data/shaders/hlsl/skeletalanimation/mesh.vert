// Copyright 2020 Google LLC

struct VSInput
{
[[vk::location(0)]] float3 Pos : POSITION0;
[[vk::location(1)]] float3 Normal : NORMAL0;
[[vk::location(2)]] float2 UV : TEXCOORD0;
[[vk::location(3)]] float3 Color : COLOR0;
[[vk::location(4)]] float4 BoneWeights : TEXCOORD1;
[[vk::location(5)]] int4 BoneIDs : TEXCOORD2;
};

#define MAX_BONES 64

struct UBO
{
	float4x4 projection;
	float4x4 view;
	float4x4 model;
	float4x4 bones[MAX_BONES];
	float4 lightPos;
	float4 viewPos;
};

cbuffer ubo : register(b0) { UBO ubo; }

struct VSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float3 Normal : NORMAL0;
[[vk::location(1)]] float3 Color : COLOR0;
[[vk::location(2)]] float2 UV : TEXCOORD0;
[[vk::location(3)]] float3 ViewVec : TEXCOORD1;
[[vk::location(4)]] float3 LightVec : TEXCOORD2;
};

VSOutput main(VSInput input)
{
	VSOutput output = (VSOutput)0;
	float4x4 boneTransform = ubo.bones[input.BoneIDs[0]] * input.BoneWeights[0];
	boneTransform     += ubo.bones[input.BoneIDs[1]] * input.BoneWeights[1];
	boneTransform     += ubo.bones[input.BoneIDs[2]] * input.BoneWeights[2];
	boneTransform     += ubo.bones[input.BoneIDs[3]] * input.BoneWeights[3];

	output.Color = input.Color;
	output.UV = input.UV;

	output.Pos = mul(ubo.projection, mul(ubo.view, mul(ubo.model, mul(boneTransform, float4(input.Pos.xyz, 1.0)))));

	float4 pos = mul(ubo.model, float4(input.Pos, 1.0));
	output.Normal = mul((float3x3)(boneTransform), input.Normal);
	output.LightVec = ubo.lightPos.xyz - pos.xyz;
	output.ViewVec = ubo.viewPos.xyz - pos.xyz;
	return output;
}