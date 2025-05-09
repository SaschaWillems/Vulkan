/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct Payload
{
    float3 hitValue;
}

RaytracingAccelerationStructure accelStruct;
RWTexture2D<float4> image;

struct UBO
{
    float4x4 viewInverse;
    float4x4 projInverse;
    float4 lightPos;
};
ConstantBuffer<UBO> ubo;

struct Sphere {
	float3 center;
	float radius;
	float4 color;
};
StructuredBuffer<Sphere> spheres;

// Ray-sphere intersection
// By Inigo Quilez, from https://iquilezles.org/articles/spherefunctions/
float sphIntersect(const Sphere s, float3 ro, float3 rd)
{
    float3 oc = ro - s.center;
    float b = dot(oc, rd);
    float c = dot(oc, oc) - s.radius * s.radius;
    float h = b * b - c;
    if (h < 0.0) {
        return -1.0;
    }
    h = sqrt(h);
    return -b - h;
}

[shader("intersection")]
void intersectionMain() {
    Sphere sphere = spheres[PrimitiveIndex()];
    float hit = sphIntersect(sphere, WorldRayOrigin(), WorldRayDirection());

    if (hit > 0) {
        ReportHit(hit, 0, 0);
    }
}

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
void closesthitMain(inout Payload payload)
{
	Sphere sphere = spheres[PrimitiveIndex()];

    float3 worldPos = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    float3 worldNormal = normalize(worldPos - sphere.center);

    // Basic lighting
    float3 lightVector = normalize(ubo.lightPos.xyz);
    float dot_product = max(dot(lightVector, worldNormal), 0.2);
    payload.hitValue = sphere.color.rgb * dot_product;	
}

[shader("miss")]
void missMain(inout Payload payload)
{
    payload.hitValue = float3(0.0, 0.0, 0.2);
}