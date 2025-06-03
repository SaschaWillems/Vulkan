/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSInput
{
    float3 Pos;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float4 WorldPos;
    float3 LightPos;
};

struct UBO
{
    float4x4 projection;
    float4x4 view;
    float4x4 model;
    float4 lightPos;
};
ConstantBuffer<UBO> ubo;

[shader("vertex")]
VSOutput vertexMain(VSInput input, uniform float4x4 view)
{
    VSOutput output;
    output.Pos = mul(ubo.projection, mul(view, mul(ubo.model, float4(input.Pos, 1.0))));

    output.WorldPos = float4(input.Pos, 1.0);
    output.LightPos = ubo.lightPos.xyz;
    return output;
}

[shader("fragment")]
float fragmentMain(VSOutput input)
{
	// Store distance to light as 32 bit float value
    float3 lightVec = input.WorldPos.xyz - input.LightPos;
    return length(lightVec);
}