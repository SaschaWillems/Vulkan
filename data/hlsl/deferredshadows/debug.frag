// Copyright 2020 Google LLC

Texture2D texturePosition : register(t1);
SamplerState samplerPosition : register(s1);
Texture2D textureNormal : register(t2);
SamplerState samplerNormal : register(s2);
Texture2D textureAlbedo : register(t3);
SamplerState samplerAlbedo : register(s3);
Texture2DArray textureDepth : register(t5);
SamplerState samplerDepth : register(s5);

float LinearizeDepth(float depth)
{
  float n = 0.1; // camera z near
  float f = 64.0; // camera z far
  float z = depth;
  return (2.0 * n) / (f + n - z * (f - n));
}

float4 main([[vk::location(0)]] float3 inUV : TEXCOORD0) : SV_TARGET
{
	// Display depth from light's point-of-view
	// inUV.w = number of light source
	float depth = textureDepth.Sample(samplerDepth, float3(inUV)).r;
	return float4((1.0 - LinearizeDepth(depth)).xxx, 0.0);
}