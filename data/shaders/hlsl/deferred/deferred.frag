// Copyright 2020 Google LLC

Texture2D textureposition : register(t1);
SamplerState samplerposition : register(s1);
Texture2D textureNormal : register(t2);
SamplerState samplerNormal : register(s2);
Texture2D textureAlbedo : register(t3);
SamplerState samplerAlbedo : register(s3);

struct Light {
	float4 position;
	float3 color;
	float radius;
};

struct UBO
{
	Light lights[6];
	float4 viewPos;
	int displayDebugTarget;
};

cbuffer ubo : register(b4) { UBO ubo; }


float4 main([[vk::location(0)]] float2 inUV : TEXCOORD0) : SV_TARGET
{
	// Get G-Buffer values
	float3 fragPos = textureposition.Sample(samplerposition, inUV).rgb;
	float3 normal = textureNormal.Sample(samplerNormal, inUV).rgb;
	float4 albedo = textureAlbedo.Sample(samplerAlbedo, inUV);

	float3 fragcolor;

	// Debug display
	if (ubo.displayDebugTarget > 0) {
		switch (ubo.displayDebugTarget) {
			case 1: 
				fragcolor.rgb = fragPos;
				break;
			case 2: 
				fragcolor.rgb = normal;
				break;
			case 3: 
				fragcolor.rgb = albedo.rgb;
				break;
			case 4: 
				fragcolor.rgb = albedo.aaa;
				break;
		}		
		return float4(fragcolor, 1.0);
	}

	#define lightCount 6
	#define ambient 0.0

	// Ambient part
	fragcolor = albedo.rgb * ambient;

	for(int i = 0; i < lightCount; ++i)
	{
		// Vector to light
		float3 L = ubo.lights[i].position.xyz - fragPos;
		// Distance from light to fragment position
		float dist = length(L);

		// Viewer to fragment
		float3 V = ubo.viewPos.xyz - fragPos;
		V = normalize(V);

		//if(dist < ubo.lights[i].radius)
		{
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
			float3 spec = ubo.lights[i].color * albedo.a * pow(NdotR, 16.0) * atten;

			fragcolor += diff + spec;
		}
	}

  return float4(fragcolor, 1.0);
}