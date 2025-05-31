/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSInput
{
    float3 Pos;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float3 UVW;
};

struct UBO
{
    float4x4 projection;
    float4x4 model;
    float4x4 invModel;
    float lodBias;
    int cubeMapIndex;
};
ConstantBuffer<UBO> ubo;

SamplerCubeArray samplerCubeMapArray;

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.UVW = input.Pos;
    output.UVW.xy *= -1.0;
    // Remove translation from view matrix
    float4x4 viewMat = ubo.model;
    viewMat[0][3] = 0.0;
    viewMat[1][3] = 0.0;
    viewMat[2][3] = 0.0;
    output.Pos = mul(ubo.projection, mul(viewMat, float4(input.Pos.xyz, 1.0)));
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
    return samplerCubeMapArray.SampleLevel(float4(input.UVW, ubo.cubeMapIndex), ubo.lodBias);
}