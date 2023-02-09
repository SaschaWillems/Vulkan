// Copyright 2020 Google LLC

Texture2D textureColorMap : register(t1);
SamplerState samplerColorMap : register(s1);

struct VSOutput
{
[[vk::location(0)]] float3 Normal : NORMAL0;
[[vk::location(1)]] float3 Color : COLOR0;
[[vk::location(2)]] float3 LightVec : TEXCOORD2;
};

float4 main(VSOutput input) : SV_TARGET
{
	float3 color;
	float3 N = normalize(input.Normal);
	float3 L = normalize(input.LightVec);
	float intensity = dot(N,L);
	if (intensity > 0.98)
		color = input.Color * 1.5;
	else if  (intensity > 0.9)
		color = input.Color * 1.0;
	else if (intensity > 0.5)
		color = input.Color * 0.6;
	else if (intensity > 0.25)
		color = input.Color * 0.4;
	else
		color = input.Color * 0.2;
	// Desaturate a bit
	color = lerp(color, dot(float3(0.2126,0.7152,0.0722), color).xxx, 0.1);
	return float4(color, 1);
}