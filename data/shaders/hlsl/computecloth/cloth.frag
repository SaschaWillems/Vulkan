// Copyright 2020 Google LLC

Texture2D textureColor : register(t1);
SamplerState samplerColor : register(s1);

struct VSOutput
{
[[vk::location(0)]]float2 UV : TEXCOORD0;
[[vk::location(1)]]float3 Normal : NORMAL0;
[[vk::location(2)]]float3 ViewVec : TEXCOORD1;
[[vk::location(3)]]float3 LightVec : TEXCOORD2;
};

float4 main (VSOutput input) : SV_TARGET
{
	float3 color = textureColor.Sample(samplerColor, input.UV).rgb;
	float3 N = normalize(input.Normal);
	float3 L = normalize(input.LightVec);
	float3 V = normalize(input.ViewVec);
	float3 R = reflect(-L, N);
	float3 diffuse = max(dot(N, L), 0.15) * float3(1, 1, 1);
	float3 specular = pow(max(dot(R, V), 0.0), 8.0) * float3(0.2, 0.2, 0.2);
	return float4(diffuse * color.rgb + specular, 1.0);
}
