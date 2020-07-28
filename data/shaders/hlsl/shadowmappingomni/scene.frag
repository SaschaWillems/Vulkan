// Copyright 2020 Google LLC

TextureCube shadowCubeMapTexture : register(t1);
SamplerState shadowCubeMapSampler : register(s1);

struct VSOutput
{
[[vk::location(0)]] float3 Normal : NORMAL0;
[[vk::location(1)]] float3 Color : COLOR0;
[[vk::location(2)]] float3 EyePos : POSITION0;
[[vk::location(3)]] float3 LightVec : TEXCOORD2;
[[vk::location(4)]] float3 WorldPos : POSITION1;
[[vk::location(5)]] float3 LightPos : POSITION2;
};

#define EPSILON 0.15
#define SHADOW_OPACITY 0.5

float4 main(VSOutput input) : SV_TARGET
{
	// Lighting
	float3 N = normalize(input.Normal);
	float3 L = normalize(float3(1.0, 1.0, 1.0));

	float3 Eye = normalize(-input.EyePos);
	float3 Reflected = normalize(reflect(-input.LightVec, input.Normal));

	float4 IAmbient = float4(float3(0.05, 0.05, 0.05), 1.0);
	float4 IDiffuse = float4(1.0, 1.0, 1.0, 1.0) * max(dot(input.Normal, input.LightVec), 0.0);

	float4 outFragColor = float4(IAmbient + IDiffuse * float4(input.Color, 1.0));

	// Shadow
	float3 lightVec = input.WorldPos - input.LightPos;
    float sampledDist = shadowCubeMapTexture.Sample(shadowCubeMapSampler, lightVec).r;
    float dist = length(lightVec);

	// Check if fragment is in shadow
    float shadow = (dist <= sampledDist + EPSILON) ? 1.0 : SHADOW_OPACITY;

	outFragColor.rgb *= shadow;
	return outFragColor;
}