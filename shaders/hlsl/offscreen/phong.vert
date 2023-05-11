// Copyright 2020 Google LLC

struct VSInput
{
[[vk::location(0)]] float4 Pos : POSITION0;
[[vk::location(1)]] float3 Color : COLOR0;
[[vk::location(2)]] float3 Normal : NORMAL0;
};

struct UBO
{
	float4x4 projection;
	float4x4 view;
	float4x4 model;
	float4 lightPos;
};

cbuffer ubo : register(b0) { UBO ubo; }

struct VSOutput
{
	float4 Pos : SV_POSITION;
	float ClipDistance : SV_ClipDistance0;
[[vk::location(0)]] float3 Normal : NORMAL0;
[[vk::location(1)]] float3 Color : COLOR0;
[[vk::location(2)]] float3 EyePos : POSITION0;
[[vk::location(3)]] float3 LightVec : TEXCOORD2;
};

VSOutput main(VSInput input)
{
	VSOutput output = (VSOutput)0;
	output.Normal = input.Normal;
	output.Color = input.Color;
	output.Pos = mul(ubo.projection, mul(ubo.view, mul(ubo.model, input.Pos)));
	output.EyePos = mul(ubo.view, mul(ubo.model, input.Pos)).xyz;
	output.LightVec = normalize(ubo.lightPos.xyz - output.EyePos);

	// Clip against reflection plane
	float4 clipPlane = float4(0.0, -1.0, 0.0, 1.5);
	output.ClipDistance = dot(input.Pos, clipPlane);
	return output;
}
