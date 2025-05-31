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
    float3 UVW;
};

struct UBO
{
    float4x4 projection;
    float4x4 model;
};
ConstantBuffer<UBO> ubo;

struct UBOParams {
	float4 lights[4];
	float exposure;
	float gamma;
};
ConstantBuffer<UBOParams> uboParams;

SamplerCube samplerEnv;

// From http://filmicworlds.com/blog/filmic-tonemapping-operators/
float3 Uncharted2Tonemap(float3 color)
{
	float A = 0.15;
	float B = 0.50;
	float C = 0.10;
	float D = 0.20;
	float E = 0.02;
	float F = 0.30;
	float W = 11.2;
	return ((color*(A*color+C*B)+D*E)/(color*(A*color+B)+D*F))-E/F;
}

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.UVW = input.Pos;
    output.Pos = mul(ubo.projection, mul(ubo.model, float4(input.Pos.xyz, 1.0)));
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
    float3 color = samplerEnv.Sample(input.UVW).rgb;

	// Tone mapping
	color = Uncharted2Tonemap(color * uboParams.exposure);
	color = color * (1.0f / Uncharted2Tonemap((11.2f).xxx));
	// Gamma correction
	color = pow(color, (1.0f / uboParams.gamma).xxx);

	return float4(color, 1.0);
}