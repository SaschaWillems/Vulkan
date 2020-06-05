// Copyright 2020 Google LLC

TextureCube textureCubeMap : register(t1);
SamplerState samplerCubeMap : register(s1);

float4 main([[vk::location(0)]] float3 inUVW : NORMAL0) : SV_TARGET
{
	return textureCubeMap.Sample(samplerCubeMap, inUVW);
}