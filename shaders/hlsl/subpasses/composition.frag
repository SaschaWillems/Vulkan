// Copyright 2020 Google LLC

[[vk::input_attachment_index(0)]][[vk::binding(0)]] SubpassInput inputPosition;
[[vk::input_attachment_index(1)]][[vk::binding(1)]] SubpassInput inputNormal;
[[vk::input_attachment_index(2)]][[vk::binding(2)]] SubpassInput inputAlbedo;

struct Light {
	float4 position;
	float3 color;
	float radius;
};

RWStructuredBuffer<Light> lights: register(u3);

float4 main([[vk::location(0)]] float2 inUV : TEXCOORD) : SV_TARGET
{
	// Read G-Buffer values from previous sub pass
	float3 fragPos = inputPosition.SubpassLoad().rgb;
	float3 normal = inputNormal.SubpassLoad().rgb;
	float4 albedo = inputAlbedo.SubpassLoad();

	#define ambient 0.05

	// Ambient part
	float3 fragcolor  = albedo.rgb * ambient;

	uint lightsLength;
	uint lightsStride;
	lights.GetDimensions(lightsLength, lightsStride);

	for(int i = 0; i < lightsLength; ++i)
	{
		float3 L = lights[i].position.xyz - fragPos;
		float dist = length(L);

		L = normalize(L);

		float atten = lights[i].radius / (pow(dist, 3.0) + 1.0);
		float3 N = normalize(normal);
		float NdotL = max(0.0, dot(N, L));
		float3 diff = lights[i].color * albedo.rgb * NdotL * atten;

		fragcolor += diff;
	}

	return float4(fragcolor, 1.0);
}