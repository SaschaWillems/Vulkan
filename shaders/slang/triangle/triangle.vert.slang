/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct UBO
{
	float4x4 projectionMatrix;
	float4x4 modelMatrix;
	float4x4 viewMatrix;
};
[[vk::binding(0, 0)]]
ConstantBuffer<UBO> ubo;

struct VSInput
{
    [[vk::location(0)]] float3 Pos;
    [[vk::location(1)]] float3 Color;
};

struct VSOutput
{
	float4 Pos : SV_POSITION;
	[[vk::location(0)]] float3 Color;
};

[shader("vertex")]
VSOutput main(VSInput input)
{
	VSOutput output;
	output.Color = input.Color;
	output.Pos = mul(ubo.projectionMatrix, mul(ubo.viewMatrix, mul(ubo.modelMatrix, float4(input.Pos.xyz, 1.0))));
	return output;
}
