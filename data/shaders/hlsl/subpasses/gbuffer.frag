// Copyright 2020 Google LLC

struct VSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float3 Normal : NORMAL0;
[[vk::location(1)]] float3 Color : COLOR0;
[[vk::location(2)]] float3 WorldPos : POSITION0;
};

struct FSOutput
{
[[vk::location(0)]] float4 Color : SV_TARGET0;
[[vk::location(1)]] float4 Position : SV_TARGET1;
[[vk::location(2)]] float4 Normal : SV_TARGET2;
[[vk::location(3)]] float4 Albedo : SV_TARGET3;
};

[[vk::constant_id(0)]] const float NEAR_PLANE = 0.1f;
[[vk::constant_id(1)]] const float FAR_PLANE = 256.0f;

float linearDepth(float depth)
{
	float z = depth * 2.0f - 1.0f;
	return (2.0f * NEAR_PLANE * FAR_PLANE) / (FAR_PLANE + NEAR_PLANE - z * (FAR_PLANE - NEAR_PLANE));
}

FSOutput main(VSOutput input)
{
	FSOutput output = (FSOutput)0;
	output.Position = float4(input.WorldPos, 1.0);

	float3 N = normalize(input.Normal);
	N.y = -N.y;
	output.Normal = float4(N, 1.0);

	output.Albedo.rgb = input.Color;

	// Store linearized depth in alpha component
	output.Position.a = linearDepth(input.Pos.z);

	// Write color attachments to avoid undefined behaviour (validation error)
	output.Color = float4(0.0, 0.0, 0.0, 0.0);
	return output;
}