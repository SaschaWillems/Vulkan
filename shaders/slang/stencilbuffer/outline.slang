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
}

struct UBO
{
	float4x4 projection;
	float4x4 model;
	float4 lightPos;
	float outlineWidth;
};
ConstantBuffer<UBO> ubo;

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    // Extrude along normal
    VSOutput output;
	float4 pos = float4(input.Pos.xyz + input.Normal * ubo.outlineWidth, input.Pos.w);
    output.Pos = mul(ubo.projection, mul(ubo.model, pos));
    return output;
}

[shader("fragment")]
float4 fragmentMain()
{
	return float4(1.0, 1.0, 1.0, 1.0);
}