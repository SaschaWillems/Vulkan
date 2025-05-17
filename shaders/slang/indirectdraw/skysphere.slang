/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSInput
{
    float4 Pos;
    float2 UV;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float2 UV;
};

struct UBO
{
    float4x4 projection;
    float4x4 modelview;
};
ConstantBuffer<UBO> ubo;

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.UV = input.UV;
    // Skysphere always at center, only use rotation part of modelview matrix
    output.Pos = mul(ubo.projection, float4(mul((float3x3)ubo.modelview, input.Pos.xyz), 1));
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
	const float4 gradientStart = float4(0.93, 0.9, 0.81, 1.0);
	const float4 gradientEnd = float4(0.35, 0.5, 1.0, 1.0);
	return lerp(gradientStart, gradientEnd, min(0.5 - (input.UV.y + 0.05), 0.5)/0.15 - 0.5);
}