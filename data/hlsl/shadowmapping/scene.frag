// Copyright 2020 Google LLC

Texture2D shadowMapTexture : register(t1);
SamplerState shadowMapSampler : register(s1);

struct VSOutput
{
[[vk::location(0)]] float3 Normal : NORMAL0;
[[vk::location(1)]] float3 Color : COLOR0;
[[vk::location(2)]] float3 ViewVec : TEXCOORD1;
[[vk::location(3)]] float3 LightVec : TEXCOORD2;
[[vk::location(4)]] float4 ShadowCoord : TEXCOORD3;
};

[[vk::constant_id(0)]] const int enablePCF = 0;

#define ambient 0.1

float textureProj(float4 shadowCoord, float2 off)
{
	float shadow = 1.0;
	if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 )
	{
		float dist = shadowMapTexture.Sample( shadowMapSampler, shadowCoord.xy + off ).r;
		if ( shadowCoord.w > 0.0 && dist < shadowCoord.z )
		{
			shadow = ambient;
		}
	}
	return shadow;
}

float filterPCF(float4 sc)
{
	int2 texDim;
	shadowMapTexture.GetDimensions(texDim.x, texDim.y);
	float scale = 1.5;
	float dx = scale * 1.0 / float(texDim.x);
	float dy = scale * 1.0 / float(texDim.y);

	float shadowFactor = 0.0;
	int count = 0;
	int range = 1;

	for (int x = -range; x <= range; x++)
	{
		for (int y = -range; y <= range; y++)
		{
			shadowFactor += textureProj(sc, float2(dx*x, dy*y));
			count++;
		}

	}
	return shadowFactor / count;
}

float4 main(VSOutput input) : SV_TARGET
{
	float shadow = (enablePCF == 1) ? filterPCF(input.ShadowCoord / input.ShadowCoord.w) : textureProj(input.ShadowCoord / input.ShadowCoord.w, float2(0.0, 0.0));

	float3 N = normalize(input.Normal);
	float3 L = normalize(input.LightVec);
	float3 V = normalize(input.ViewVec);
	float3 R = normalize(-reflect(L, N));
	float3 diffuse = max(dot(N, L), ambient) * input.Color;

	return float4(diffuse * shadow, 1.0);
}
