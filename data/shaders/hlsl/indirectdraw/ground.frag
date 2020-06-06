// Copyright 2020 Google LLC

Texture2D textureColor : register(t2);
SamplerState samplerColor : register(s2);

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
	// Last array layer is terrain tex
	float4 color = textureColor.Sample(samplerColor, input.UV);
	float3 N = normalize(input.Normal);
	float3 L = normalize(input.LightVec);
	float3 V = normalize(input.ViewVec);
	float3 R = reflect(-L, N);
	float3 ambient = float3(0.65, 0.65, 0.65);
	float3 diffuse = max(dot(N, L), 0.0) * input.Color;
	float3 specular = pow(max(dot(R, V), 0.0), 16.0) * float3(0.1, 0.1, 0.1);
	return float4((ambient + diffuse) * color.rgb + specular, 1.0);
}