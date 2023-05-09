// Copyright 2020 Google LLC

struct RayPayload {
	float3 color;
	float distance;
	float3 normal;
	float reflector;
};

[shader("miss")]
void main(inout RayPayload rayPayload)
{
	float3 worldRayDirection = WorldRayDirection();

	// View-independent background gradient to simulate a basic sky background
	const float3 gradientStart = float3(0.5, 0.6, 1.0);
	const float3 gradientEnd = float3(1.0, 1.0, 1.0);
	float3 unitDir = normalize(worldRayDirection);
	float t = 0.5 * (unitDir.y + 1.0);
	rayPayload.color = (1.0-t) * gradientStart + t * gradientEnd;

	rayPayload.distance = -1.0f;
	rayPayload.normal = float3(0, 0, 0);
	rayPayload.reflector = 0.0f;
}