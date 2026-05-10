/* Copyright (c) 2026, Sascha Willems
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
    float3 Normal;
    float2 UV;
    float3 ViewVec;
    float3 LightVec;
};

struct UBO
{
    float4x4 projection;
    float4x4 model;
    float4 lightPos;
};
ConstantBuffer<UBO> ubo;

[[vk::binding(0, 1)]] Sampler2D samplerColorMap;

[shader("vertex")]
VSOutput vertexMain(VSInput input, uniform float3 position)
{
    float4 locPos = float4(input.Pos.xyz + position, 1.0);
    VSOutput output;
    output.Normal = input.Normal;
    output.UV = input.UV;
    output.Pos = mul(ubo.projection, mul(ubo.model, locPos));
    float4 pos = mul(ubo.model, locPos);
    output.Normal = mul((float3x3)ubo.model, input.Normal);
    float3 lPos = mul((float3x3)ubo.model, ubo.lightPos.xyz);
    output.LightVec = lPos - input.Pos;
    output.ViewVec = -input.Pos;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
    float4 color = samplerColorMap.Sample(input.UV);
    float3 N = normalize(input.Normal);
    float3 L = normalize(input.LightVec);
    float3 V = normalize(input.ViewVec);
    float3 R = reflect(-L, N);
    float diffuse = max(dot(N, L), 0.5);
    return float4(diffuse * color.rgb, color.a);
}