/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

[[vk::binding(0, 0)]]
Sampler2D fontTexture;

struct VSOutput
{
	[[vk::location(0)]]float2 UV : TEXCOORD0;
	[[vk::location(1)]]float4 Color : COLOR0;
};

[shader("fragment")]
float4 main(VSOutput input) : SV_TARGET
{
	return input.Color * fontTexture.Sample(input.UV);
}