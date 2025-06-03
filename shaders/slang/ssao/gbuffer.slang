/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSInput
{
    float4 Pos;
    float2 UV;
    float3 Color;
    float3 Normal;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float3 Normal;
    float2 UV;
    float3 Color;
    float3 WorldPos;
};

struct FSOutput
{
    float4 Position : SV_TARGET0;
    float4 Normal : SV_TARGET1;
    float4 Albedo : SV_TARGET2;
};

struct UBO
{
    float4x4 projection;
    float4x4 model;
    float4x4 view;
    float nearPlane;
    float farPlane;
};
ConstantBuffer<UBO> ubo;

[[vk::binding(0, 1)]] Texture2D textureColorMap;
[[vk::binding(0, 1)]] SamplerState samplerColorMap;

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.Pos = mul(ubo.projection, mul(ubo.view, mul(ubo.model, input.Pos)));
    output.UV = input.UV;
    // Vertex position in view space
    output.WorldPos = mul(ubo.view, mul(ubo.model, input.Pos)).xyz;
    // Normal in view space
    float3x3 normalMatrix = (float3x3)mul(ubo.view, ubo.model);
    output.Normal = mul(normalMatrix, input.Normal);
    output.Color = input.Color;
    return output;
}

float linearDepth(float depth)
{
	float z = depth * 2.0f - 1.0f;
	return (2.0f * ubo.nearPlane * ubo.farPlane) / (ubo.farPlane + ubo.nearPlane - z * (ubo.farPlane - ubo.nearPlane));
}

[shader("fragment")]
FSOutput fragmentMain(VSOutput input)
{
	FSOutput output;
	output.Position = float4(input.WorldPos, linearDepth(input.Pos.z));
	output.Normal = float4(normalize(input.Normal) * 0.5 + 0.5, 1.0);
	output.Albedo = textureColorMap.Sample(samplerColorMap, input.UV) * float4(input.Color, 1.0);
	return output;
}