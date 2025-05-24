/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

RaytracingAccelerationStructure accelStruct;
RWTexture2D<float4> image;
struct CameraProperties
{
	float4x4 viewInverse;
	float4x4 projInverse;
};
ConstantBuffer<CameraProperties> cam;

struct Attributes
{
    float2 bary;
};

struct Payload
{
	float3 hitValue;
};

[shader("raygeneration")]
void raygenerationMain()
{
	uint3 LaunchID = DispatchRaysIndex();
	uint3 LaunchSize = DispatchRaysDimensions();

	const float2 pixelCenter = float2(LaunchID.xy) + float2(0.5, 0.5);
	const float2 inUV = pixelCenter/float2(LaunchSize.xy);
	float2 d = inUV * 2.0 - 1.0;
	float4 target = mul(cam.projInverse, float4(d.x, d.y, 1, 1));

	RayDesc rayDesc;
	rayDesc.Origin = mul(cam.viewInverse, float4(0,0,0,1)).xyz;
	rayDesc.Direction = mul(cam.viewInverse, float4(normalize(target.xyz), 0)).xyz;
	rayDesc.TMin = 0.001;
	rayDesc.TMax = 10000.0;

    Payload payload;
    TraceRay(accelStruct, RAY_FLAG_FORCE_OPAQUE, 0xff, 0, 0, 0, rayDesc, payload);

	image[int2(LaunchID.xy)] = float4(payload.hitValue, 0.0);
}

[shader("closesthit")]
void closesthitMain(inout Payload p, in Attributes attribs)
{
    const float3 barycentricCoords = float3(1.0f - attribs.bary.x - attribs.bary.y, attribs.bary.x, attribs.bary.y);
    p.hitValue = barycentricCoords;
}

[shader("miss")]
void missMain(inout Payload p)
{
    p.hitValue = float3(0.0, 0.0, 0.2);
}
