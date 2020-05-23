// Copyright 2020 Google LLC

struct VSInput
{
	[[vk::location(0)]]float2 Pos : POSITION0;
	[[vk::location(1)]]float2 UV : TEXCOORD0;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
	[[vk::location(0)]]float2 UV : TEXCOORD0;
};

VSOutput main(VSInput input)
{
	VSOutput output = (VSOutput)0;
	output.Pos = float4(input.Pos, 0.0, 1.0);
	output.UV = input.UV;
	return output;
}
