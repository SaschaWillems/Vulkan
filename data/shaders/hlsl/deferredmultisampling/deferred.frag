// Copyright 2020 Google LLC

Texture2DMS<float4> texturePosition : register(t1);
SamplerState samplerPosition : register(s1);
Texture2DMS<float4> textureNormal : register(t2);
SamplerState samplerNormal : register(s2);
Texture2DMS<float4> textureAlbedo : register(t3);
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
	int debugDisplayTarget;
};

cbuffer ubo : register(b4) { UBO ubo; }

[[vk::constant_id(0)]] const int NUM_SAMPLES = 8;

#define NUM_LIGHTS 6

// Manual resolve for MSAA samples
float4 resolve(Texture2DMS<float4> tex, int2 uv)
{
	float4 result = float4(0.0, 0.0, 0.0, 0.0);
	for (int i = 0; i < NUM_SAMPLES; i++)
	{
		uint status = 0;
		float4 val = tex.Load(uv, i, int2(0, 0), status);
		result += val;
	}
	// Average resolved samples
	return result / float(NUM_SAMPLES);
}

float3 calculateLighting(float3 pos, float3 normal, float4 albedo)
{
	float3 result = float3(0.0, 0.0, 0.0);

	for(int i = 0; i < NUM_LIGHTS; ++i)
	{
		// Vector to light
		float3 L = ubo.lights[i].position.xyz - pos;
		// Distance from light to fragment position
		float dist = length(L);

		// Viewer to fragment
		float3 V = ubo.viewPos.xyz - pos;
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
		float3 R = reflect(-L, N);
		float NdotR = max(0.0, dot(R, V));
		float3 spec = ubo.lights[i].color * albedo.a * pow(NdotR, 8.0) * atten;

		result += diff + spec;
	}
	return result;
}

float4 main([[vk::location(0)]] float2 inUV : TEXCOORD0) : SV_TARGET
{
	int2 attDim; int sampleCount;
	texturePosition.GetDimensions(attDim.x, attDim.y, sampleCount);
	int2 UV = int2(inUV * attDim);

	float3 fragColor;
	uint status = 0;

	// Debug display
	if (ubo.debugDisplayTarget > 0) {
		switch (ubo.debugDisplayTarget) {
			case 1: 
				fragColor.rgb = texturePosition.Load(UV, 0, int2(0, 0), status).rgb;
				break;
			case 2: 
				fragColor.rgb = textureNormal.Load(UV, 0, int2(0, 0), status).rgb;
				break;
			case 3: 
				fragColor.rgb = textureAlbedo.Load(UV, 0, int2(0, 0), status).rgb;
				break;
			case 4: 
				fragColor.rgb = textureAlbedo.Load(UV, 0, int2(0, 0), status).aaa;
				break;
		}		
		return float4(fragColor, 1.0);
	}

	#define ambient 0.15

	// Ambient part
	float4 alb = resolve(textureAlbedo, UV);
	fragColor = float3(0.0, 0.0, 0.0);

	// Calualte lighting for every MSAA sample
	for (int i = 0; i < NUM_SAMPLES; i++)
	{
		float3 pos = texturePosition.Load(UV, i, int2(0, 0), status).rgb;
		float3 normal = textureNormal.Load(UV, i, int2(0, 0), status).rgb;
		float4 albedo = textureAlbedo.Load(UV, i, int2(0, 0), status);
		fragColor += calculateLighting(pos, normal, albedo);
	}

	fragColor = (alb.rgb * ambient) + fragColor / float(NUM_SAMPLES);

	return float4(fragColor, 1.0);
}