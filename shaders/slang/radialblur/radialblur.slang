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
    float radialBlurScale;
    float radialBlurStrength;
    float2 radialOrigin;
};
ConstantBuffer<UBO> ubo;

Sampler2D samplerColor;

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
    int2 texDim;
    samplerColor.GetDimensions(texDim.x, texDim.y);
	float2 radialSize = float2(1.0 / texDim.x, 1.0 / texDim.y);
	float2 UV = input.UV;
	float4 color = float4(0.0, 0.0, 0.0, 0.0);
	UV += radialSize * 0.5 - ubo.radialOrigin;
	#define samples 32
	for (int i = 0; i < samples; i++)
	{
        float scale = 1.0 - ubo.radialBlurScale * (float(i) / float(samples - 1));
        color += samplerColor.Sample(UV * scale + ubo.radialOrigin);
	}
	return (color / samples) * ubo.radialBlurStrength;
}