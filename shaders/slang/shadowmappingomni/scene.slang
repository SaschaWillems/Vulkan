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
    float3 EyePos;
    float3 LightVec;
    float3 WorldPos;
    float3 LightPos;
};

struct UBO
{
    float4x4 projection;
    float4x4 view;
    float4x4 model;
    float4 lightPos;
};
ConstantBuffer<UBO> ubo;

SamplerCube shadowCubeMapSampler;

#define EPSILON 0.15
#define SHADOW_OPACITY 0.5

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.Color = input.Color;
    output.Normal = input.Normal;

    output.Pos = mul(ubo.projection, mul(ubo.view, mul(ubo.model, float4(input.Pos.xyz, 1.0))));
    output.EyePos = mul(ubo.model, float4(input.Pos, 1.0f)).xyz;
    output.LightVec = normalize(ubo.lightPos.xyz - input.Pos.xyz);
    output.WorldPos = input.Pos;

    output.LightPos = ubo.lightPos.xyz;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
	// Lighting
	float3 N = normalize(input.Normal);
	float3 L = normalize(float3(1.0, 1.0, 1.0));

	float3 Eye = normalize(-input.EyePos);
	float3 Reflected = normalize(reflect(-input.LightVec, input.Normal));

	float4 IAmbient = float4(float3(0.05, 0.05, 0.05), 1.0);
	float4 IDiffuse = float4(1.0, 1.0, 1.0, 1.0) * max(dot(input.Normal, input.LightVec), 0.0);

	float4 outFragColor = float4(IAmbient + IDiffuse * float4(input.Color, 1.0));

	// Shadow
    float3 lightVec = input.WorldPos - input.LightPos;
    float sampledDist = shadowCubeMapSampler.Sample(lightVec).r;
    float dist = length(lightVec);

	// Check if fragment is in shadow
    float shadow = (dist <= sampledDist + EPSILON) ? 1.0 : SHADOW_OPACITY;

	outFragColor.rgb *= shadow;
	return outFragColor;
}