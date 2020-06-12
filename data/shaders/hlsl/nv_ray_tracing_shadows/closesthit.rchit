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
struct UBO
{
	float4x4 viewInverse;
	float4x4 projInverse;
	float4 lightPos;
	int vertexSize;
};
cbuffer ubo : register(b2) { UBO ubo; };

StructuredBuffer<float4> vertices : register(t3);
StructuredBuffer<uint> indices : register(t4);

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
	float3 lightVector = normalize(ubo.lightPos.xyz);
	float dot_product = max(dot(lightVector, normal), 0.2);
	inPayload.hitValue = v0.color.rgb * dot_product;

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
