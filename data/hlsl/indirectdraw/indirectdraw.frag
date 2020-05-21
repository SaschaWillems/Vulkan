// Copyright 2020 Google LLC

Texture2DArray textureArray : register(t1);
SamplerState samplerArray : register(s1);

struct VSOutput
{
[[vk::location(0)]] float3 Normal : NORMAL0;
[[vk::location(1)]] float3 Color : COLOR0;
[[vk::location(2)]] float3 UV : TEXCOORD0;
[[vk::location(3)]] float3 ViewVec : TEXCOORD1;
[[vk::location(4)]] float3 LightVec : TEXCOORD2;
};

float4 main(VSOutput input) : SV_TARGET
{
	float4 color = textureArray.Sample(samplerArray, input.UV);

	if (color.a < 0.5)
	{
		clip(-1);
	}

	float3 N = normalize(input.Normal);
	float3 L = normalize(input.LightVec);
	float3 ambient = float3(0.65, 0.65, 0.65);
	float3 diffuse = max(dot(N, L), 0.0) * input.Color;
	return float4((ambient + diffuse) * color.rgb, 1.0);
}
