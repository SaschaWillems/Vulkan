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
  float3 hitValue;
};

RaytracingAccelerationStructure accelStruct;
RWTexture2D<float4> image;
struct CameraProperties
{
    float4x4 viewInverse;
    float4x4 projInverse;
};
ConstantBuffer<CameraProperties> cam;

struct SBT {
  float r;
  float g;
  float b;
};
[[vk::shader_record]] ConstantBuffer<SBT> sbt;

[shader("raygeneration")]
void raygenerationMain()
{
    uint3 LaunchID = DispatchRaysIndex();
    uint3 LaunchSize = DispatchRaysDimensions();

    const float2 pixelCenter = float2(LaunchID.xy) + float2(0.5, 0.5);
    const float2 inUV = pixelCenter / float2(LaunchSize.xy);
    float2 d = inUV * 2.0 - 1.0;
    float4 target = mul(cam.projInverse, float4(d.x, d.y, 1, 1));

    RayDesc rayDesc;
    rayDesc.Origin = mul(cam.viewInverse, float4(0, 0, 0, 1)).xyz;
    rayDesc.Direction = mul(cam.viewInverse, float4(normalize(target.xyz), 0)).xyz;
    rayDesc.TMin = 0.001;
    rayDesc.TMax = 10000.0;

    Payload payload;

    // use border to demonstrate raygen record data
    if (all(LaunchID.xy > int2(16, 16)) && all(LaunchID.xy < LaunchSize.xy - int2(16, 16)))
    {
        // Generate a checker board pattern to trace out rays or use hit record data
        int2 pos = int2(LaunchID.xy / 16);
        if (((pos.x + pos.y % 2) % 2) == 0) {
            // This will set hit value to either hit or miss SBT record color
            TraceRay(accelStruct, RAY_FLAG_FORCE_OPAQUE, 0xff, 0, 0, 0, rayDesc, payload);
        }
        else {
            // Set the hit value to the raygen SBT data
            payload.hitValue = float3(sbt.r, sbt.g, sbt.b);
        }
    }
    else {
        // Set hit value to black
        payload.hitValue = float3(0.0, 0.0, 0.0);
    }

    image[int2(LaunchID.xy)] = float4(payload.hitValue, 0.0);
}

[shader("closesthit")]
void closesthitMain(inout Payload payload, in Attributes attribs)
{
    // Update the hit value to the hit record SBT data associated with this
    // geometry ID and ray ID
    payload.hitValue = float3(sbt.r, sbt.g, sbt.g);
}

[shader("miss")]
void missMain(inout Payload payload)
{
    // Update the hit value to the hit record SBT data associated with this
    // miss record
    payload.hitValue = float3(sbt.r, sbt.g, sbt.g);
}
