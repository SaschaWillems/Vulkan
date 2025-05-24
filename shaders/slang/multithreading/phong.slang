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

[shader("vertex")]
VSOutput vertexMain(VSInput input, uniform float4x4 mvp, uniform float3 color)
{
    VSOutput output;
    if ((input.Color.r == 1.0) && (input.Color.g == 0.0) && (input.Color.b == 0.0))
    {
        output.Color = color;
    }
    else
    {
        output.Color = input.Color;
    }
    output.Pos = mul(mvp, float4(input.Pos.xyz, 1.0));
    float4 pos = mul(mvp, float4(input.Pos, 1.0));
    output.Normal = mul((float3x3)mvp, input.Normal);
    float3 lPos = float3(0.0, 0.0, 0.0);
    output.LightVec = lPos - pos.xyz;
    output.ViewVec = -pos.xyz;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
	float3 N = normalize(input.Normal);
	float3 L = normalize(input.LightVec);
	float3 V = normalize(input.ViewVec);
	float3 R = reflect(-L, N);
	float3 diffuse = max(dot(N, L), 0.0) * input.Color;
	float3 specular = pow(max(dot(R, V), 0.0), 8.0) * float3(0.75, 0.75, 0.75);
	return float4(diffuse + specular, 1.0);
}