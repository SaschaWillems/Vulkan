/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSInput
{
    float3 Pos;
    float3 Color;
};

struct VSOutput
{
	float4 Pos : SV_POSITION;
	float3 Color;
};

struct UboView
{
    float4x4 projection;
    float4x4 view;
};
ConstantBuffer<UboView> uboView;

struct UboInstance
{
    float4x4 model;
};
ConstantBuffer<UboInstance> uboInstance;

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.Color = input.Color;
    float4x4 modelView = mul(uboView.view, uboInstance.model);
    float3 worldPos = mul(modelView, float4(input.Pos, 1.0)).xyz;
    output.Pos = mul(uboView.projection, mul(modelView, float4(input.Pos.xyz, 1.0)));
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
	return float4(input.Color, 1.0);
}