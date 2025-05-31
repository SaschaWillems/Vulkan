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
    float3 Tangent;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float3 Normal;
    float2 UV;
    float3 Color;
    float3 WorldPos;
    float3 Tangent;
};

struct FSOutput
{
	float4 Position;
	float4 Normal;
	float4 Albedo;
};

struct UBO
{
    float4x4 projection;
    float4x4 model;
    float4x4 view;
    float4 instancePos[3];
};
ConstantBuffer<UBO> ubo;

Sampler2D samplerColor;
Sampler2D samplerNormalMap;

[shader("vertex")]
VSOutput vertexMain(VSInput input, uint InstanceIndex: SV_InstanceID)
{
    VSOutput output;
    float4 tmpPos = input.Pos + ubo.instancePos[InstanceIndex];

    output.Pos = mul(ubo.projection, mul(ubo.view, mul(ubo.model, tmpPos)));

    output.UV = input.UV;

    // Vertex position in world space
    output.WorldPos = mul(ubo.model, tmpPos).xyz;

    // Normal in world space
    output.Normal = normalize(input.Normal);
    output.Tangent = normalize(input.Tangent);

    // Currently just vertex color
    output.Color = input.Color;
    return output;
}

[shader("fragment")]
FSOutput fragmentMain(VSOutput input)
{
	FSOutput output;
	output.Position = float4(input.WorldPos, 1.0);

	// Calculate normal in tangent space
	float3 N = normalize(input.Normal);
	float3 T = normalize(input.Tangent);
	float3 B = cross(N, T);
	float3x3 TBN = float3x3(T, B, N);
	float3 tnorm = mul(normalize(samplerNormalMap.Sample(input.UV).xyz * 2.0 - float3(1.0, 1.0, 1.0)), TBN);
	output.Normal = float4(tnorm, 1.0);

	output.Albedo = samplerColor.Sample(input.UV);
	return output;
}