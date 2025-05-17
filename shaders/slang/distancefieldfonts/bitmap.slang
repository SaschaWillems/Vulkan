/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSInput
{
	float3 Pos;
	float2 UV;
};

struct VSOutput
{
	float4 Pos : SV_POSITION;
	float2 UV;
};

struct UBO
{
    float4x4 projection;
    float4x4 model;
};
ConstantBuffer<UBO> ubo;
Sampler2D samplerColor;

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.UV = input.UV;
    output.Pos = mul(ubo.projection, mul(ubo.model, float4(input.Pos.xyz, 1.0)));
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
    return samplerColor.Sample(input.UV).aaaa;
}