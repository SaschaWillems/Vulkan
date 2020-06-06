// Copyright 2020 Google LLC

[[vk::input_attachment_index(0)]][[vk::binding(0)]] SubpassInput samplerposition;
[[vk::input_attachment_index(1)]][[vk::binding(1)]] SubpassInput samplerNormal;
[[vk::input_attachment_index(2)]][[vk::binding(2)]] SubpassInput samplerAlbedo;

#define MAX_NUM_LIGHTS 64
[[vk::constant_id(0)]] const int NUM_LIGHTS = 64;

struct Light {
	float4 position;
	float3 color;
	float radius;
};

struct UBO
{
	float4 viewPos;
	Light lights[MAX_NUM_LIGHTS];
};

cbuffer ubo : register(b3) { UBO ubo; }


float4 main([[vk::location(0)]] float2 inUV : TEXCOORD) : SV_TARGET
{
	// Read G-Buffer values from previous sub pass
	float3 fragPos = samplerposition.SubpassLoad().rgb;
	float3 normal = samplerNormal.SubpassLoad().rgb;
	float4 albedo = samplerAlbedo.SubpassLoad();

	#define ambient 0.15

	// Ambient part
	float3 fragcolor  = albedo.rgb * ambient;

	for(int i = 0; i < NUM_LIGHTS; ++i)
	{
		// Vector to light
		float3 L = ubo.lights[i].position.xyz - fragPos;
		// Distance from light to fragment position
		float dist = length(L);

		// Viewer to fragment
		float3 V = ubo.viewPos.xyz - fragPos;
		V = normalize(V);

		// Light to fragment
		L = normalize(L);

		// Attenuation
		float atten = ubo.lights[i].radius / (pow(dist, 2.0) + 1.0);

		// Diffuse part
		float3 N = normalize(normal);
		float NdotL = max(0.0, dot(N, L));
		float3 diff = ubo.lights[i].color * albedo.rgb * NdotL * atten;

		// Specular part
		// Specular map values are stored in alpha of albedo mrt
		float3 R = reflect(-L, N);
		float NdotR = max(0.0, dot(R, V));
		//float3 spec = ubo.lights[i].color * albedo.a * pow(NdotR, 32.0) * atten;

		fragcolor += diff;// + spec;
	}

	return float4(fragcolor, 1.0);
}