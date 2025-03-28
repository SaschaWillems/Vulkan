/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSOutput
{
    float3 Color;
};

[shader("fragment")]
float4 main(VSOutput input) : SV_TARGET
{
	return float4(input.Color, 1.0);
}