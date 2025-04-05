/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

Sampler2D samplerColorMap;

struct VSInput
{
    float3 Pos;
    float3 Normal;
    float2 UV;
    float3 Color;
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

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float3 Normal;
    float3 Color;
    float2 UV;
};

[shader("vertex")]
VSOutput vertexMain(VSInput input, uniform PushConsts pushConstants) 
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

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
    return samplerColorMap.Sample(input.UV) * float4(input.Color, 1.0);
}