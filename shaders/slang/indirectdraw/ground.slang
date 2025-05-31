/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSInput
{
   float4 Pos;
   float3 Normal;
   float2 UV;
   float3 Color;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float2 UV;
};

struct UBO
{
    float4x4 projection;
    float4x4 modelview;
};
ConstantBuffer<UBO> ubo;

[[vk::binding(2,0)]] Sampler2D samplerColor;

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.UV = input.UV * 32.0;
    output.Pos = mul(ubo.projection, mul(ubo.modelview, float4(input.Pos.xyz, 1.0)));
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
    float4 color = samplerColor.Sample(input.UV);
	return float4(color.rgb, 1.0);
}