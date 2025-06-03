/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSInput
{
    float4 Pos;
    float3 Color;
    float3 Normal;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float3 Normal;
    float3 Color;
    float3 WorldPos;
    float3 Tangent;
};

struct FSOutput
{
    float4 Color : SV_TARGET0;
    float4 Position : SV_TARGET1;
    float4 Normal : SV_TARGET2;
    float4 Albedo : SV_TARGET3;
};

struct UBO
{
    float4x4 projection;
    float4x4 model;
    float4x4 view;
};
ConstantBuffer<UBO> ubo;

[[SpecializationConstant]] const float NEAR_PLANE = 0.1;
[[SpecializationConstant]] const float FAR_PLANE = 256.0;

float linearDepth(float depth)
{
    float z = depth * 2.0f - 1.0f;
    return (2.0f * NEAR_PLANE * FAR_PLANE) / (FAR_PLANE + NEAR_PLANE - z * (FAR_PLANE - NEAR_PLANE));
}

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.Pos = mul(ubo.projection, mul(ubo.view, mul(ubo.model, input.Pos)));
    // Vertex position in world space
    output.WorldPos = mul(ubo.model, input.Pos).xyz;
    // GL to Vulkan coord space
    output.WorldPos.y = -output.WorldPos.y;
    // Normal in world space
    output.Normal = mul((float3x3)ubo.model, normalize(input.Normal));
    // Currently just vertex color
    output.Color = input.Color;
    return output;
}

[shader("fragment")]
FSOutput fragmentMain(VSOutput input)
{
	FSOutput output;
	output.Position = float4(input.WorldPos, 1.0);
	float3 N = normalize(input.Normal);
	N.y = -N.y;
	output.Normal = float4(N, 1.0);
	output.Albedo.rgb = input.Color;
    // Store linearized depth in alpha component
	output.Position.a = linearDepth(input.Pos.z);
	// Write color attachments to avoid undefined behaviour (validation error)
	output.Color = float4(0.0);
	return output;
}