// Copyright 2020 Google LLC

Texture2D textureColorMap : register(t1);
SamplerState samplerColorMap : register(s1);

struct VSOutput
{
[[vk::location(0)]] float3 Normal : NORMAL0;
[[vk::location(1)]] float3 Color : COLOR0;
[[vk::location(2)]] float2 UV : TEXCOORD0;
[[vk::location(3)]] float3 ViewVec : TEXCOORD1;
[[vk::location(4)]] float3 LightVec : TEXCOORD2;
};

float4 main(VSOutput input) : SV_TARGET
{
	float4 color = textureColorMap.Sample(samplerColorMap, input.UV) * float4(input.Color, 1.0);

	float3 N = normalize(input.Normal);
	float3 L = normalize(input.LightVec);
	float3 V = normalize(input.ViewVec);
	float3 R = reflect(-L, N);
	float3 diffuse = max(dot(N, L), 0.0) * input.Color;
	float3 specular = pow(max(dot(R, V), 0.0), 16.0) * float3(0.75, 0.75, 0.75);
	return float4(diffuse * color.rgb + specular, 1.0);
}