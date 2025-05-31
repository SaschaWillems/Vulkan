/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSInput
{
    float3 Pos;
    float3 Normal;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float3 WorldPos;
    float3 Normal;
    float LodBias;
    float3 ViewVec;
    float3 LightVec;
};

struct UBO
{
	float4x4 projection;
	float4x4 model;
	float4x4 invModel;
	float lodBias;
};
ConstantBuffer<UBO> ubo;

SamplerCube samplerColor;

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.Pos = mul(ubo.projection, mul(ubo.model, float4(input.Pos.xyz, 1.0)));

    output.WorldPos = mul(ubo.model, float4(input.Pos, 1.0)).xyz;
    output.Normal = mul((float3x3)ubo.model, input.Normal);
    output.LodBias = ubo.lodBias;

    float3 lightPos = float3(0.0f, -5.0f, 5.0f);
    output.LightVec = lightPos.xyz - output.WorldPos.xyz;
    output.ViewVec = -output.WorldPos;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
	float3 cI = normalize (input.ViewVec);
	float3 cR = reflect (cI, normalize(input.Normal));

	cR = mul(ubo.invModel, float4(cR, 0.0)).xyz;
	// Convert cubemap coordinates into Vulkan coordinate space
	cR.z *= -1.0;

    float4 color = samplerColor.SampleLevel(cR, input.LodBias);

	float3 N = normalize(input.Normal);
	float3 L = normalize(input.LightVec);
	float3 V = normalize(input.ViewVec);
	float3 R = reflect(-L, N);
	float3 ambient = float3(0.5, 0.5, 0.5) * color.rgb;
	float3 diffuse = max(dot(N, L), 0.0) * float3(1.0, 1.0, 1.0);
	float3 specular = pow(max(dot(R, V), 0.0), 16.0) * float3(0.5, 0.5, 0.5);
	return float4(ambient + diffuse * color.rgb + specular, 1.0);
}