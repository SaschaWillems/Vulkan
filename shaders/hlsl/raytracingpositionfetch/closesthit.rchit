/* Copyright (c) 2024, Sascha Willems
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

struct UBO
{
    float4x4 viewInverse;
    float4x4 projInverse;
    float4 lightPos;
};
ConstantBuffer<UBO> ubo : register(b2);

// We need to use special syntax for SPIR-V inlines
#define HitTriangleVertexPositionsKHR 5335
#define RayTracingPositionFetchKHR 5336

[[vk::ext_extension("SPV_KHR_ray_tracing_position_fetch")]]
[[vk::ext_capability(RayTracingPositionFetchKHR)]]
[[vk::ext_builtin_input(HitTriangleVertexPositionsKHR)]]
const static float3 gl_HitTriangleVertexPositions[3];

[shader("closesthit")]
void main(inout Payload p, in Attributes attribs)
{
	// We need the barycentric coordinates to calculate data for the current position
    const float3 barycentricCoords = float3(1.0f - attribs.bary.x - attribs.bary.y, attribs.bary.x, attribs.bary.y);

	// With VK_KHR_ray_tracing_position_fetch we can access the vertices for the hit triangle in the shader

    float3 vertexPos0 = gl_HitTriangleVertexPositions[0];
    float3 vertexPos1 = gl_HitTriangleVertexPositions[1];
    float3 vertexPos2 = gl_HitTriangleVertexPositions[2];
    float3 currentPos = vertexPos0 * barycentricCoords.x + vertexPos1 * barycentricCoords.y + vertexPos2 * barycentricCoords.z;

	// Calcualte the normal from above values
    float3 normal = normalize(cross(vertexPos1 - vertexPos0, vertexPos2 - vertexPos0));
    normal = normalize(mul(float4(normal, 1.0), WorldToObject4x3()));

	// Basic lighting
    float3 lightDir = normalize(ubo.lightPos.xyz - currentPos);
    float diffuse = max(dot(normal, lightDir), 0.0);

    p.hitValue.rgb = 0.1 + diffuse;
}