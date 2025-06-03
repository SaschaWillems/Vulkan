/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct Payload
{
    float3 hitValue;
    uint payloadSeed;
    bool shadowed;
};

struct GeometryNode {
    ConstBufferPointer<float4> vertices;
    ConstBufferPointer<uint> indices;
    int textureIndexBaseColor;
    int textureIndexOcclusion;
};

struct UBOCameraProperties {
    float4x4 viewInverse;
    float4x4 projInverse;
    uint frame;
}

[[vk::binding(0, 0)]] RaytracingAccelerationStructure accelStruct;
[[vk::binding(1, 0)]] RWTexture2D<float4> image;
[[vk::binding(2, 0)]] ConstantBuffer<UBOCameraProperties> cam;
[[vk::binding(4, 0)]] StructuredBuffer<GeometryNode> geometryNodes;
[[vk::binding(5, 0)]] Sampler2D textures[];

struct Vertex
{
    float3 pos;
    float3 normal;
    float2 uv;
};

struct Triangle {
    Vertex vertices[3];
    float3 normal;
    float2 uv;
};

struct Attributes
{
    float2 bary;
};

// Tiny Encryption Algorithm
// By Fahad Zafar, Marc Olano and Aaron Curtis, see https://www.highperformancegraphics.org/previous/www_2010/media/GPUAlgorithms/HPG2010_GPUAlgorithms_Zafar.pdf
uint tea(uint val0, uint val1)
{
    uint sum = 0;
    uint v0 = val0;
    uint v1 = val1;
    for (uint n = 0; n < 16; n++)
    {
        sum += 0x9E3779B9;
        v0 += ((v1 << 4) + 0xA341316C) ^ (v1 + sum) ^ ((v1 >> 5) + 0xC8013EA4);
        v1 += ((v0 << 4) + 0xAD90777D) ^ (v0 + sum) ^ ((v0 >> 5) + 0x7E95761E);
    }
    return v0;
}

// Linear congruential generator based on the previous RNG state
// See https://en.wikipedia.org/wiki/Linear_congruential_generator
uint lcg(inout uint previous)
{
    const uint multiplier = 1664525u;
    const uint increment = 1013904223u;
    previous = (multiplier * previous + increment);
    return previous & 0x00FFFFFF;
}

// Generate a random float in [0, 1) given the previous RNG state
float rnd(inout uint previous)
{
    return (float(lcg(previous)) / float(0x01000000));
}

// This function will unpack our vertex buffer data into a single triangle and calculates uv coordinates
Triangle unpackTriangle(uint index, Attributes attribs) {
    Triangle tri;
    const uint triIndex = index * 3;
    const uint vertexsize = 112;

    GeometryNode geometryNode = geometryNodes[GeometryIndex()];

    // Indices indices = Indices(geometryNode.indexBufferDeviceAddress);
    // Vertices vertices = Vertices(geometryNode.vertexBufferDeviceAddress);

    // Unpack vertices
    // Data is packed as float4 so we can map to the glTF vertex structure from the host side
    // We match vkglTF::Vertex: pos.xyz+normal.x, normalyz+uv.xy
    // glm::float3 pos;
    // glm::float3 normal;
    // glm::float2 uv;
    // ...
    for (uint i = 0; i < 3; i++) {
        const uint offset = geometryNode.indices[triIndex + i] * 6;
        float4 d0 = geometryNode.vertices[offset + 0]; // pos.xyz, n.x
        float4 d1 = geometryNode.vertices[offset + 1]; // n.yz, uv.xy
        tri.vertices[i].pos = d0.xyz;
        tri.vertices[i].normal = float3(d0.w, d1.xy);
        tri.vertices[i].uv = float2(d1.z, d1.w);
    }
    // Calculate values at barycentric coordinates
    float3 barycentricCoords = float3(1.0f - attribs.bary.x - attribs.bary.y, attribs.bary.x, attribs.bary.y);
    tri.uv = tri.vertices[0].uv * barycentricCoords.x + tri.vertices[1].uv * barycentricCoords.y + tri.vertices[2].uv * barycentricCoords.z;
    tri.normal = tri.vertices[0].normal * barycentricCoords.x + tri.vertices[1].normal * barycentricCoords.y + tri.vertices[2].normal * barycentricCoords.z;
    return tri;
}

[shader("raygeneration")]
void raygenerationMain()
{
    uint3 LaunchID = DispatchRaysIndex();
    uint3 LaunchSize = DispatchRaysDimensions();

    uint seed = tea(LaunchID.y * LaunchSize.x + LaunchID.x, cam.frame);

    float r1 = rnd(seed);
    float r2 = rnd(seed);

    // Subpixel jitter: send the ray through a different position inside the pixel
    // each time, to provide antialiasing.
    float2 subpixel_jitter = cam.frame == 0 ? float2(0.5f, 0.5f) : float2(r1, r2);
    const float2 pixelCenter = float2(LaunchID.xy) + subpixel_jitter;
    const float2 inUV = pixelCenter / float2(LaunchSize.xy);
    float2 d = inUV * 2.0 - 1.0;

    float4 target = mul(cam.projInverse, float4(d.x, d.y, 1, 1));

    RayDesc rayDesc;
    rayDesc.Origin = mul(cam.viewInverse, float4(0, 0, 0, 1)).xyz;
    rayDesc.Direction = mul(cam.viewInverse, float4(normalize(target.xyz), 0)).xyz;
    rayDesc.TMin = 0.001;
    rayDesc.TMax = 10000.0;

    Payload payload;
    payload.hitValue = float3(0.0);
    float3 hitValues = float3(0);

    const int samples = 4;

    // Trace multiple rays for e.g. transparency
    for (int smpl = 0; smpl < samples; smpl++) {
        payload.payloadSeed = tea(LaunchID.y * LaunchSize.x + LaunchID.x, cam.frame);
        TraceRay(accelStruct, RAY_FLAG_NONE, 0xff, 0, 0, 0, rayDesc, payload);
        hitValues += payload.hitValue;
    }

    float3 hitVal = hitValues / float(samples);

    if (cam.frame > 0)
    {
        float a = 1.0f / float(cam.frame + 1);
        float3 old_color = image[int2(LaunchID.xy)].xyz;
        image[int2(LaunchID.xy)] = float4(lerp(old_color, hitVal, a), 1.0f);
    }
    else
    {
        // First frame, replace the value in the buffer
        image[int2(LaunchID.xy)] = float4(hitVal, 1.0f);
    }
}

[shader("closesthit")]
void closesthitMain(inout Payload payload, in Attributes attribs)
{
    Triangle tri = unpackTriangle(PrimitiveIndex(), attribs);
    payload.hitValue = float3(tri.normal);

    GeometryNode geometryNode = geometryNodes[GeometryIndex()];

    float3 color = textures[NonUniformResourceIndex(geometryNode.textureIndexBaseColor)].SampleLevel(tri.uv, 0.0).rgb;
    if (geometryNode.textureIndexOcclusion > -1) {
        float occlusion = textures[NonUniformResourceIndex(geometryNode.textureIndexOcclusion)].SampleLevel(tri.uv, 0.0).r;
        color *= occlusion;
    }

    payload.hitValue = color;

    // Shadow casting
    float tmin = 0.001;
    float tmax = 10000.0;
    float epsilon = 0.001;
    float3 origin = WorldRayOrigin() + WorldRayDirection() * RayTCurrent() + tri.normal * epsilon;
    payload.shadowed = true;
    float3 lightVector = float3(-5.0, -2.5, -5.0);
    // Trace shadow ray and offset indices to match shadow hit/miss shader group indices
    //	traceRayEXT(topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT, 0xFF, 0, 0, 1, origin, tmin, lightVector, tmax, 2);
    //	if (shadowed) {
    //		hitValue *= 0.7;
    //	}
}

[shader("anyhit")]
void anyhitMain(inout Payload payload, in Attributes attribs)
{
    Triangle tri = unpackTriangle(PrimitiveIndex(), attribs);
    GeometryNode geometryNode = geometryNodes[GeometryIndex()];
    float4 color = textures[NonUniformResourceIndex(geometryNode.textureIndexBaseColor)].SampleLevel(tri.uv, 0.0);
	// If the alpha value of the texture at the current UV coordinates is below a given threshold, we'll ignore this intersection
	// That way ray traversal will be stopped and the miss shader will be invoked
	if (color.a < 0.9) {
        if (rnd(payload.payloadSeed) > color.a) {
            IgnoreHit();
		}
	}
}

[shader("miss")]
void missMain(inout Payload payload)
{
    payload.hitValue = float3(1.0);
}