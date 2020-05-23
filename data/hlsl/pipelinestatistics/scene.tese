// Copyright 2020 Google LLC

struct HSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float3 Normal : NORMAL0;
[[vk::location(1)]] float3 Color : COLOR0;
[[vk::location(2)]] float3 ViewVec : TEXCOORD1;
[[vk::location(3)]] float3 LightVec : TEXCOORD2;
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
[[vk::location(1)]] float3 Color : COLOR0;
[[vk::location(2)]] float3 ViewVec : TEXCOORD1;
[[vk::location(3)]] float3 LightVec : TEXCOORD2;
};

[domain("tri")]
DSOutput main(ConstantsHSOutput input, float3 TessCoord : SV_DomainLocation, const OutputPatch<HSOutput, 3> patch)
{
	DSOutput output = (DSOutput)0;
	output.Pos = 	(TessCoord.x * patch[2].Pos) +
					(TessCoord.y * patch[1].Pos) +
					(TessCoord.z * patch[0].Pos);
	output.Normal = TessCoord.x * patch[2].Normal + TessCoord.y * patch[1].Normal + TessCoord.z * patch[0].Normal;
	output.ViewVec = TessCoord.x * patch[2].ViewVec + TessCoord.y * patch[1].ViewVec + TessCoord.z * patch[0].ViewVec;
	output.LightVec = TessCoord.x * patch[2].LightVec + TessCoord.y * patch[1].LightVec + TessCoord.z * patch[0].LightVec;
	output.Color = patch[0].Color;
	return output;
}