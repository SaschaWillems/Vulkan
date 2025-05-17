/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct Attributes
{
    float2 bary;
};

struct Payload
{
    [[vk::location(0)]] float3 hitValue;
};

RaytracingAccelerationStructure accelStruct;
RWTexture2D<float4> image;
struct UBO
{
    float4x4 viewInverse;
    float4x4 projInverse;
    float4 lightPos;
};
ConstantBuffer<UBO> ubo;

[shader("raygeneration")]
void raygenerationMain()
{
    uint3 LaunchID = DispatchRaysIndex();
    uint3 LaunchSize = DispatchRaysDimensions();

    const float2 pixelCenter = float2(LaunchID.xy) + float2(0.5, 0.5);
    const float2 inUV = pixelCenter / float2(LaunchSize.xy);
    float2 d = inUV * 2.0 - 1.0;
    float4 target = mul(ubo.projInverse, float4(d.x, d.y, 1, 1));

    RayDesc rayDesc;
    rayDesc.Origin = mul(ubo.viewInverse, float4(0, 0, 0, 1)).xyz;
    rayDesc.Direction = mul(ubo.viewInverse, float4(normalize(target.xyz), 0)).xyz;
    rayDesc.TMin = 0.001;
    rayDesc.TMax = 10000.0;

    Payload payload;
    TraceRay(accelStruct, RAY_FLAG_FORCE_OPAQUE, 0xff, 0, 0, 0, rayDesc, payload);

    image[int2(LaunchID.xy)] = float4(payload.hitValue, 0.0);
}

[shader("closesthit")]
void closesthitMain(inout Payload payload, in Attributes attribs)
{
	// We need the barycentric coordinates to calculate data for the current position
    const float3 barycentricCoords = float3(1.0f - attribs.bary.x - attribs.bary.y, attribs.bary.x, attribs.bary.y);

	// With VK_KHR_ray_tracing_position_fetch we can access the vertices for the hit triangle in the shader

    float3 vertexPos0 = HitTriangleVertexPosition(0);
    float3 vertexPos1 = HitTriangleVertexPosition(1);
    float3 vertexPos2 = HitTriangleVertexPosition(2);
    float3 currentPos = vertexPos0 * barycentricCoords.x + vertexPos1 * barycentricCoords.y + vertexPos2 * barycentricCoords.z;

	// Calcualte the normal from above values
    float3 normal = normalize(cross(vertexPos1 - vertexPos0, vertexPos2 - vertexPos0));
    normal = normalize(mul(float4(normal, 1.0), WorldToObject4x3()));

	// Basic lighting
    float3 lightDir = normalize(ubo.lightPos.xyz - currentPos);
    float diffuse = max(dot(normal, lightDir), 0.0);

    payload.hitValue.rgb = 0.1 + diffuse;
}

[shader("miss")]
void missMain(inout Payload payload)
{
    payload.hitValue = float3(0.0, 0.0, 0.2);
}