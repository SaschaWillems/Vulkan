/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSInput
{
	float3 Pos;
	float3 Normal;
	float3 Color;
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
    float4x4 view;
    float4x4 model;
};
ConstantBuffer<UBO> ubo;

struct Node
{
    float4x4 transform;
};
[[vk::binding(0,1)]] ConstantBuffer<Node> node;

[shader("vertex")]
VSOutput vertexMain(VSInput input, uniform float4 baseColorFactor)
{
    VSOutput output;
    output.Normal = input.Normal;
    output.Color = baseColorFactor.rgb;
    float4 pos = float4(input.Pos, 1.0);
    output.Pos = mul(ubo.projection, mul(ubo.view, mul(ubo.model, mul(node.transform, pos))));

    output.Normal = mul((float4x3)mul(ubo.view, mul(ubo.model, node.transform)), input.Normal).xyz;

    float4 localpos = mul(ubo.view, mul(ubo.model, mul(node.transform, pos)));
    float3 lightPos = float3(10.0f, -10.0f, 10.0f);
    output.LightVec = lightPos.xyz - localpos.xyz;
    output.ViewVec = -localpos.xyz;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
	float3 N = normalize(input.Normal);
	float3 L = normalize(input.LightVec);
	float3 V = normalize(input.ViewVec);
	float3 R = reflect(-L, N);
	float3 ambient = float3(0.1, 0.1, 0.1);
	float3 diffuse = max(dot(N, L), 0.0) * float3(1.0, 1.0, 1.0);
	float3 specular = pow(max(dot(R, V), 0.0), 16.0) * float3(0.75, 0.75, 0.75);
	return float4((ambient + diffuse) * input.Color.rgb + specular, 1.0);
}