// Copyright 2020 Google LLC

Texture2D textureHeight : register(t1);
SamplerState samplerHeight : register(s1);
Texture2DArray textureLayers : register(t2);
SamplerState samplerLayers : register(s2);

struct DSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float3 Normal : NORMAL0;
[[vk::location(1)]] float2 UV : TEXCOORD0;
[[vk::location(2)]] float3 ViewVec : TEXCOORD1;
[[vk::location(3)]] float3 LightVec : TEXCOORD2;
[[vk::location(4)]] float3 EyePos : POSITION1;
[[vk::location(5)]] float3 WorldPos : POSITION0;
};

float3 sampleTerrainLayer(float2 inUV)
{
	// Define some layer ranges for sampling depending on terrain height
	float2 layers[6];
	layers[0] = float2(-10.0, 10.0);
	layers[1] = float2(5.0, 45.0);
	layers[2] = float2(45.0, 80.0);
	layers[3] = float2(75.0, 100.0);
	layers[4] = float2(95.0, 140.0);
	layers[5] = float2(140.0, 190.0);

	float3 color = float3(0.0, 0.0, 0.0);

	// Get height from displacement map
	float height = textureHeight.SampleLevel(samplerHeight, inUV, 0.0).r * 255.0;

	for (int i = 0; i < 6; i++)
	{
		float range = layers[i].y - layers[i].x;
		float weight = (range - abs(height - layers[i].y)) / range;
		weight = max(0.0, weight);
		color += weight * textureLayers.Sample(samplerLayers, float3(inUV * 16.0, i)).rgb;
	}

	return color;
}

float fog(float density, float4 FragCoord)
{
	const float LOG2 = -1.442695;
	float dist = FragCoord.z / FragCoord.w * 0.1;
	float d = density * dist;
	return 1.0 - clamp(exp2(d * d * LOG2), 0.0, 1.0);
}

float4 main(DSOutput input) : SV_TARGET
{
	float3 N = normalize(input.Normal);
	float3 L = normalize(input.LightVec);
	float3 ambient = float3(0.5, 0.5, 0.5);
	float3 diffuse = max(dot(N, L), 0.0) * float3(1.0, 1.0, 1.0);

	float4 color = float4((ambient + diffuse) * sampleTerrainLayer(input.UV), 1.0);

	const float4 fogColor = float4(0.47, 0.5, 0.67, 0.0);
	return lerp(color, fogColor, fog(0.25, input.Pos));
}
