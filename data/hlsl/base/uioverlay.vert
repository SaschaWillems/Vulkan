// Copyright 2020 Google LLC

struct VSInput
{
	[[vk::location(0)]]float2 Pos : POSITION0;
	[[vk::location(1)]]float2 UV : TEXCOORD0;
	[[vk::location(2)]]float4 Color : COLOR0;
};

struct VSOutput
{
	float4 Pos : SV_POSITION;
	[[vk::location(0)]]float2 UV : TEXCOORD0;
	[[vk::location(1)]]float4 Color : COLOR0;
};

struct PushConstants
{
	float2 scale;
	float2 translate;
};

[[vk::push_constant]]
PushConstants pushConstants;

VSOutput main(VSInput input)
{
	VSOutput output = (VSOutput)0;
	output.Pos = float4(input.Pos * pushConstants.scale + pushConstants.translate, 0.0, 1.0);
	output.UV = input.UV;
	output.Color = input.Color;
	return output;
}