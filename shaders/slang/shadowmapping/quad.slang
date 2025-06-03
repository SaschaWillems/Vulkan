/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct UBO
{
	float4x4 projection;
	float4x4 view;
	float4x4 model;
	float4x4 lightSpace;
	float4 lightPos;
	float zNear;
	float zFar;
};
ConstantBuffer<UBO> ubo;
Sampler2D samplerColor;

float LinearizeDepth(float depth)
{
  float n = ubo.zNear;
  float f = ubo.zFar;
  float z = depth;
  return (2.0 * n) / (f + n - z * (f - n));
}

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
float4 fragmentMain(VSOutput input)
{
    float depth = samplerColor.Sample(input.UV).r;
    return float4((1.0 - LinearizeDepth(depth)).xxx, 1.0);
}
