// Copyright 2020 Google LLC

struct VSInput
{
[[vk::location(0)]] float4 Pos : POSITION0;
[[vk::location(1)]] float4 Color : COLOR0;
[[vk::location(2)]] float Alpha : TEXCOORD0;
[[vk::location(3)]] float Size : TEXCOORD1;
[[vk::location(4)]] float Rotation : TEXCOORD2;
[[vk::location(5)]] int Type : TEXCOORD3;
};

struct VSOutput
{
	float4 Pos : SV_POSITION;
[[vk::builtin("PointSize")]] float PSize : PSIZE;
[[vk::location(0)]] float4 Color : COLOR0;
[[vk::location(1)]] float Alpha : TEXCOORD0;
[[vk::location(2)]] int Type : TEXCOORD1;
[[vk::location(3)]] float Rotation : TEXCOORD2;
[[vk::location(4)]] float2 CenterPos : POSITION1;
[[vk::location(5)]] float PointSize : TEXCOORD3;
};

struct UBO
{
	float4x4 projection;
	float4x4 modelview;
	float2 viewportDim;
	float pointSize;
};

cbuffer ubo : register(b0) { UBO ubo; }

VSOutput main (VSInput input)
{
	VSOutput output = (VSOutput)0;
	output.Color = input.Color;
	output.Alpha = input.Alpha;
	output.Type = input.Type;
	output.Rotation = input.Rotation;

	output.Pos = mul(ubo.projection, mul(ubo.modelview, float4(input.Pos.xyz, 1.0)));

	// Base size of the point sprites
	float spriteSize = 8.0 * input.Size;

	// Scale particle size depending on camera projection
	float4 eyePos = mul(ubo.modelview, float4(input.Pos.xyz, 1.0));
	float4 projectedCorner = mul(ubo.projection, float4(0.5 * spriteSize, 0.5 * spriteSize, eyePos.z, eyePos.w));
	output.PointSize = output.PSize = ubo.viewportDim.x * projectedCorner.x / projectedCorner.w;
	output.CenterPos = ((output.Pos.xy / output.Pos.w) + 1.0) * 0.5 * ubo.viewportDim;
	return output;
}