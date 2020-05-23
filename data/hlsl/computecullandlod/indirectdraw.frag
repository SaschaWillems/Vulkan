// Copyright 2020 Google LLC

struct VSOutput
{
[[vk::location(0)]] float3 Normal : NORMAL0;
[[vk::location(1)]] float3 Color : COLOR0;
[[vk::location(2)]] float3 ViewVec : TEXCOORD1;
[[vk::location(3)]] float3 LightVec : TEXCOORD2;
};

float4 main(VSOutput input) : SV_TARGET
{
	float3 N = normalize(input.Normal);
	float3 L = normalize(input.LightVec);
	float3 ambient = float3(0.25, 0.25, 0.25);
	float3 diffuse = max(dot(N, L), 0.0).xxx;
	return float4((ambient + diffuse) * input.Color, 1.0);
}
