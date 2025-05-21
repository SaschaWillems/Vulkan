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
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float2 UV;
    float LodBias;
};

struct UBO
{
    float4x4 projection;
    float4x4 model;
    float4 viewPos;
    float lodBias;
};
ConstantBuffer<UBO> ubo;

Sampler2D samplerColor;

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
	VSOutput output;
    output.UV = input.UV;
    output.LodBias = ubo.lodBias;
    output.Pos = mul(ubo.projection, mul(ubo.model, float4(input.Pos.xyz, 1.0)));
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
    // Check if texel is resident
    uint status = 0;
    float4 sampledColor = samplerColor.Sample(input.UV, int2(0, 0), input.LodBias, status);
    bool texelResident = CheckAccessFullyMapped(status);
    if (texelResident) {
        return sampledColor;
    } else {
        return float4(0.0, 0.0, 0.0, 1.0);
    }
}