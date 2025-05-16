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
};

struct UBO
{
    float4x4 mvp;
};
ConstantBuffer<UBO> ubo;

Sampler2D samplerColorMap;

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.Pos = mul(ubo.mvp, float4(input.Pos, 1.0));
    output.UV = input.UV;
    return output;
}


[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
	return samplerColorMap.Sample(input.UV);
}
