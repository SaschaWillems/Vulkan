/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSInput
{
    float3 Pos;
    float3 Normal;
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
    float4 color;
};
ConstantBuffer<UBO> ubo;

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.Color = input.Color * ubo.color.rgb;
    output.Pos = mul(ubo.projection, mul(ubo.view, mul(ubo.model, float4(input.Pos.xyz, 1.0))));
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
    return float4(input.Color, 0.5);
}