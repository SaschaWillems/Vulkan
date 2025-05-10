/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSInput
{
    float3 Pos;
    float3 Color;
    float3 Normal;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float3 Color;
    float3 Normal;
    float3 ViewVec;
    float3 LightVec;
};

struct UBO {
    float4x4 projection;
    float4x4 model;
    float4x4 view;
};
ConstantBuffer<UBO> ubo;

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.Pos = mul(ubo.projection, mul(ubo.view, mul(ubo.model, float4(input.Pos, 1.0))));
    output.Color = input.Color;
    output.Normal = input.Normal;
    output.LightVec = float3(0.0f, 5.0f, 15.0f) - input.Pos;
    output.ViewVec = -input.Pos.xyz;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
	// Toon shading color attachment output
	float intensity = dot(normalize(input.Normal), normalize(input.LightVec));
	float shade = 1.0;
	shade = intensity < 0.5 ? 0.75 : shade;
	shade = intensity < 0.35 ? 0.6 : shade;
	shade = intensity < 0.25 ? 0.5 : shade;
	shade = intensity < 0.1 ? 0.25 : shade;

	return float4(input.Color * 3.0 * shade, 1.0);

	// Depth attachment does not need to be explicitly written
}