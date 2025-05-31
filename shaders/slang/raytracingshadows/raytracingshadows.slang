/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

import payload;

struct Attributes
{
  float2 bary;
};

RaytracingAccelerationStructure accelStruct;
RWTexture2D<float4> image;

struct UBO
{
	float4x4 viewInverse;
	float4x4 projInverse;
	float4 lightPos;
	int vertexSize;
};
ConstantBuffer<UBO> ubo;

StructuredBuffer<float4> vertices;
StructuredBuffer<uint> indices;

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

    Payload payload;
    TraceRay(accelStruct, RAY_FLAG_FORCE_OPAQUE, 0xff, 0, 0, 0, rayDesc, payload);

    image[int2(LaunchID.xy)] = float4(payload.hitValue, 0.0);
}

[shader("closesthit")]
void closesthitMain(inout Payload payload, in Attributes attribs)
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
	float dot_product = max(dot(lightVector, normal), 0.2);
	payload.hitValue = v0.color.rgb * dot_product;

	RayDesc rayDesc;
	rayDesc.Origin = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
	rayDesc.Direction = lightVector;
	rayDesc.TMin = 0.001;
	rayDesc.TMax = 100.0;

	payload.shadowed = true;
    // Offset indices to match shadow hit/miss index
    TraceRay(accelStruct, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER, 0xff, 0, 0, 1, rayDesc, payload);
	if (payload.shadowed) {
		payload.hitValue *= 0.3;
	}
}

[shader("miss")]
void missMain(inout Payload payload)
{
    payload.hitValue = float3(0.0, 0.0, 0.2);
}