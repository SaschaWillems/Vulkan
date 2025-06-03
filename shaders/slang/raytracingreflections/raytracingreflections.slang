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
    float4 lightPos;
    int vertexSize;
};
ConstantBuffer<CameraProperties> ubo;
StructuredBuffer<float4> vertices;
StructuredBuffer<uint> indices;

// Max. number of recursion is passed via a specialization constant
[SpecializationConstant] const int MAX_RECURSION = 0;

struct Attributes
{
    float2 bary;
};

struct RayPayload
{
	float3 color;
	float distance;
	float3 normal;
	float reflector;
};

struct Vertex
{
  float3 pos;
  float3 normal;
  float2 uv;
  float4 color;
  float4 _pad0; 
  float4 _pad1;
};

Vertex unpack(uint index)
{
	// Unpack the vertices from the SSBO using the glTF vertex structure
	// The multiplier is the size of the vertex divided by four float components (=16 bytes)
	const int m = ubo.vertexSize / 16;

	float4 d0 = vertices[m * index + 0];
	float4 d1 = vertices[m * index + 1];
	float4 d2 = vertices[m * index + 2];

	Vertex v;
	v.pos = d0.xyz;
	v.normal = float3(d0.w, d1.x, d1.y);
	v.color = float4(d2.x, d2.y, d2.z, 1.0);

	return v;
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

    float3 color = float3(0.0, 0.0, 0.0);

    for (int i = 0; i < MAX_RECURSION; i++) {
        RayPayload rayPayload;
        TraceRay(accelStruct, RAY_FLAG_FORCE_OPAQUE, 0xff, 0, 0, 0, rayDesc, rayPayload);
        float3 hitColor = rayPayload.color;

        if (rayPayload.distance < 0.0f) {
            color += hitColor;
            break;
        } else if (rayPayload.reflector == 1.0f) {
            const float3 hitPos = rayDesc.Origin + rayDesc.Direction * rayPayload.distance;
            rayDesc.Origin = hitPos + rayPayload.normal * 0.001f;
            rayDesc.Direction = reflect(rayDesc.Direction, rayPayload.normal);
        } else {
            color += hitColor;
            break;
        }
    }

    image[int2(LaunchID.xy)] = float4(color, 0.0);
}

[shader("closesthit")]
void closesthitMain(inout RayPayload rayPayload, in Attributes attribs)
{
    uint PrimitiveID = PrimitiveIndex();
    int3 index = int3(indices[3 * PrimitiveID], indices[3 * PrimitiveID + 1], indices[3 * PrimitiveID + 2]);

    Vertex v0 = unpack(index.x);
    Vertex v1 = unpack(index.y);
    Vertex v2 = unpack(index.z);

    // Interpolate normal
    const float3 barycentricCoords = float3(1.0f - attribs.bary.x - attribs.bary.y, attribs.bary.x, attribs.bary.y);
    float3 normal = normalize(v0.normal * barycentricCoords.x + v1.normal * barycentricCoords.y + v2.normal * barycentricCoords.z);

    // Basic lighting
    float3 lightVector = normalize(ubo.lightPos.xyz);
    float dot_product = max(dot(lightVector, normal), 0.6);
    rayPayload.color.rgb = v0.color.rgb * dot_product;
    rayPayload.distance = RayTCurrent();
    rayPayload.normal = normal;

    // Objects with full white vertex color are treated as reflectors
    rayPayload.reflector = ((v0.color.r == 1.0f) && (v0.color.g == 1.0f) && (v0.color.b == 1.0f)) ? 1.0f : 0.0f;
}

[shader("miss")]
void missMain(inout RayPayload rayPayload)
{
    float3 worldRayDirection = WorldRayDirection();

    // View-independent background gradient to simulate a basic sky background
    const float3 gradientStart = float3(0.5, 0.6, 1.0);
    const float3 gradientEnd = float3(1.0, 1.0, 1.0);
    float3 unitDir = normalize(worldRayDirection);
    float t = 0.5 * (unitDir.y + 1.0);
    rayPayload.color = (1.0 - t) * gradientStart + t * gradientEnd;

    rayPayload.distance = -1.0f;
    rayPayload.normal = float3(0, 0, 0);
    rayPayload.reflector = 0.0f;
}