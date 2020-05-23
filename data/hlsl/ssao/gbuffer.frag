// Copyright 2020 Google LLC

struct VSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float3 Normal : NORMAL0;
[[vk::location(1)]] float2 UV : TEXCOORD0;
[[vk::location(2)]] float3 Color : COLOR0;
[[vk::location(3)]] float3 WorldPos : POSITION0;
};

struct FSOutput
{
	float4 Position : SV_TARGET0;
	float4 Normal : SV_TARGET1;
	float4 Albedo : SV_TARGET2;
};
static const float NEAR_PLANE = 0.1f; //todo: specialization const
static const float FAR_PLANE = 64.0f; //todo: specialization const

float linearDepth(float depth)
{
	float z = depth * 2.0f - 1.0f;
	return (2.0f * NEAR_PLANE * FAR_PLANE) / (FAR_PLANE + NEAR_PLANE - z * (FAR_PLANE - NEAR_PLANE));
}

FSOutput main(VSOutput input)
{
	FSOutput output = (FSOutput)0;
	output.Position = float4(input.WorldPos, linearDepth(input.Pos.z));
	output.Normal = float4(normalize(input.Normal) * 0.5 + 0.5, 1.0);
	output.Albedo = float4(input.Color * 2.0, 1.0);
	return output;
}