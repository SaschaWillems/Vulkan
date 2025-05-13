/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct Vertex
{
    float3 pos;
    float2 uv;
};

struct Triangle {
    Vertex vertices[3];
    float2 uv;
};

struct BufferReferences {
    // Pointer to the buffer with the scene's MVP matrix
    ConstBufferPointer<float4> vertices;
    // Pointer to the buffer for the data for each model
    ConstBufferPointer<uint> indices;
};
[[vk::push_constant]] BufferReferences bufferReferences;

struct Payload
{
    float3 hitValue;
};

struct UBOCameraProperties {
    float4x4 viewInverse;
    float4x4 projInverse;
}

RaytracingAccelerationStructure accelStruct;
RWTexture2D<float4> image;
ConstantBuffer<UBOCameraProperties> cam;
Sampler2D samplerColor;

struct Attributes
{
    float2 bary;
};

Triangle unpackTriangle(uint index, Attributes attribs) {
    Triangle tri;
    const uint triIndex = index * 3;
    const uint vertexSize = 32;

    // Unpack vertices
    // Data is packed as float4 so we can map to the glTF vertex structure from the host side
    for (uint i = 0; i < 3; i++) {
        const uint offset = bufferReferences.indices[triIndex + i] * (vertexSize / 16);
        float4 d0 = bufferReferences.vertices[offset + 0]; // pos.xyz, n.x
        float4 d1 = bufferReferences.vertices[offset + 1]; // n.yz, uv.xy
        tri.vertices[i].pos = d0.xyz;
        tri.vertices[i].uv = d1.zw;
    }
    // Calculate values at barycentric coordinates
    float3 barycentricCoords = float3(1.0f - attribs.bary.x - attribs.bary.y, attribs.bary.x, attribs.bary.y);
    tri.uv = tri.vertices[0].uv * barycentricCoords.x + tri.vertices[1].uv * barycentricCoords.y + tri.vertices[2].uv * barycentricCoords.z;
    return tri;
}

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
    TraceRay(accelStruct, RAY_FLAG_NONE, 0xff, 0, 0, 0, rayDesc, payload);

    image[int2(LaunchID.xy)] = float4(payload.hitValue, 0.0);
}

[shader("closesthit")]
void closesthitMain(inout Payload payload, in Attributes attribs)
{
    Triangle tri = unpackTriangle(PrimitiveIndex(), attribs);
    // Fetch the color for this ray hit from the texture at the current uv coordinates
    float4 color = samplerColor.SampleLevel(tri.uv, 0.0);
    payload.hitValue = color.rgb;
}

[shader("anyhit")]
void anyhitMain(inout Payload payload, in Attributes attribs)
{
    Triangle tri = unpackTriangle(PrimitiveIndex(), attribs);
    float4 color = samplerColor.SampleLevel(tri.uv, 0.0);
    // If the alpha value of the texture at the current UV coordinates is below a given threshold, we'll ignore this intersection
    // That way ray traversal will be stopped and the miss shader will be invoked
    if (color.a < 0.9) {
        IgnoreHit();
    }
}

[shader("miss")]
void missMain(inout Payload payload)
{
    payload.hitValue = float3(0.0, 0.0, 0.2);
}