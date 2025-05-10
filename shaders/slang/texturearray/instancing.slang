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
    float3 UV;
};

struct Instance
{
    float4x4 model;
    float arrayIndex;
};

struct UBO
{
    float4x4 projection;
    float4x4 view;
    Instance instance[8];
};
ConstantBuffer<UBO> ubo;

Sampler2DArray samplerArray;

[shader("vertex")]
VSOutput vertexMain(VSInput input, uint InstanceIndex: SV_InstanceID)
{
    VSOutput output;
    output.UV = float3(input.UV, ubo.instance[InstanceIndex].arrayIndex);
    float4x4 modelView = mul(ubo.view, ubo.instance[InstanceIndex].model);
    output.Pos = mul(ubo.projection, mul(modelView, float4(input.Pos, 1.0)));
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
	return samplerArray.Sample(input.UV);
}