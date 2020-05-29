// Copyright 2020 Google LLC

struct UBO
{
	float4x4 projection;
	float4x4 model;
	float4 lightPos;
	float tessAlpha;
	float tessStrength;
};

cbuffer ubo : register(b1) { UBO ubo; }

Texture2D textureDisplacementMap : register(t2);
SamplerState samplerDisplacementMap : register(s2);

struct HSOutput
{
[[vk::location(2)]]	float4 Pos : POSITION;
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
[[vk::location(2)]] float3 EyesPos : POSITION0;
[[vk::location(3)]] float3 LightVec : TEXCOORD1;
};

[domain("tri")]
DSOutput main(ConstantsHSOutput input, float3 TessCoord : SV_DomainLocation, const OutputPatch<HSOutput, 3> patch)
{
	DSOutput output = (DSOutput)0;
	output.Pos = (TessCoord.x * patch[0].Pos) + (TessCoord.y * patch[1].Pos) + (TessCoord.z * patch[2].Pos);
	output.UV = mul(TessCoord.x, patch[0].UV) + mul(TessCoord.y, patch[1].UV) + mul(TessCoord.z, patch[2].UV);
	output.Normal = TessCoord.x * patch[0].Normal + TessCoord.y * patch[1].Normal + TessCoord.z * patch[2].Normal;

	output.Pos.xyz += normalize(output.Normal) * (max(textureDisplacementMap.SampleLevel(samplerDisplacementMap, output.UV.xy, 0).a, 0.0) * ubo.tessStrength);

	output.EyesPos = output.Pos.xyz;
	output.LightVec = normalize(ubo.lightPos.xyz - output.EyesPos);

	output.Pos = mul(ubo.projection, mul(ubo.model, output.Pos));
	return output;
}