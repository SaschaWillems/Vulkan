/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSInput
{
    [[vk::location(0)]] float3 Pos;
    [[vk::location(1)]] float3 Normal;
    [[vk::location(2)]] float2 UV;
    [[vk::location(3)]] float3 Color;
};

struct MatrixReference {
    float4x4 matrix;
};

struct PushConsts {
    // Pointer to the buffer with the scene's MVP matrix
    ConstBufferPointer<MatrixReference> sceneDataReference;
    // Pointer to the buffer for the data for each model
    ConstBufferPointer<MatrixReference> modelDataReference;
};
[[vk::push_constant]] PushConsts pushConstants;

struct VSOutput
{
    float4 Pos : SV_POSITION;
    [[vk::location(0)]] float3 Normal;
    [[vk::location(1)]] float3 Color;
    [[vk::location(2)]] float2 UV;
};

[shader("vertex")]
VSOutput main(VSInput input) 
{
    MatrixReference sceneData = pushConstants.sceneDataReference.get();
    MatrixReference modelData = pushConstants.modelDataReference.get();

    VSOutput output;
    output.Normal = input.Normal;
    output.Color = input.Color;
    output.UV = input.UV;
    output.Pos = mul(sceneData.matrix, mul(modelData.matrix, float4(input.Pos.xyz, 1.0)));
    return output;
}