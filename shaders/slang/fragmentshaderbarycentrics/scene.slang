/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSInput
{
    float3 Pos;
    float3 Color;
}

struct VSOutput
{
    float4 Pos : SV_Position;
    float3 Color;
}

struct UBO
{
    float4x4 projection;
    float4x4 modelview;
};
ConstantBuffer<UBO> ubo;

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.Color = input.Color;
    output.Pos = mul(ubo.projection, mul(ubo.modelview, float4(input.Pos, 1.0)));
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input, float3 baryCoords: SV_Barycentrics)
{	
    // Use barycentric coordinates to emulate a wireframe overlay by drawing thick black borders at the triangle edges
    if (baryCoords.x < 0.02 || baryCoords.y < 0.02 || baryCoords.z < 0.02) {
        return float4(input.Color, 1.0) * 2.0;
    } else {
        return float4(input.Color, 1.0) * 0.5;
    }
}