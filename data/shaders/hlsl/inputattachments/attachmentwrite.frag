// Copyright 2020 Google LLC

struct VSOutput
{
[[vk::location(0)]] float3 Color : COLOR0;
[[vk::location(1)]] float3 Normal : NORMAL0;
[[vk::location(2)]] float3 ViewVec : TEXCOORD1;
[[vk::location(3)]] float3 LightVec : TEXCOORD2;
};

float4 main(VSOutput input) : SV_TARGET
{
	// Toon shading color attachment output
	float intensity = dot(normalize(input.Normal), normalize(input.LightVec));
	float shade = 1.0;
	shade = intensity < 0.5 ? 0.75 : shade;
	shade = intensity < 0.35 ? 0.6 : shade;
	shade = intensity < 0.25 ? 0.5 : shade;
	shade = intensity < 0.1 ? 0.25 : shade;

	return float4(input.Color * 3.0 * shade, 1);

	// Depth attachment does not need to be explicitly written
}