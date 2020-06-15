// Copyright 2020 Google LLC

struct VSInput
{
[[vk::location(0)]] float3 Pos : POSITION0;
[[vk::location(1)]] float2 UV : TEXCOORD0;
[[vk::location(2)]] float3 Normal : NORMAL0;
[[vk::location(3)]] float3 Tangent : TEXCOORD1;
[[vk::location(4)]] float3 BiTangent : TEXCOORD2;
};

struct UBO
{
	float4x4 projection;
	float4x4 view;
	float4x4 model;
	float4 lightPos;
	float4 cameraPos;
};

cbuffer ubo : register(b0) { UBO ubo; }

struct VSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float2 UV : TEXCOORD0;
[[vk::location(1)]] float3 TangentLightPos : TEXCOORD1;
[[vk::location(2)]] float3 TangentViewPos : TEXCOORD2;
[[vk::location(3)]] float3 TangentFragPos : TEXCOORD3;
};

VSOutput main(VSInput input)
{
	VSOutput output = (VSOutput)0;
	output.Pos = mul(ubo.projection, mul(ubo.view, mul(ubo.model, float4(input.Pos, 1.0f))));
	output.TangentFragPos = mul(ubo.model, float4(input.Pos, 1.0)).xyz;
	output.UV = input.UV;

	float3 T = normalize(mul((float3x3)ubo.model, input.Tangent));
	float3 B = normalize(mul((float3x3)ubo.model, input.BiTangent));
	float3 N = normalize(mul((float3x3)ubo.model, input.Normal));
	float3x3 TBN = transpose(float3x3(T, B, N));

	output.TangentLightPos = mul(TBN, ubo.lightPos.xyz);
	output.TangentViewPos  = mul(TBN, ubo.cameraPos.xyz);
	output.TangentFragPos  = mul(TBN, output.TangentFragPos);
	return output;
}
