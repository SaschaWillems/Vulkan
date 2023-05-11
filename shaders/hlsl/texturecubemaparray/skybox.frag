// Copyright 2020 Google LLC

TextureCubeArray textureCubeMapArray : register(t1);
SamplerState samplerCubeMapArray : register(s1);

struct UBO
{
	float4x4 projection;
	float4x4 model;
	float4x4 invModel;
	float lodBias;
	int cubeMapIndex;
};

cbuffer ubo : register(b0) { UBO ubo; }

float4 main([[vk::location(0)]] float3 inUVW : TEXCOORD0) : SV_TARGET
{
	return textureCubeMapArray.SampleLevel(samplerCubeMapArray, float4(inUVW, ubo.cubeMapIndex), ubo.lodBias);
}