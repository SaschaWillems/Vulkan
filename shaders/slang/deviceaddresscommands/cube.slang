/* Copyright (c) 2026, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSInput
{
    float3 Pos;
    float3 Color;
};

struct Matrices {
    float4x4 camera;
    float4x4 models[2];
}

// Pointers to the matrices, addresses are set in the application
struct PushConsts {
    Matrices *matrices;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float3 Color;
};

[shader("vertex")]
VSOutput vertexMain(VSInput input, uniform PushConsts pushConstants, uint InstanceIndex: SV_VulkanInstanceID) 
{
    VSOutput output;
    output.Color = input.Color;
    output.Pos = mul(pushConstants.matrices->camera, mul(pushConstants.matrices->models[InstanceIndex], float4(input.Pos.xyz, 1.0)));
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
    return float4(input.Color, 1.0);
}