// Copyright 2020 Google LLC

struct VSInput
{
[[vk::location(0)]] float2 Pos : POSITION0;
[[vk::location(1)]] float4 GradientPos : POSITION1;
};

struct VSOutput
{
  float4 Pos : SV_POSITION;
[[vk::builtin("PointSize")]] float PSize : PSIZE;
[[vk::location(0)]] float4 Color : COLOR0;
[[vk::location(1)]] float GradientPos : POSITION0;
[[vk::location(2)]] float2 CenterPos : POSITION1;
[[vk::location(3)]] float PointSize : TEXCOORD0;
};

struct PushConsts
{
  float2 screendim;
};

[[vk::push_constant]] PushConsts pushConstants;

VSOutput main (VSInput input)
{
  VSOutput output = (VSOutput)0;
  output.PSize = output.PointSize = 8.0;
  output.Color = float4(0.035, 0.035, 0.035, 0.035);
  output.GradientPos = input.GradientPos.x;
  output.Pos = float4(input.Pos.xy, 1.0, 1.0);
	output.CenterPos = ((output.Pos.xy / output.Pos.w) + 1.0) * 0.5 * pushConstants.screendim;
  return output;
}