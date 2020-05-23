// Copyright 2020 Google LLC

Texture2D texturePosition : register(t1);
SamplerState samplerPosition : register(s1);
Texture2D textureNormal : register(t2);
SamplerState samplerNormal : register(s2);
Texture2D textureAlbedo : register(t3);
SamplerState samplerAlbedo : register(s3);

float4 main([[vk::location(0)]] float3 inUV : TEXCOORD0) : SV_TARGET
{
	float3 components[3];
	components[0] = texturePosition.Sample(samplerPosition, inUV.xy).rgb;
	components[1] = textureNormal.Sample(samplerNormal, inUV.xy).rgb;
	components[2] = textureAlbedo.Sample(samplerAlbedo, inUV.xy).rgb;
	// Uncomment to display specular component
	//components[2] = float3(textureAlbedo.Sample(samplerAlbedo, inUV.st).a);

	// Select component depending on z coordinate of quad
	int index = int(inUV.z);
	return float4(components[index], 1);
}