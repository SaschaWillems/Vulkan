/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSInput
{
	float4 Pos;
	float3 Color;
	float3 Normal;
	float2 UV;
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
    float4x4 view;
};
ConstantBuffer<UBO> ubo;

[[vk::input_attachment_index(0)]] SubpassInput samplerPositionDepth;

Sampler2D samplerTexture;

[[SpecializationConstant]] const float NEAR_PLANE = 0.1f;
[[SpecializationConstant]] const float FAR_PLANE = 256.0f;

float linearDepth(float depth)
{
    float z = depth * 2.0f - 1.0f;
    return (2.0f * NEAR_PLANE * FAR_PLANE) / (FAR_PLANE + NEAR_PLANE - z * (FAR_PLANE - NEAR_PLANE));
}

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.Color = input.Color;
    output.UV = input.UV;
    output.Pos = mul(ubo.projection, mul(ubo.view, mul(ubo.model, float4(input.Pos.xyz, 1.0))));
    return output;
} 

[shader("fragment")]
float4 fragmentMain (VSOutput input)
{
	// Sample depth from deferred depth buffer and discard if obscured
	float depth = samplerPositionDepth.SubpassLoad().a;
	if ((depth != 0.0) && (linearDepth(input.Pos.z) > depth))
	{
		clip(-1);
	};

    return samplerTexture.Sample(input.UV);
}
