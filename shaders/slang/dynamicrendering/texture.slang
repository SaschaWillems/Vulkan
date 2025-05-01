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
	float2 UV;
	float3 Normal;
	float3 ViewVec;
	float3 LightVec;
};

struct UBO
{
    float4x4 projection;
    float4x4 model;
    float4 viewPos;
};
ConstantBuffer<UBO> ubo;

[[vk::binding(0, 1)]] Sampler2D samplerColor;

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
	VSOutput output;
	output.UV = input.UV;

	float3 worldPos = mul(ubo.model, float4(input.Pos, 1.0)).xyz;

	output.Pos = mul(ubo.projection, mul(ubo.model, float4(input.Pos.xyz, 1.0)));

    float4 pos = mul(ubo.model, float4(input.Pos, 1.0));
	output.Normal = mul((float3x3)ubo.model, input.Normal);
	float3 lightPos = float3(0.0, 0.0, 0.0);
	float3 lPos = mul((float3x3)ubo.model, lightPos.xyz);
    output.LightVec = lPos - pos.xyz;
    output.ViewVec = ubo.viewPos.xyz - pos.xyz;
	return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
    float4 color = samplerColor.Sample(input.UV);

    float3 N = normalize(input.Normal);
    float3 L = normalize(input.LightVec);
    float3 V = normalize(input.ViewVec);
    float3 R = reflect(-L, N);
    float3 diffuse = max(dot(N, L), 0.0) * float3(1.0, 1.0, 1.0);
    float specular = pow(max(dot(R, V), 0.0), 16.0) * color.a;

    return float4(diffuse * color.rgb + specular, 1.0);
}