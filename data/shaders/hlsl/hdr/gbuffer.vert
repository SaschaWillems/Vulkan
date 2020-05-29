// Copyright 2020 Google LLC

struct VSInput
{
[[vk::location(0)]] float3 Pos : POSITION0;
[[vk::location(1)]] float3 Normal : NORMAL0;
};

[[vk::constant_id(0)]] const int type = 0;

struct UBO  {
	float4x4 projection;
	float4x4 modelview;
	float4x4 inverseModelview;
};

cbuffer ubo : register(b0) { UBO ubo; }

struct VSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float3 UVW : TEXCOORD0;
[[vk::location(1)]] float3 WorldPos : POSITION0;
[[vk::location(2)]] float3 Normal : NORMAL0;
[[vk::location(3)]] float3 ViewVec : TEXCOORD1;
[[vk::location(4)]] float3 LightVec : TEXCOORD2;
};

VSOutput main(VSInput input)
{
	VSOutput output = (VSOutput)0;
	output.UVW = input.Pos;

	switch(type) {
		case 0: // Skybox
			output.WorldPos = mul((float4x3)ubo.modelview, input.Pos).xyz;
			output.Pos = mul(ubo.projection, float4(output.WorldPos, 1.0));
			break;
		case 1: // Object
			output.WorldPos = mul(ubo.modelview, float4(input.Pos, 1.0)).xyz;
			output.Pos = mul(ubo.projection, mul(ubo.modelview, float4(input.Pos.xyz, 1.0)));
			break;
	}
	output.WorldPos = mul(ubo.modelview, float4(input.Pos, 1.0)).xyz;
	output.Normal = mul((float4x3)ubo.modelview, input.Normal).xyz;

	float3 lightPos = float3(0.0f, -5.0f, 5.0f);
	output.LightVec = lightPos.xyz - output.WorldPos.xyz;
	output.ViewVec = -output.WorldPos.xyz;
	return output;
}
