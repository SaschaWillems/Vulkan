/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSInput
{
    float3 Pos;
    float3 Normal;
    float2 UV;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float3 Normal;
    float2 UV;
};

struct DSOutput
{
	float3 Normal;
	float2 UV;
};

[[vk::binding(0, 1)]] Sampler2D samplerColorMap;

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.Pos = float4(input.Pos.xyz, 1.0);
    output.Normal = input.Normal;
    output.UV = input.UV;
    return output;
}

[shader("fragment")]
float4 fragmentMain(DSOutput input)
{
	float3 N = normalize(input.Normal);
    float3 L = normalize(float3(0.0, -4.0, 4.0));
    float4 color = samplerColorMap.Sample(input.UV);
	return float4(clamp(max(dot(N,L), 0.0), 0.2, 1.0) * color.rgb * 1.5, 1);
}
