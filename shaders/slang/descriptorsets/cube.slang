/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSInput
{
    float3 Pos;
    float3 Normal;
    float2 UV;
    float3 Color;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float3 Normal;
    float3 Color;
    float2 UV;
};

// Slang auto generates bindings by order of descriptors
// So the UBO is bound to slot 0, the sampler to slot 1
// due to their order in the shader

struct UBOMatrices {
    float4x4 projection;
    float4x4 view;
    float4x4 model;
};
ConstantBuffer<UBOMatrices> uboMatrices;

Sampler2D samplerColorMap;

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.Normal = input.Normal;
    output.Color = input.Color;
    output.UV = input.UV;
    output.Pos = mul(uboMatrices.projection, mul(uboMatrices.view, mul(uboMatrices.model, float4(input.Pos.xyz, 1.0))));
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
    return samplerColorMap.Sample(input.UV) * float4(input.Color, 1.0);
}