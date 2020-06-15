// Copyright 2020 Google LLC

#define lightCount 6

struct VSOutput
{
[[vk::location(0)]] float3 Normal : NORMAL0;
[[vk::location(2)]] float3 Color : COLOR0;
[[vk::location(3)]] float4 LightVec[lightCount] : TEXCOORD1;
};

#define MAX_LIGHT_DIST 9.0 * 9.0

float4 main(VSOutput input) : SV_TARGET
{
	float3 lightColor[lightCount];
	lightColor[0] = float3(1.0, 0.0, 0.0);
	lightColor[1] = float3(0.0, 1.0, 0.0);
	lightColor[2] = float3(0.0, 0.0, 1.0);
	lightColor[3] = float3(1.0, 0.0, 1.0);
	lightColor[4] = float3(0.0, 1.0, 1.0);
	lightColor[5] = float3(1.0, 1.0, 0.0);

	float3 diffuse = float3(0.0, 0.0, 0.0);
	// Just some very basic attenuation
	for (int i = 0; i < lightCount; ++i)
	{
		float lRadius =  MAX_LIGHT_DIST * input.LightVec[i].w;

		float dist = min(dot(input.LightVec[i], input.LightVec[i]), lRadius) / lRadius;
		float distFactor = 1.0 - dist;

		diffuse += lightColor[i] * distFactor;
	}

	return float4(diffuse, 1);
}