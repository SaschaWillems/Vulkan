/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSInput
{
	float3 Pos : POSITION0;
	float2 UV;
	int TextureIndex;
};

struct Matrices {
	float4x4 projection;
	float4x4 view;
	float4x4 model;
};
ConstantBuffer<Matrices> matrices;
Sampler2D textures[];

struct VSOutput
{
	float4 Pos : SV_POSITION;
	int TextureIndex;
	float2 UV;
};

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
	VSOutput output;
	output.UV = input.UV;
	output.TextureIndex = input.TextureIndex;
	output.Pos = mul(matrices.projection, mul(matrices.view, mul(matrices.model, float4(input.Pos.xyz, 1.0))));
	return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
    return textures[NonUniformResourceIndex(input.TextureIndex)].Sample(input.UV);
}