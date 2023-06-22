/* Copyright (c) 2023, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSOutput
{
[[vk::location(0)]] float4 color : COLOR0;
};

float4 main(VSOutput input) : SV_TARGET
{
	return float4(input.color);
}