// Copyright 2020 Google LLC

Texture2D colorMapTexture : register(t1);
SamplerState colorMapSampler : register(s1);

struct VSOutput
{
[[vk::location(0)]] float3 Normal : NORMAL0;
[[vk::location(1)]] float2 UV : TEXCOORD0;
[[vk::location(2)]] float3 Color : COLOR0;
[[vk::location(3)]] float3 ViewVec : TEXCOORD1;
[[vk::location(4)]] float3 LightVec : TEXCOORD2;
};

float4 main(VSOutput input) : SV_TARGET
{
	float3 ambient = float3(0.0f, 0.0f, 0.0f);

	// Adjust light calculations for glow color
	if ((input.Color.r >= 0.9) || (input.Color.g >= 0.9) || (input.Color.b >= 0.9))
	{
		ambient = input.Color * 0.25;
	}

	float3 N = normalize(input.Normal);
	float3 L = normalize(input.LightVec);
	float3 V = normalize(input.ViewVec);
	float3 R = reflect(-L, N);
	float3 diffuse = max(dot(N, L), 0.0) * input.Color;
	float3 specular = pow(max(dot(R, V), 0.0), 8.0) * float3(0.75f, 0.75f, 0.75f);
	return float4(ambient + diffuse + specular, 1.0);
}