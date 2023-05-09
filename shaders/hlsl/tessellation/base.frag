// Copyright 2020 Google LLC

Texture2D textureColorMap : register(t2);
SamplerState samplerColorMap : register(s2);

struct DSOutput
{
[[vk::location(0)]] float3 Normal : NORMAL0;
[[vk::location(1)]] float2 UV : TEXCOORD0;
};

float4 main(DSOutput input) : SV_TARGET
{
	float3 N = normalize(input.Normal);
	float3 L = normalize(float3(0.0, -4.0, 4.0));

	float4 color = textureColorMap.Sample(samplerColorMap, input.UV);

	return float4(clamp(max(dot(N,L), 0.0), 0.2, 1.0) * color.rgb * 1.5, 1);
}
