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
	float blurScale;
	float blurStrength;
};
ConstantBuffer<UBO> ubo;

Sampler2D samplerColor;

[[SpecializationConstant]] const int blurdirection = 0;

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
	float weight[5];
	weight[0] = 0.227027;
	weight[1] = 0.1945946;
	weight[2] = 0.1216216;
	weight[3] = 0.054054;
	weight[4] = 0.016216;

    float2 textureSize;
    samplerColor.GetDimensions(textureSize.x, textureSize.y);
    float2 tex_offset = 1.0 / textureSize * ubo.blurScale;        	// gets size of single texel
    float3 result = samplerColor.Sample(input.UV).rgb * weight[0]; 	// current fragment's contribution
	for(int i = 1; i < 5; ++i)
	{
		if (blurdirection == 1)
		{
            // H
            result += samplerColor.Sample(input.UV + float2(tex_offset.x * i, 0.0)).rgb * weight[i] * ubo.blurScale;
            result += samplerColor.Sample(input.UV - float2(tex_offset.x * i, 0.0)).rgb * weight[i] * ubo.blurScale;
		}
		else
		{
            // V
            result += samplerColor.Sample(input.UV + float2(0.0, tex_offset.y * i)).rgb * weight[i] * ubo.blurScale;
            result += samplerColor.Sample(input.UV - float2(0.0, tex_offset.y * i)).rgb * weight[i] * ubo.blurScale;
		}
	}
	return float4(result, 1.0);
}