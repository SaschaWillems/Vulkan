/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSInput
{
    float3 Pos;
    float3 Color;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float3 Color;
};

[shader("vertex")]
VSOutput vertexMain(VSInput input, uniform float4x4 mvp)
{
	VSOutput output;
	output.Color = input.Color;
	output.Pos = mul(mvp, float4(input.Pos.xyz, 1.0));
	return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
	return float4(input.Color, 1.0);
}