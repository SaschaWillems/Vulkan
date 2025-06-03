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
    float Visible;
    float3 ViewVec;
    float3 LightVec;
};

struct UBO
{
    float4x4 projection;
    float4x4 view;
    float4x4 model;
    float4 color;
    float4 lightPos;
    float visible;
};
ConstantBuffer<UBO> ubo;

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.Color = input.Color * ubo.color.rgb;
    output.Visible = ubo.visible;
    float4x4 modelView = mul(ubo.view, ubo.model);
    output.Pos = mul(ubo.projection, mul(modelView, float4(input.Pos.xyz, 1.0)));
    float4 pos = mul(ubo.model, float4(input.Pos, 1.0));
    output.Normal = mul((float3x3)ubo.model, input.Normal);
    output.LightVec = ubo.lightPos.xyz - pos.xyz;
    output.ViewVec = -pos.xyz;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
	if (input.Visible > 0.0)
	{
		float3 N = normalize(input.Normal);
		float3 L = normalize(input.LightVec);
		float3 V = normalize(input.ViewVec);
		float3 R = reflect(-L, N);
		float3 diffuse = max(dot(N, L), 0.25) * input.Color;
		float3 specular = pow(max(dot(R, V), 0.0), 8.0) * float3(0.75);
		return float4(diffuse + specular, 1.0);
	}
	else
	{
		return float4(float3(0.1, 0.1, 0.1), 1.0);
	}
}