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
	float3 Color;
};

struct VSOutput
{
	float4 Pos : SV_POSITION;
	float3 Normal;
	float3 Color;
	float2 UV;
	float3 ViewVec;
	float3 LightVec;
};

struct UBO
{
	float4x4 projection;
	float4x4 view;
	float4 lightPos;
	float4 viewPos;
};
ConstantBuffer<UBO> ubo;

[shader("vertex")]
VSOutput vertexMain(VSInput input, uniform float4x4 modelMat)
{
	VSOutput output;
	output.Normal = input.Normal;
	output.Color = input.Color;
    output.UV = input.UV;
    output.Pos = mul(ubo.projection, mul(ubo.view, mul(modelMat, float4(input.Pos.xyz, 1.0))));

    float4 pos = mul(ubo.view, mul(modelMat, float4(input.Pos.xyz, 1.0)));
	output.Normal = mul((float3x3)ubo.view, input.Normal);
	output.LightVec = ubo.lightPos.xyz - pos.xyz;
	output.ViewVec = ubo.viewPos.xyz - pos.xyz;
	return output;
}

[[vk::binding(0, 1)]] Sampler2D samplerColorMap;

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
    float4 color = samplerColorMap.Sample(input.UV) * float4(input.Color, 1.0);

    float3 N = normalize(input.Normal);
    float3 L = normalize(input.LightVec);
    float3 V = normalize(input.ViewVec);
    float3 R = reflect(L, N);
    float3 diffuse = max(dot(N, L), 0.0) * input.Color;
    float3 specular = pow(max(dot(R, V), 0.0), 16.0) * float3(0.75, 0.75, 0.75);
    return float4(diffuse * color.rgb + specular, 1.0);
}