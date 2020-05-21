// Copyright 2020 Google LLC

Texture2DMS<float4> texturePosition : register(t1);
SamplerState samplerPosition : register(s1);
Texture2DMS<float4> textureNormal : register(t2);
SamplerState samplerNormal : register(s2);
Texture2DMS<float4> textureAlbedo : register(t3);
SamplerState samplerAlbedo : register(s3);

[[vk::constant_id(0)]] const int NUM_SAMPLES = 8;

float4 resolve(Texture2DMS<float4> tex, int2 uv)
{
	float4 result = float4(0.0, 0.0, 0.0, 0.0);
	int count = 0;
	for (int i = 0; i < NUM_SAMPLES; i++)
	{
		uint status = 0;
		float4 val = tex.Load(uv, i, int2(0, 0), status);
		result += val;
		count++;
	}
	return result / float(NUM_SAMPLES);
}

float4 main([[vk::location(0)]] float3 inUV : TEXCOORD0) : SV_TARGET
{
	int2 attDim; int sampleCount;
	texturePosition.GetDimensions(attDim.x, attDim.y, sampleCount);
	int2 UV = int2(inUV.xy * attDim * 2.0);

	int index = 0;
	if (inUV.x > 0.5)
	{
		index = 1;
		UV.x -= attDim.x;
	}
	if (inUV.y > 0.5)
	{
		index = 2;
		UV.y -= attDim.y;
	}

	float3 components[3];
	components[0] = resolve(texturePosition, UV).rgb;
	components[1] = resolve(textureNormal, UV).rgb;
	components[2] = resolve(textureAlbedo, UV).rgb;
	// Uncomment to display specular component
	//components[2] = float3(textureAlbedo.Sample(samplerAlbedo, inUV.xt).a);

	// Select component depending on UV
	return float4(components[index], 1);
}