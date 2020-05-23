// Copyright 2020 Google LLC

Texture2D textureColorMap : register(t2);
SamplerState samplerColorMap : register(s2);

struct DSOutput
{
[[vk::location(0)]] float3 Normal : NORMAL0;
[[vk::location(1)]] float2 UV : TEXCOORD0;
[[vk::location(2)]] float3 EyePos : POSITION0;
[[vk::location(3)]] float3 LightVec : TEXCOORD2;
};

float4 main(DSOutput input) : SV_TARGET
{
	float3 N = normalize(input.Normal);
	float3 L = normalize(float3(1.0, 1.0, 1.0));

	float3 Eye = normalize(-input.EyePos);
	float3 Reflected = normalize(reflect(-input.LightVec, input.Normal));

	float4 IAmbient = float4(0.0, 0.0, 0.0, 1.0);
	float4 IDiffuse = float4(1.0, 1.0, 1.0, 1.0) * max(dot(input.Normal, input.LightVec), 0.0);

	return float4((IAmbient + IDiffuse) * float4(textureColorMap.Sample(samplerColorMap, input.UV).rgb, 1.0));
}