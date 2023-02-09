// Copyright 2020 Google LLC

Texture2D textureColorMap : register(t1);
SamplerState samplerColorMap : register(s1);
Texture2D textureNormalHeightMap : register(t2);
SamplerState samplerNormalHeightMap : register(s2);

#define lightRadius 45.0

struct VSOutput
{
[[vk::location(0)]] float2 UV : TEXCOORD0;
[[vk::location(1)]] float3 LightVec : TEXCOORD2;
[[vk::location(2)]] float3 LightVecB : TEXCOORD3;
[[vk::location(3)]] float3 LightDir : TEXCOORD4;
[[vk::location(4)]] float3 ViewVec : TEXCOORD1;
};

float4 main(VSOutput input) : SV_TARGET
{
	float3 specularColor = float3(0.85, 0.5, 0.0);

	float invRadius = 1.0/lightRadius;
	float ambient = 0.25;

	float3 rgb, normal;

	rgb = textureColorMap.Sample(samplerColorMap, input.UV).rgb;
	normal = normalize((textureNormalHeightMap.Sample(samplerNormalHeightMap, input.UV).rgb - 0.5) * 2.0);

	float distSqr = dot(input.LightVecB, input.LightVecB);
	float3 lVec = input.LightVecB * rsqrt(distSqr);

	float atten = max(clamp(1.0 - invRadius * sqrt(distSqr), 0.0, 1.0), ambient);
	float diffuse = clamp(dot(lVec, normal), 0.0, 1.0);

	float3 light = normalize(-input.LightVec);
	float3 view = normalize(input.ViewVec);
	float3 reflectDir = reflect(-light, normal);

	float specular = pow(max(dot(view, reflectDir), 0.0), 4.0);

	return float4((rgb * atten + (diffuse * rgb + 0.5 * specular * specularColor.rgb)) * atten, 1.0);
}