// Copyright 2020 Google LLC

struct VSOutput
{
[[vk::location(0)]] float3 Normal : NORMAL0;
[[vk::location(1)]] float3 ViewVec : TEXCOORD0;
[[vk::location(2)]] float3 LightVec : TEXCOORD1;
};

float4 main (VSOutput input) : SV_TARGET
{
	float3 color = float3(0.5, 0.5, 0.5);
	float3 N = normalize(input.Normal);
	float3 L = normalize(input.LightVec);
	float3 V = normalize(input.ViewVec);
	float3 R = reflect(-L, N);
	float3 diffuse = max(dot(N, L), 0.15);
	float3 specular = pow(max(dot(R, V), 0.0), 32.0);
	return float4(diffuse * color.rgb + specular, 1.0);
}
