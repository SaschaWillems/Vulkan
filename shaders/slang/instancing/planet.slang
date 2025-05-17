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
    float4x4 modelview;
    float4 lightPos;
};
ConstantBuffer<UBO> ubo;

Sampler2D samplerColorMap;

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.Color = input.Color;
    output.UV = input.UV;
    output.Pos = mul(ubo.projection, mul(ubo.modelview, float4(input.Pos.xyz, 1.0)));

    float4 pos = mul(ubo.modelview, float4(input.Pos, 1.0));
    output.Normal = mul((float3x3)ubo.modelview, input.Normal);
    float3 lPos = mul((float3x3)ubo.modelview, ubo.lightPos.xyz);
    output.LightVec = lPos - pos.xyz;
    output.ViewVec = -pos.xyz;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
    float4 color = samplerColorMap.Sample(input.UV) * float4(input.Color, 1.0) * 1.5;
	float3 N = normalize(input.Normal);
	float3 L = normalize(input.LightVec);
	float3 V = normalize(input.ViewVec);
	float3 R = reflect(-L, N);
	float3 diffuse = max(dot(N, L), 0.0) * input.Color;
	float3 specular = pow(max(dot(R, V), 0.0), 4.0) * float3(0.5, 0.5, 0.5) * color.r;
	return float4(diffuse * color.rgb + specular, 1.0);
}