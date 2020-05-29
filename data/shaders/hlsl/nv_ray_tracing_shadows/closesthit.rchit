// Copyright 2020 Google LLC

struct InPayload
{
	[[vk::location(0)]] float3 hitValue;
};

struct InOutPayload
{
	[[vk::location(2)]] bool shadowed;
};

RaytracingAccelerationStructure topLevelAS : register(t0);
struct CameraProperties
{
	float4x4 viewInverse;
	float4x4 projInverse;
	float4 lightPos;
};
cbuffer cam : register(b2) { CameraProperties cam; };

StructuredBuffer<float4> vertices : register(t3);
StructuredBuffer<uint> indices : register(t4);

struct Vertex
{
  float3 pos;
  float3 normal;
  float3 color;
  float2 uv;
  float _pad0;
};

Vertex unpack(uint index)
{
	float4 d0 = vertices[3 * index + 0];
	float4 d1 = vertices[3 * index + 1];
	float4 d2 = vertices[3 * index + 2];

	Vertex v;
	v.pos = d0.xyz;
	v.normal = float3(d0.w, d1.x, d1.y);
	v.color = float3(d1.z, d1.w, d2.x);
	return v;
}

[shader("closesthit")]
void main(in InPayload inPayload, inout InOutPayload inOutPayload, in float3 attribs)
{
	uint PrimitiveID = PrimitiveIndex();
	int3 index = int3(indices[3 * PrimitiveID], indices[3 * PrimitiveID + 1], indices[3 * PrimitiveID + 2]);

	Vertex v0 = unpack(index.x);
	Vertex v1 = unpack(index.y);
	Vertex v2 = unpack(index.z);

	// Interpolate normal
	const float3 barycentricCoords = float3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
	float3 normal = normalize(v0.normal * barycentricCoords.x + v1.normal * barycentricCoords.y + v2.normal * barycentricCoords.z);

	// Basic lighting
	float3 lightVector = normalize(cam.lightPos.xyz);
	float dot_product = max(dot(lightVector, normal), 0.2);
	inPayload.hitValue = v0.color * dot_product;

	RayDesc rayDesc;
	rayDesc.Origin = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
	rayDesc.Direction = lightVector;
	rayDesc.TMin = 0.001;
	rayDesc.TMax = 100.0;

	inOutPayload.shadowed = true;
	// Offset indices to match shadow hit/miss index
	TraceRay(topLevelAS, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER, 0xff, 1, 0, 1, rayDesc, inOutPayload);
	if (inOutPayload.shadowed) {
		inPayload.hitValue *= 0.3;
	}
}
