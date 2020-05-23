// Copyright 2020 Google LLC

Texture2D textureColor : register(t1);
SamplerState samplerColor : register(s1);

struct VSOutput
{
[[vk::location(0)]] float2 UV : TEXCOORD0;
[[vk::location(1)]] float LodBias : TEXCOORD3;
[[vk::location(2)]] float3 Normal : NORMAL0;
[[vk::location(3)]] float3 ViewVec : TEXCOORD1;
[[vk::location(4)]] float3 LightVec : TEXCOORD2;
};

float4 main(VSOutput input) : SV_TARGET
{
	float4 color = float4(0.0, 0.0, 0.0, 0.0);

	// Fetch sparse until we get a valid texel
	uint status;
	float minLod = input.LodBias;
	do
	{
		color = textureColor.SampleLevel(samplerColor, input.UV, minLod, 0, status);
		minLod += 1.0f;
	} while(!CheckAccessFullyMapped(status));

	float3 N = normalize(input.Normal);

	N = normalize((input.Normal - 0.5) * 2.0);

	float3 L = normalize(input.LightVec);
	float3 R = reflect(-L, N);
	float3 diffuse = max(dot(N, L), 0.25) * color.rgb;
	return float4(diffuse, 1.0);
}