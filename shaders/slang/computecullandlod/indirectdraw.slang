/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSInput
{
    float4 Pos : POSITION0;
    float3 Normal;
    float3 Color;
    // Instanced attributes
    float3 instancePos;
    float instanceScale;    
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float3 Normal;
    float3 Color;
    float3 ViewVec;
    float3 LightVec;
};

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
    output.Normal = input.Normal;
    float4 pos = float4((input.Pos.xyz * input.instanceScale) + input.instancePos, 1.0);
    output.Pos = mul(ubo.projection, mul(ubo.modelview, pos));
    float4 wPos = mul(ubo.modelview, float4(pos.xyz, 1.0));
    float4 lPos = float4(0.0, 10.0, 50.0, 1.0);
    output.LightVec = lPos.xyz - pos.xyz;
    output.ViewVec = -pos.xyz;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
	float3 N = normalize(input.Normal);
	float3 L = normalize(input.LightVec);
	float3 ambient = float3(0.25, 0.25, 0.25);
	float3 diffuse = max(dot(N, L), 0.0).xxx;
	return float4((ambient + diffuse) * input.Color, 1.0);
}