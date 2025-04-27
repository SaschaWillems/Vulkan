/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSInput
{
	float3 Pos;
	float2 UV;
	float3 Normal;
};

struct VSOutput
{
	float4 Pos : SV_POSITION;
	float2 UV;
	float LodBias;
	float3 Normal;
	float3 ViewVec;
	float3 LightVec;
};

struct UBO
{
    float4x4 projection;
    float4x4 view;
    float4x4 model;
    float4 viewPos;
    float lodBias;
    int samplerIndex;	
};
ConstantBuffer<UBO> ubo;

Texture2D textureColor;
SamplerState samplers[3];

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.UV = input.UV * float2(2.0, 1.0);
    output.LodBias = ubo.lodBias;

    float3 worldPos = mul(ubo.model, float4(input.Pos, 1.0)).xyz;

    output.Pos = mul(ubo.projection, mul(ubo.view, mul(ubo.model, float4(input.Pos.xyz, 1.0))));

    output.Normal = mul((float3x3)ubo.model, input.Normal);
    float3 lightPos = float3(-30.0, 0.0, 0.0);
    output.LightVec = worldPos - lightPos;
    output.ViewVec = ubo.viewPos.xyz - worldPos;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
    float4 color = textureColor.Sample(samplers[ubo.samplerIndex], input.UV, int2(0, 0), input.LodBias);

    float3 N = normalize(input.Normal);
    float3 L = normalize(input.LightVec);
    float3 V = normalize(input.ViewVec);
    float3 R = reflect(L, N);
    float3 diffuse = max(dot(N, L), 0.65) * float3(1.0, 1.0, 1.0);
    float specular = pow(max(dot(R, V), 0.0), 16.0) * color.a;
    return float4(diffuse * color.r + specular, 1.0);
}