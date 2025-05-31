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
    float4 JointIndices;
    float4 JointWeights;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float3 Normal;
    float3 Color;
    float2 UV;
    float3 ViewVec;
    float3 LightVec;
};

struct UBO
{
    float4x4 projection;
    float4x4 view;
    float4 lightPos;
    float4x4 inverse;
};
ConstantBuffer<UBO> uboScene;

[[vk::binding(0, 1)]] StructuredBuffer<float4x4> jointMatrices;

[[vk::binding(0, 2)]] Sampler2D samplerColorMap;

[shader("vertex")]
VSOutput vertexMain(VSInput input, uniform float4x4 modelMat)
{
    VSOutput output;
    output.Normal = input.Normal;
    output.Color = input.Color;
    output.UV = input.UV;

    // Calculate skinned matrix from weights and joint indices of the current vertex
    float4x4 skinMat =
        input.JointWeights.x * jointMatrices[int(input.JointIndices.x)] +
        input.JointWeights.y * jointMatrices[int(input.JointIndices.y)] +
        input.JointWeights.z * jointMatrices[int(input.JointIndices.z)] +
        input.JointWeights.w * jointMatrices[int(input.JointIndices.w)];

    output.Pos = mul(uboScene.projection, mul(uboScene.view, mul(modelMat, mul(skinMat, float4(input.Pos, 1.0)))));
    output.Normal = mul((float3x3)modelMat, mul((float3x3)skinMat, input.Normal));

    float4 pos = mul(uboScene.view, float4(input.Pos, 1.0));
	float3 lPos = mul(float3x3(uboScene.view), uboScene.lightPos.xyz);
	output.LightVec = lPos - pos.xyz;
	output.ViewVec = -pos.xyz;

    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
    float4 color = samplerColorMap.Sample(input.UV) * float4(input.Color, 1.0);

    float3 N = normalize(input.Normal);
    float3 L = normalize(input.LightVec);
    float3 V = normalize(input.ViewVec);
    float3 R = reflect(L, N);
    float3 diffuse = max(dot(N, L), 0.0) * input.Color;
    float3 specular = pow(max(dot(R, V), 0.0), 16.0) * float3(0.75, 0.75, 0.75);
    return float4(diffuse * color.rgb + specular, 1.0);
}