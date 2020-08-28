// Copyright 2020 Google LLC

TextureCubeArray textureCubeMapArray : register(t1);
SamplerState samplerCubeMapArray : register(s1);

struct UBO
{
	float4x4 projection;
	float4x4 model;
	float4x4 invModel;
	float lodBias;
	int cubeMapIndex;
};

cbuffer ubo : register(b0) { UBO ubo; }

struct VSOutput
{
[[vk::location(0)]] float3 Pos : POSITION0;
[[vk::location(1)]] float3 Normal : NORMAL0;
[[vk::location(2)]] float LodBias : TEXCOORD3;
[[vk::location(3)]] float3 ViewVec : TEXCOORD1;
[[vk::location(4)]] float3 LightVec : TEXCOORD2;
};

float4 main(VSOutput input) : SV_TARGET
{
	float3 cI = normalize (input.Pos);
	float3 cR = reflect (cI, normalize(input.Normal));

	cR = mul(ubo.invModel, float4(cR, 0.0)).xyz;
	cR *= float3(1.0, -1.0, -1.0);

	float4 color = textureCubeMapArray.SampleLevel(samplerCubeMapArray, float4(cR, ubo.cubeMapIndex), input.LodBias);

	float3 N = normalize(input.Normal);
	float3 L = normalize(input.LightVec);
	float3 V = normalize(input.ViewVec);
	float3 R = reflect(-L, N);
	float3 ambient = float3(0.5, 0.5, 0.5) * color.rgb;
	float3 diffuse = max(dot(N, L), 0.0) * float3(1.0, 1.0, 1.0);
	float3 specular = pow(max(dot(R, V), 0.0), 16.0) * float3(0.5, 0.5, 0.5);
	return float4(ambient + diffuse * color.rgb + specular, 1.0);
}