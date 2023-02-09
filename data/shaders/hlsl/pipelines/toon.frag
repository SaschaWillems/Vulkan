// Copyright 2020 Google LLC

Texture2D textureColorMap : register(t1);
SamplerState samplerColorMap : register(s1);

struct VSOutput
{
[[vk::location(0)]] float3 Normal : NORMAL0;
[[vk::location(1)]] float3 Color : COLOR0;
[[vk::location(2)]] float3 ViewVec : TEXCOORD1;
[[vk::location(3)]] float3 LightVec : TEXCOORD2;
};

float4 main(VSOutput input) : SV_TARGET
{
	// Desaturate color
    float3 color = lerp(input.Color, dot(float3(0.2126,0.7152,0.0722), input.Color).xxx, 0.65);

	// High ambient colors because mesh materials are pretty dark
	float3 ambient = color * float3(1.0, 1.0, 1.0);
	float3 N = normalize(input.Normal);
	float3 L = normalize(input.LightVec);
	float3 V = normalize(input.ViewVec);
	float3 R = reflect(-L, N);
	float3 diffuse = max(dot(N, L), 0.0) * color;
	float3 specular = pow(max(dot(R, V), 0.0), 16.0) * float3(0.75, 0.75, 0.75);

	float intensity = dot(N,L);
	float shade = 1.0;
	shade = intensity < 0.5 ? 0.75 : shade;
	shade = intensity < 0.35 ? 0.6 : shade;
	shade = intensity < 0.25 ? 0.5 : shade;
	shade = intensity < 0.1 ? 0.25 : shade;

	return float4(input.Color * 3.0 * shade, 1);
}