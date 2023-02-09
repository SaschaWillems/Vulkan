// Copyright 2020 Google LLC

Texture2D textureColor : register(t1);
SamplerState samplerColor : register(s1);

struct UBO
{
	float4x4 projection;
	float4x4 view;
	float4x4 model;
	float4x4 lightSpace;
	float4 lightPos;
	float zNear;
	float zFar;
};

cbuffer ubo : register(b0) { UBO ubo; }

float LinearizeDepth(float depth)
{
  float n = ubo.zNear;
  float f = ubo.zFar;
  float z = depth;
  return (2.0 * n) / (f + n - z * (f - n));
}

float4 main([[vk::location(0)]] float2 inUV : TEXCOORD0) : SV_TARGET
{
	float depth = textureColor.Sample(samplerColor, inUV).r;
	return float4((1.0-LinearizeDepth(depth)).xxx, 1.0);
}