/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSInput
{
    float4 Pos;
    float2 UV;
    float3 Color;
    float3 Normal;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float3 Normal;
    float2 UV;
    float3 Color;
    float3 ViewVec;
    float3 LightVec;
};

struct UBO
{
    float4x4 projection;
    float4x4 view;
    float4x4 model;
};
ConstantBuffer<UBO> ubo;

Sampler2D colorMapSampler;

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.Normal = input.Normal;
    output.Color = input.Color;
    output.UV = input.UV;
    output.Pos = mul(ubo.projection, mul(ubo.view, mul(ubo.model, input.Pos)));

    float3 lightPos = float3(-5.0, -5.0, 0.0);
    float4 pos = mul(ubo.view, mul(ubo.model, input.Pos));
    output.Normal = mul((float4x3)mul(ubo.view, ubo.model), input.Normal).xyz;
    output.LightVec = lightPos - pos.xyz;
    output.ViewVec = -pos.xyz;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
	float3 ambient = float3(0.0f, 0.0f, 0.0f);

	// Adjust light calculations for glow color
	if ((input.Color.r >= 0.9) || (input.Color.g >= 0.9) || (input.Color.b >= 0.9))
	{
		ambient = input.Color * 0.25;
	}

	float3 N = normalize(input.Normal);
	float3 L = normalize(input.LightVec);
	float3 V = normalize(input.ViewVec);
	float3 R = reflect(-L, N);
	float3 diffuse = max(dot(N, L), 0.0) * input.Color;
	float3 specular = pow(max(dot(R, V), 0.0), 8.0) * float3(0.75f, 0.75f, 0.75f);
	return float4(ambient + diffuse + specular, 1.0);
}