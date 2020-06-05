// Copyright 2020 Google LLC

Texture2D textureColor : register(t1);
SamplerState samplerColor : register(s1);

float LinearizeDepth(float depth)
{
  float n = 1.0; // camera z near
  float f = 128.0; // camera z far
  float z = depth;
  return (2.0 * n) / (f + n - z * (f - n));
}

float4 main([[vk::location(0)]] float2 inUV : TEXCOORD0) : SV_TARGET
{
	float depth = textureColor.Sample(samplerColor, inUV).r;
	return float4((1.0-LinearizeDepth(depth)).xxx, 1.0);
}