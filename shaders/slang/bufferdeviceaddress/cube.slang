/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

Sampler2D samplerColorMap;

struct VSInput
{
    float3 Pos;
    float3 Normal;
    float2 UV;
    float3 Color;
};

// Pointers to the matrices, addresses are set in the application
struct PushConsts {
    float4x4 *sceneMatrices;
    float4x4 *modelMatrices;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float3 Normal;
    float3 Color;
    float2 UV;
};

[shader("vertex")]
VSOutput vertexMain(VSInput input, uniform PushConsts pushConstants) 
{
    VSOutput output;
    output.Normal = input.Normal;
    output.Color = input.Color;
    output.UV = input.UV;
    output.Pos = mul(*pushConstants.sceneMatrices, mul(*pushConstants.modelMatrices, float4(input.Pos.xyz, 1.0)));
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
    return samplerColorMap.Sample(input.UV) * float4(input.Color, 1.0);
}