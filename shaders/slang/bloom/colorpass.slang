/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSInput
{
    float4 Pos;
    float2 UV;
    float3 Color;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float3 Color;
    float2 UV;
};

struct UBO
{
    float4x4 projection;
    float4x4 view;
    float4x4 model;
};
ConstantBuffer<UBO> ubo;

Sampler2D colorMapSampler;

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.UV = input.UV;
    output.Color = input.Color;
    output.Pos = mul(ubo.projection, mul(ubo.view, mul(ubo.model, input.Pos)));
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
	return float4(input.Color, 1);
}