/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

Sampler2D fontTexture;

struct VSInput
{
	float2 Pos : POSITION0;
	float2 UV;
	float4 Color;
};

struct VSOutput
{
	float4 Pos : SV_POSITION;
	float2 UV;
	float4 Color;
};

struct PushConstants
{
	float2 scale;
	float2 translate;
};

[shader("vertex")]
VSOutput vertexMain(VSInput input, uniform PushConstants pushConstants)
{
	VSOutput output;
	output.Pos = float4(input.Pos * pushConstants.scale + pushConstants.translate, 0.0, 1.0);
	output.UV = input.UV;
	output.Color = input.Color;
	return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
    return input.Color * fontTexture.Sample(input.UV);
}