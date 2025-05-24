/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

[shader("callable")]
void callableMain(inout float3 outColor)
{
    // Generate a checker board pattern
	float2 pos = float2(DispatchRaysIndex().x / 8, DispatchRaysIndex().y / 8);
	float col = (pos.x + (pos.y % 2.0)) % 2.0;
	outColor = float3(col, col, col);
}