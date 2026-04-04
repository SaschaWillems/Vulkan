/* Copyright (c) 2026, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSInput
{
    float3 Pos;
    float3 Color;
    float3 Normal;
    float2 UV;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float3 Normal;
    float3 Color;
    float3 WorldPos;
    float2 UV;
};

[[SpecializationConstant]] const float NEAR_PLANE = 0.1;
[[SpecializationConstant]] const float FAR_PLANE = 256.0;

float linearDepth(float depth)
{
	float z = depth * 2.0f - 1.0f; 
	return (2.0f * NEAR_PLANE * FAR_PLANE) / (FAR_PLANE + NEAR_PLANE - z * (FAR_PLANE - NEAR_PLANE));	
}

struct FSOutput
{
    float4 Color;
    float4 PositionDepth;
    float4 Normal;
    float4 Albedo;
};

struct UBO
{
    float4x4 projection;
    float4x4 model;
    float4x4 view;
    float4x4 inverseTranspose;
};
ConstantBuffer<UBO> ubo;

[[vk::binding(0, 1)]] Sampler2D samplerColor;

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.Pos = mul(ubo.projection, mul(ubo.view, mul(ubo.model, float4(input.Pos, 1.0))));
    // Vertex position in world space
    output.WorldPos = mul(ubo.model, float4(input.Pos, 1.0)).xyz;
    // GL to Vulkan coord space
    output.WorldPos.y = -output.WorldPos.y;
    output.Normal = mul((float3x3)ubo.model, input.Normal);
    output.Color = input.Color;
    output.UV = input.UV;
    return output;
}

[shader("fragment")]
FSOutput fragmentMain(VSOutput input)
{
    FSOutput output;
	float3 N = normalize(input.Normal);
	N.y = -N.y;
    output.Normal = float4(N, 1.0);
    float4 sampledColor = samplerColor.Sample(input.UV) * float4(input.Color, 1.0);
    if (sampledColor.a < 0.5) {
        discard;
    };
    output.Albedo = sampledColor;
	// Store linearized depth in alpha component
    output.PositionDepth.rgb = input.WorldPos;
    output.PositionDepth.a = linearDepth(input.Pos.z);
    // Write color attachments to avoid undefined behaviour (validation error)
    output.Color = (1.0).rrrr;
	return output;
}