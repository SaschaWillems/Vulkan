// Copyright 2020 Google LLC

struct VSInput
{
[[vk::location(0)]] float4 Pos : POSITION0;
[[vk::location(1)]] float4 Vel : TEXCOORD0;
};

struct VSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float GradientPos : POSITION0;
[[vk::location(1)]] float2 CenterPos : POSITION1;
[[vk::builtin("PointSize")]] float PSize : PSIZE;
[[vk::location(2)]] float PointSize : TEXCOORD0;
};

struct UBO
{
	float4x4 projection;
	float4x4 modelview;
	float2 screendim;
};

cbuffer ubo : register(b2) { UBO ubo; }

VSOutput main (VSInput input)
{
	VSOutput output = (VSOutput)0;
	const float spriteSize = 0.005 * input.Pos.w; // Point size influenced by mass (stored in input.Pos.w);

	float4 eyePos = mul(ubo.modelview, float4(input.Pos.x, input.Pos.y, input.Pos.z, 1.0));
	float4 projectedCorner = mul(ubo.projection, float4(0.5 * spriteSize, 0.5 * spriteSize, eyePos.z, eyePos.w));
	output.PSize = output.PointSize = clamp(ubo.screendim.x * projectedCorner.x / projectedCorner.w, 1.0, 128.0);

	output.Pos = mul(ubo.projection, eyePos);
	output.CenterPos = ((output.Pos.xy / output.Pos.w) + 1.0) * 0.5 * ubo.screendim;

	output.GradientPos = input.Vel.w;
	return output;
}