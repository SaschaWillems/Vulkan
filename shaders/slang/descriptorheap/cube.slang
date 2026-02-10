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
    float3 Color;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float3 Normal;
    float3 Color;
    float2 UV;
    int InstanceIndex;
};

struct UBO {
    float4x4 projection;
    float4x4 view;
    float4x4 model[2];
    int selectedSampler;
};
[[vk::binding(0, 0)]] ConstantBuffer<UBO> ubo;
[[vk::binding(0, 1)]] Texture2D textureImage[2];
[[vk::binding(0, 2)]] SamplerState textureSampler[2];

[shader("vertex")]
VSOutput vertexMain(VSInput input, uint InstanceIndex: SV_VulkanInstanceID)
{
    VSOutput output;
    output.Normal = input.Normal;
    output.Color = input.Color;
    output.UV = input.UV;
    output.Pos = mul(ubo.projection, mul(ubo.view, mul(ubo.model[InstanceIndex], float4(input.Pos.xyz, 1.0))));
    output.InstanceIndex = InstanceIndex;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
    return textureImage[input.InstanceIndex].Sample(textureSampler[ubo.selectedSampler], input.UV) * float4(input.Color, 1.0);
}