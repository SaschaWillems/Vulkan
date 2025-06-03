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

struct UBO
{
    float4x4 projection[2];
    float4x4 modelview[2];
    float4 lightPos;	
	float distortionAlpha;
};
ConstantBuffer<UBO> ubo;

Sampler2DArray samplerView;

[[SpecializationConstant]] const float VIEW_LAYER = 0.0f;

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
	const float alpha = ubo.distortionAlpha;

    float2 p1 = float2(2.0 * input.UV - 1.0);
	float2 p2 = p1 / (1.0 - alpha * length(p1));
	p2 = (p2 + 1.0) * 0.5;

    bool inside = ((p2.x >= 0.0) && (p2.x <= 1.0) && (p2.y >= 0.0) && (p2.y <= 1.0));
    return inside ? samplerView.Sample(float3(p2, VIEW_LAYER)) : float4(0.0, 0.0, 0.0, 0.0);
}