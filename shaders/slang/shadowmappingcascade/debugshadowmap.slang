/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

[[vk::binding(1,0)]] Sampler2DArray shadowMapSampler;

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float2 UV;
};

[shader("vertex")]
VSOutput vertexMain(uint VertexIndex: SV_VertexID)
{
	VSOutput output;
	output.UV = float2((VertexIndex << 1) & 2, VertexIndex & 2);
	output.Pos = float4(output.UV * 2.0f - 1.0f, 0.0f, 1.0f);
	return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input, uniform float4 meshPosition, uniform uint cascadeIndex)
{
    float depth = shadowMapSampler.Sample(float3(input.UV, float(cascadeIndex))).r;
	return float4(depth.xxx, 1.0);
}