/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSInput
{
    float3 Pos;
    float2 UV;
    float3 Color;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float3 Color;
    float2 UV;
};

struct UBO
{
    float4x4 projection;
    float4x4 model;
    float gradientPos;
};
ConstantBuffer<UBO> ubo;

Sampler2D samplerGradientRamp;

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.Color = input.Color;
    output.UV = float2(ubo.gradientPos, 0.0f);
    output.Pos = mul(ubo.projection, mul(ubo.model, float4(input.Pos, 1.0)));
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
	// Use max. color channel value to detect bright glow emitters
	if ((input.Color.r >= 0.9) || (input.Color.g >= 0.9) || (input.Color.b >= 0.9))
    {
        return float4(samplerGradientRamp.Sample(input.UV).rgb, 1);
	} else {
		return float4(input.Color, 1);
	}
}