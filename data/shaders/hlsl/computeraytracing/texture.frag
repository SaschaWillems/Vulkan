// Copyright 2020 Google LLC

Texture2D textureColor : register(t0);
SamplerState samplerColor : register(s0);

float4 main([[vk::location(0)]] float2 inUV : TEXCOORD0) : SV_TARGET
{
  return textureColor.Sample(samplerColor, float2(inUV.x, 1.0 - inUV.y));
}