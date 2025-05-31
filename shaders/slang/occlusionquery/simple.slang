/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSInput
{
    float4 Pos;
    float3 Color;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float3 Color;
};

struct UBO
{
    float4x4 projection;
    float4x4 view;
    float4x4 model;
};
ConstantBuffer<UBO> ubo;

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.Pos = mul(ubo.projection, mul(ubo.view, mul(ubo.model, float4(input.Pos.xyz, 1.0))));
    return output;
}

[shader("fragment")]
float4 fragmentMain()
{
    return float4(1.0, 1.0, 1.0, 1.0);
}