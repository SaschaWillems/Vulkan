/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float2 UV;
};

[[vk::binding(1, 0)]] Sampler2D samplerColor;

[shader("vertex")]
VSOutput vertexMain(uint VertexIndex: SV_VertexID)
{
    VSOutput output;
    output.UV = float2((VertexIndex << 1) & 2, VertexIndex & 2);
    output.Pos = float4(output.UV * float2(2.0f, 2.0f) + float2(-1.0f, -1.0f), 0.0f, 1.0f);
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
	// Single pass gauss blur

	const float2 texOffset = float2(0.01, 0.01);

	float2 tc0 = input.UV + float2(-texOffset.x, -texOffset.y);
	float2 tc1 = input.UV + float2(         0.0, -texOffset.y);
	float2 tc2 = input.UV + float2(+texOffset.x, -texOffset.y);
	float2 tc3 = input.UV + float2(-texOffset.x,          0.0);
	float2 tc4 = input.UV + float2(         0.0,          0.0);
	float2 tc5 = input.UV + float2(+texOffset.x,          0.0);
	float2 tc6 = input.UV + float2(-texOffset.x, +texOffset.y);
	float2 tc7 = input.UV + float2(         0.0, +texOffset.y);
	float2 tc8 = input.UV + float2(+texOffset.x, +texOffset.y);

	float4 col0 = samplerColor.Sample(tc0);
	float4 col1 = samplerColor.Sample(tc1);
	float4 col2 = samplerColor.Sample(tc2);
	float4 col3 = samplerColor.Sample(tc3);
	float4 col4 = samplerColor.Sample(tc4);
	float4 col5 = samplerColor.Sample(tc5);
	float4 col6 = samplerColor.Sample(tc6);
	float4 col7 = samplerColor.Sample(tc7);
	float4 col8 = samplerColor.Sample(tc8);

	float4 sum = (1.0 * col0 + 2.0 * col1 + 1.0 * col2 +
			  2.0 * col3 + 4.0 * col4 + 2.0 * col5 +
			  1.0 * col6 + 2.0 * col7 + 1.0 * col8) / 16.0;
	return float4(sum.rgb, 1.0);
}