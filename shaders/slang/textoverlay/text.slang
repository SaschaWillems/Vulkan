/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

 struct VSInput
{
    float2 Pos;
    float2 UV;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float2 UV;
};

Sampler2D samplerFont;

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.Pos = float4(input.Pos, 0.0, 1.0);
    output.UV = input.UV;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
	float color = samplerFont.Sample(input.UV).r;
	return color.xxxx;
}
