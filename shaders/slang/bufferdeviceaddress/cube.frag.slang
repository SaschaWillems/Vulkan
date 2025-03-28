/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

 [[vk::binding(0, 0)]]
Sampler2D samplerColorMap;

struct VSInput
{
    [[vk::location(1)]] float3 Color;
    [[vk::location(2)]] float2 UV;
};

[shader("fragment")]
float4 main(VSInput input)
{
    return samplerColorMap.Sample(input.UV) * float4(input.Color, 1.0);
}