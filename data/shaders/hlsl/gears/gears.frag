// Copyright 2020 Google LLC

struct VSOutput
{
[[vk::location(0)]] float3 Normal : NORMAL0;
[[vk::location(1)]] float3 Color : COLOR0;
[[vk::location(2)]] float3 EyePos : POSITION0;
[[vk::location(3)]] float3 LightVec : TEXCOORD2;
};

float4 main(VSOutput input) : SV_TARGET
{
  float3 Eye = normalize(-input.EyePos);
  float3 Reflected = normalize(reflect(-input.LightVec, input.Normal));

  float4 IAmbient = float4(0.2, 0.2, 0.2, 1.0);
  float4 IDiffuse = float4(0.5, 0.5, 0.5, 0.5) * max(dot(input.Normal, input.LightVec), 0.0);
  float specular = 0.25;
  float4 ISpecular = float4(0.5, 0.5, 0.5, 1.0) * pow(max(dot(Reflected, Eye), 0.0), 0.8) * specular;

  return float4((IAmbient + IDiffuse) * float4(input.Color, 1.0) + ISpecular);
}