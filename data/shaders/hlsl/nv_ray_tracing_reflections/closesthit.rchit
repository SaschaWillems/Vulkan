// Copyright 2020 Google LLC

struct RayPayload
{
	float3 color;
	float distance;
	float3 normal;
	float reflector;
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
void main(inout RayPayload rayPayload, in float3 attribs)
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
	float dot_product = max(dot(lightVector, normal), 0.6);
	rayPayload.color = v0.color * dot_product;
	rayPayload.distance = RayTCurrent();
	rayPayload.normal = normal;

	// Objects with full white vertex color are treated as reflectors
	rayPayload.reflector = ((v0.color.r == 1.0f) && (v0.color.g == 1.0f) && (v0.color.b == 1.0f)) ? 1.0f : 0.0f;
}
