/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSInput
{
    float3 Pos;
}

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float3 UVW;
};

struct UBO
{
    float4x4 projection;
    float4x4 view;
    float4x4 model;
};
ConstantBuffer<UBO> ubo;

SamplerCube samplerCubeMap;

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.UVW = input.Pos;
    output.Pos = mul(ubo.projection, mul(ubo.view, mul(ubo.model, float4(input.Pos.xyz, 1.0))));
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
    return samplerCubeMap.Sample(input.UVW);
}