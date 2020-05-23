// Copyright 2020 Google LLC

struct UBO
{
	float4x4 projection;
	float4x4 model;
	float tessAlpha;
};

cbuffer ubo : register(b1) { UBO ubo; }

struct HSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float3 Normal : NORMAL0;
[[vk::location(1)]] float2 UV : TEXCOORD0;
};

struct ConstantsHSOutput
{
    float TessLevelOuter[3] : SV_TessFactor;
    float TessLevelInner : SV_InsideTessFactor;
};

struct DSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float3 Normal : NORMAL0;
[[vk::location(1)]] float2 UV : TEXCOORD0;
};

[domain("tri")]
DSOutput main(ConstantsHSOutput input, float3 TessCoord : SV_DomainLocation, const OutputPatch<HSOutput, 3> patch)
{
	DSOutput output = (DSOutput)0;
    output.Pos = (TessCoord.x * patch[0].Pos) +
                  (TessCoord.y * patch[1].Pos) +
                  (TessCoord.z * patch[2].Pos);
	output.Pos = mul(ubo.projection, mul(ubo.model, output.Pos));

	output.Normal = TessCoord.x*patch[0].Normal + TessCoord.y*patch[1].Normal + TessCoord.z*patch[2].Normal;
	output.UV = TessCoord.x*patch[0].UV + TessCoord.y*patch[1].UV + TessCoord.z*patch[2].UV;
	return output;
}