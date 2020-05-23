// Copyright 2020 Google LLC

Texture2D textureColor : register(t1);
SamplerState samplers[3] : register(s2);

struct VSOutput
{
[[vk::location(0)]] float2 UV : TEXCOORD0;
[[vk::location(1)]] float LodBias : TEXCOORD3;
[[vk::location(2)]] int SamplerIndex : TEXCOORD4;
[[vk::location(3)]] float3 Normal : NORMAL0;
[[vk::location(4)]] float3 ViewVec : TEXCOORD1;
[[vk::location(5)]] float3 LightVec : TEXCOORD2;
};

float4 main(VSOutput input) : SV_TARGET
{
	float4 color = textureColor.Sample(samplers[input.SamplerIndex], input.UV, int2(0, 0), input.LodBias);

	float3 N = normalize(input.Normal);
	float3 L = normalize(input.LightVec);
	float3 V = normalize(input.ViewVec);
	float3 R = reflect(L, N);
	float3 diffuse = max(dot(N, L), 0.65) * float3(1.0, 1.0, 1.0);
	float specular = pow(max(dot(R, V), 0.0), 16.0) * color.a;
	return float4(diffuse * color.rgb + specular, 1.0);
}