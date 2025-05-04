/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSInput
{
    float2 Pos;
    float2 UV;
    float4 Color;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float2 UV;
    float4 Color;
};

Sampler2D fontSampler;

[shader("vertex")]
VSOutput vertexMain(VSInput input, uniform float2 scale, uniform float2 translate)
{
    VSOutput output;
    output.UV = input.UV;
    output.Color = input.Color;
    output.Pos = float4(input.Pos * scale + translate, 0.0, 1.0);
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
    return input.Color * fontSampler.Sample(input.UV);
}