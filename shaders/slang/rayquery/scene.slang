/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSInput
{	
	float3 Pos;
	float2 UV;
	float3 Color;
	float3 Normal;
}

struct VSOutput
{
    float4 Pos : SV_POSITION;
	float3 Normal;
	float3 Color;
	float3 ViewVec;
	float3 LightVec;
	float3 WorldPos;	
}

struct UBO
{
    float4x4 projection;
    float4x4 view;
    float4x4 model;
    float3 lightPos;
};
ConstantBuffer<UBO> ubo;

RaytracingAccelerationStructure accelStruct;

#define ambient 0.1

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.Color = input.Color;
    output.Pos = mul(ubo.projection, mul(ubo.view, mul(ubo.model, float4(input.Pos.xyz, 1.0))));
    float4 pos = mul(ubo.model, float4(input.Pos, 1.0));
    output.WorldPos = mul(ubo.model, float4(input.Pos, 1.0)).xyz;
    output.Normal = mul((float4x3)ubo.model, input.Normal).xyz;
    output.LightVec = normalize(ubo.lightPos - input.Pos);
    output.ViewVec = -pos.xyz;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
    float3 N = normalize(input.Normal);
    float3 L = normalize(input.LightVec);
    float3 V = normalize(input.ViewVec);
    float3 R = normalize(-reflect(L, N));
    float3 diffuse = max(dot(N, L), ambient) * input.Color;

    float4 color = float4(diffuse, 1.0);

    RayDesc rayDesc;
    rayDesc.Origin = input.WorldPos;
    rayDesc.Direction = L;
    rayDesc.TMin = 0.01;
    rayDesc.TMax = 1000.0;

    RayQuery<RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> rayQuery;
    rayQuery.TraceRayInline(accelStruct, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, 0xFF, rayDesc);

    // Traverse the acceleration structure and store information about the first intersection (if any)
    rayQuery.Proceed();

    // If the intersection has hit a triangle, the fragment is shadowed
    if (rayQuery.CommittedStatus() == COMMITTED_TRIANGLE_HIT) {
        color *= 0.1;
	}

    return color;
}
