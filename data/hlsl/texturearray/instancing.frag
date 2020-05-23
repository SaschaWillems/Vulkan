// Copyright 2020 Google LLC

Texture2DArray textureArray : register(t1);
SamplerState samplerArray : register(s1);

float4 main([[vk::location(0)]] float3 inUV : TEXCOORD0) : SV_TARGET
{
	return textureArray.Sample(samplerArray, inUV);
}