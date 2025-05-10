/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSInput
{
    float3 Pos;
    float3 Color;
    float3 Normal;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float3 Normal;
    float3 Color;
    float3 LightVec;
};

struct UBO
{
    float4x4 projection;
    float4x4 model;
    float4 lightPos;
};
ConstantBuffer<UBO> ubo;

Sampler2D samplerColorMap;

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.Color = float3(1.0, 0.0, 0.0);
    output.Pos = mul(ubo.projection, mul(ubo.model, float4(input.Pos.xyz, 1.0)));
    output.Normal = mul((float3x3)ubo.model, input.Normal);
    float4 pos = mul(ubo.model, float4(input.Pos, 1.0));
    float3 lPos = mul((float3x3)ubo.model, ubo.lightPos.xyz);
    output.LightVec = lPos - pos.xyz;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
	float3 color;
	float3 N = normalize(input.Normal);
	float3 L = normalize(input.LightVec);
	float intensity = dot(N,L);
	if (intensity > 0.98)
		color = input.Color * 1.5;
	else if  (intensity > 0.9)
		color = input.Color * 1.0;
	else if (intensity > 0.5)
		color = input.Color * 0.6;
	else if (intensity > 0.25)
		color = input.Color * 0.4;
	else
		color = input.Color * 0.2;
	// Desaturate a bit
	color = lerp(color, dot(float3(0.2126,0.7152,0.0722), color).xxx, 0.1);
	return float4(color, 1);
}