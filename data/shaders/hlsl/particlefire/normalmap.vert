// Copyright 2020 Google LLC

struct VSInput
{
[[vk::location(0)]] float3 Pos : POSITION0;
[[vk::location(1)]] float2 UV : TEXCOORD0;
[[vk::location(2)]] float3 Normal : NORMAL0;
[[vk::location(3)]] float4 Tangent : TEXCOORD1;
};

struct UBO
{
	float4x4 projection;
	float4x4 model;
	float4x4 normal;
	float4 lightPos;
};

cbuffer ubo : register(b0) { UBO ubo; }

struct VSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float2 UV : TEXCOORD0;
[[vk::location(1)]] float3 LightVec : TEXCOORD2;
[[vk::location(2)]] float3 LightVecB : TEXCOORD3;
[[vk::location(3)]] float3 LightDir : TEXCOORD4;
[[vk::location(4)]] float3 ViewVec : TEXCOORD1;
};

VSOutput main(VSInput input)
{
	VSOutput output = (VSOutput)0;
	float3 vertexPosition = mul(ubo.model, float4(input.Pos, 1.0)).xyz;
	output.LightDir = normalize(ubo.lightPos.xyz - vertexPosition);

	float3 biTangent = cross(input.Normal, input.Tangent.xyz);

	// Setup (t)angent-(b)inormal-(n)ormal matrix for converting
	// object coordinates into tangent space
	float3x3 tbnMatrix;
	tbnMatrix[0] =  mul((float3x3)ubo.normal, input.Tangent);
	tbnMatrix[1] =  mul((float3x3)ubo.normal, biTangent);
	tbnMatrix[2] =  mul((float3x3)ubo.normal, input.Normal);

	output.LightVec.xyz = mul(float3(ubo.lightPos.xyz - vertexPosition), tbnMatrix);

	float3 lightDist = ubo.lightPos.xyz - input.Pos;
	output.LightVecB.x = dot(input.Tangent, lightDist);
	output.LightVecB.y = dot(biTangent, lightDist);
	output.LightVecB.z = dot(input.Normal, lightDist);

	output.ViewVec.x = dot(input.Tangent, input.Pos);
	output.ViewVec.y = dot(biTangent, input.Pos);
	output.ViewVec.z = dot(input.Normal, input.Pos);

	output.UV = input.UV;

	output.Pos = mul(ubo.projection, mul(ubo.model, float4(input.Pos, 1.0)));
	return output;
}