/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSInput
{
    float3 Pos;
    float2 UV;
    float3 Normal;
    float4 Tangent;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float2 UV;
    float3 LightVec;
    float3 LightVecB;
    float3 LightDir;
    float3 ViewVec;
};

struct UBO
{
    float4x4 projection;
    float4x4 model;
    float4x4 normal;
    float4 lightPos;
};
ConstantBuffer<UBO> ubo;

Sampler2D samplerColorMap;
Sampler2D samplerNormalHeightMap;

#define lightRadius 45.0

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    float3 vertexPosition = mul(ubo.model, float4(input.Pos, 1.0)).xyz;
    output.LightDir = normalize(ubo.lightPos.xyz - vertexPosition);

    float3 biTangent = cross(input.Normal, input.Tangent.xyz);

    // Setup (t)angent-(b)inormal-(n)ormal matrix for converting
    // object coordinates into tangent space
    float3x3 tbnMatrix;
    tbnMatrix[0] = mul((float3x3)ubo.normal, input.Tangent.xyz);
    tbnMatrix[1] = mul((float3x3)ubo.normal, biTangent);
    tbnMatrix[2] = mul((float3x3)ubo.normal, input.Normal);

    output.LightVec.xyz = mul(float3(ubo.lightPos.xyz - vertexPosition), tbnMatrix);

    float3 lightDist = ubo.lightPos.xyz - input.Pos;
    output.LightVecB.x = dot(input.Tangent.xyz, lightDist);
    output.LightVecB.y = dot(biTangent, lightDist);
    output.LightVecB.z = dot(input.Normal, lightDist);

    output.ViewVec.x = dot(input.Tangent.xyz, input.Pos);
    output.ViewVec.y = dot(biTangent, input.Pos);
    output.ViewVec.z = dot(input.Normal, input.Pos);

    output.UV = input.UV;

    output.Pos = mul(ubo.projection, mul(ubo.model, float4(input.Pos, 1.0)));
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
	float3 specularColor = float3(0.85, 0.5, 0.0);

	float invRadius = 1.0/lightRadius;
	float ambient = 0.25;

	float3 rgb, normal;

    rgb = samplerColorMap.Sample(input.UV).rgb;
    normal = normalize((samplerNormalHeightMap.Sample(input.UV).rgb - 0.5) * 2.0);

	float distSqr = dot(input.LightVecB, input.LightVecB);
	float3 lVec = input.LightVecB * rsqrt(distSqr);

	float atten = max(clamp(1.0 - invRadius * sqrt(distSqr), 0.0, 1.0), ambient);
	float diffuse = clamp(dot(lVec, normal), 0.0, 1.0);

	float3 light = normalize(-input.LightVec);
	float3 view = normalize(input.ViewVec);
	float3 reflectDir = reflect(-light, normal);

	float specular = pow(max(dot(view, reflectDir), 0.0), 4.0);

	return float4((rgb * atten + (diffuse * rgb + 0.5 * specular * specularColor.rgb)) * atten, 1.0);
}