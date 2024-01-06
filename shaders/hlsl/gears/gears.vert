// Copyright 2020 Google LLC
// Copyright 2023 Sascha Willems

struct VSInput
{
[[vk::location(0)]] float4 Pos : POSITION0;
[[vk::location(1)]] float3 Normal : NORMAL0;
[[vk::location(2)]] float3 Color : COLOR0;
};

struct UBO
{
	float4x4 projection;
	float4x4 view;
	float4 lightpos;
	float4x4 model[3];
};

cbuffer ubo : register(b0) { UBO ubo; }

struct VSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float3 Normal : NORMAL0;
[[vk::location(1)]] float3 Color : COLOR0;
[[vk::location(2)]] float3 EyePos : POSITION0;
[[vk::location(3)]] float3 LightVec : TEXCOORD2;
};

VSOutput main(VSInput input, uint InstanceIndex : SV_InstanceID)
{
	VSOutput output = (VSOutput)0;
	output.Normal = normalize(mul((float3x3)transpose(ubo.model[InstanceIndex]), input.Normal).xyz);
	output.Color = input.Color;
	float4x4 modelView = mul(ubo.view, ubo.model[InstanceIndex]);
	float4 pos = mul(modelView, input.Pos);
	output.EyePos = mul(modelView, pos).xyz;
	float4 lightPos = mul(float4(ubo.lightpos.xyz, 1.0), modelView);
	output.LightVec = normalize(lightPos.xyz - output.EyePos);
	output.Pos = mul(ubo.projection, pos);
	return output;
}