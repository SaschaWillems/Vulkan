/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSInput
{
	float3 Pos : POSITION0;
	float3 Normal;
	float3 Color;
};

struct UBO
{
	float4x4 projection;
	float4x4 model;
	float4x4 view;
};
ConstantBuffer<UBO> ubo;

struct VSOutput
{
	float4 Pos : SV_POSITION;
	float3 Color;
};

// Uniform entry-point parameters are automatically bound to push constants by slang
[shader("vertex")]
VSOutput vertexMain(VSInput input, uniform float4 pushColor, uniform float4 pushPosition)
{
    VSOutput output;
    output.Color = input.Color * pushColor.rgb;
    float3 locPos = float3(mul(ubo.model, float4(input.Pos.xyz, 1.0)).xyz);
    float3 worldPos = locPos + pushPosition.xyz;
	output.Pos = mul(ubo.projection, mul(ubo.view, float4(worldPos.xyz, 1.0)));
	return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
    return float4(input.Color, 1.0);
}