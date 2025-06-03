/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct UBO
{
    float4x4 depthMVP;
};
ConstantBuffer<UBO> ubo;

[shader("vertex")]
float4 vertexMain(float3 Pos) : SV_POSITION
{
    return mul(ubo.depthMVP, float4(Pos, 1.0));
}

[shader("fragment")]
float4 fragmentMain()
{
	return float4(1.0, 0.0, 0.0, 1.0);
}