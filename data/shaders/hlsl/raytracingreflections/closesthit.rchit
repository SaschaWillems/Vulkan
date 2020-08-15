// Copyright 2020 Google LLC

struct RayPayload
{
	float3 color;
	float distance;
	float3 normal;
	float reflector;
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
	float3 lightVector = normalize(ubo.lightPos.xyz);
	float dot_product = max(dot(lightVector, normal), 0.6);
	rayPayload.color.rgb = v0.color * dot_product;
	rayPayload.distance = RayTCurrent();
	rayPayload.normal = normal;

	// Objects with full white vertex color are treated as reflectors
	rayPayload.reflector = ((v0.color.r == 1.0f) && (v0.color.g == 1.0f) && (v0.color.b == 1.0f)) ? 1.0f : 0.0f;
}
