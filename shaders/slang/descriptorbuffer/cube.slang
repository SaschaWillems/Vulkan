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

struct UBOCamera {
    float4x4 projection;
    float4x4 view;
};
ConstantBuffer<UBOCamera> uboCamera;

struct UBOModel {
    float4x4 local;
};
[[vk::binding(0, 1)]] ConstantBuffer<UBOModel> uboModel;

[[vk::binding(0, 2)]] Sampler2D samplerColorMap;

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.Normal = input.Normal;
    output.Color = input.Color;
    output.UV = input.UV;
    output.Pos = mul(uboCamera.projection, mul(uboCamera.view, mul(uboModel.local, float4(input.Pos.xyz, 1.0))));
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
    return samplerColorMap.Sample(input.UV) * float4(input.Color, 1.0);
}