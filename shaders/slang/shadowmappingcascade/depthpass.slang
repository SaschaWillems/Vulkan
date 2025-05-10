/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSInput
{
    float3 Pos;
    float2 UV;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float2 UV;
};

// todo: pass via specialization constant
#define SHADOW_MAP_CASCADE_COUNT 4

struct UBO {
    float4x4 cascadeViewProjMat[SHADOW_MAP_CASCADE_COUNT];
};
[[vk::binding(3, 0)]] ConstantBuffer<UBO> ubo;

[[vk::binding(0, 1)]] Sampler2D colorMapSampler : register(s0, space1);

[shader("vertex")]
VSOutput vertexMain(VSInput input, uniform float4 meshPosition, uniform uint cascadeIndex)
{
    VSOutput output;
    output.UV = input.UV;
    float3 pos = input.Pos + meshPosition.xyz;
    output.Pos = mul(ubo.cascadeViewProjMat[cascadeIndex], float4(pos, 1.0));
    return output;
}

[shader("fragment")]
void fragmentMain(VSOutput input)
{
    float alpha = colorMapSampler.Sample(input.UV).a;
	if (alpha < 0.5) {
		clip(-1);
	}
}