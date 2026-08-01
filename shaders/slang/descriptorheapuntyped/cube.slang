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

struct ModelData
{
    float4 pos;
    float4 color;
};

struct MatrixReference
{
    float4x4 mvp;
    uint samplerIndex;
    uint imageHeapIndexOffset;
};

[require(spvDescriptorHeapEXT)]
[shader("vertex")]
VSOutput vertexMain(VSInput input, uint InstanceIndex: SV_VulkanInstanceID, MatrixReference *matrixReference)
{
    ModelData instanceData = getDescriptorFromHandle(ConstantBuffer<ModelData>.Handle(uint2(InstanceIndex, 0)));

    VSOutput output;
    MatrixReference uniformData = *matrixReference;
    float3 localPos = input.Pos * 0.25f + instanceData.pos.xyz;

    output.Normal = input.Normal;
    output.Color = instanceData.color.rgb;
    output.UV = input.UV;
    output.Pos = mul(uniformData.mvp, float4(localPos, 1.0));
    output.InstanceIndex = int(InstanceIndex);
    return output;
}

[require(spvDescriptorHeapEXT)]
[shader("fragment")]
float4 fragmentMain(VSOutput input, MatrixReference* matrixReference)
{
    MatrixReference uniformData = *matrixReference;
 
    Texture2D texture = Texture2D.Handle(uint2(uniformData.imageHeapIndexOffset + uint(input.InstanceIndex), 0));
    SamplerState sampler = SamplerState.Handle(uint2(uniformData.samplerIndex, 0));

    return texture.Sample(sampler, input.UV) * float4(input.Color, 1.0);
}