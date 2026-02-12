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
};
[[vk::binding(0, 0)]] ConstantBuffer<UBO> ubo[2];
[[vk::binding(0, 1)]] Texture2D textureImage[2];
[[vk::binding(0, 2)]] SamplerState textureSampler[2];

[shader("vertex")]
VSOutput vertexMain(VSInput input, uint InstanceIndex: SV_VulkanInstanceID, uniform int samplerIndex, uniform int frameIndex)
{
    VSOutput output;
    output.Normal = input.Normal;
    output.Color = input.Color;
    output.UV = input.UV;
    output.Pos = mul(ubo[frameIndex].projection, mul(ubo[frameIndex].view, mul(ubo[frameIndex].model[InstanceIndex], float4(input.Pos.xyz, 1.0))));
    output.InstanceIndex = InstanceIndex;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input, uniform int samplerIndex, uniform int frameIndex)
{
    return textureImage[input.InstanceIndex].Sample(textureSampler[samplerIndex], input.UV) * float4(input.Color, 1.0);
}