// Copyright 2020 Google LLC

struct VSOutput
{
[[vk::location(0)]] float2 UV : TEXCOORD0;
[[vk::location(1)]] float3 Normal : NORMAL0;
[[vk::location(2)]] float3 Color : COLOR0;
[[vk::location(3)]] float3 EyePos : POSITION0;
[[vk::location(4)]] float3 LightVec : TEXCOORD2;
};

float4 main(VSOutput input) : SV_TARGET
{
  float3 Eye = normalize(-input.EyePos);
  float3 Reflected = normalize(reflect(-input.LightVec, input.Normal));

  float4 diff = float4(input.Color, 1.0) * max(dot(input.Normal, input.LightVec), 0.0);
  float shininess = 0.0;
  float4 spec = float4(1.0, 1.0, 1.0, 1.0) * pow(max(dot(Reflected, Eye), 0.0), 2.5) * shininess;

  return float4((diff + spec).rgb, 1);
}