// Copyright 2020 Google LLC

struct VSInput
{
[[vk::location(0)]] float3 Pos : POSITION0;
[[vk::location(1)]] float3 Normal : NORMAL0;
[[vk::location(2)]] float2 UV : TEXCOORD0;
[[vk::location(3)]] float3 Color : COLOR0;

// Instanced attributes
[[vk::location(4)]] float3 instancePos : POSITION1;
[[vk::location(5)]] float3 instanceRot : TEXCOORD1;
[[vk::location(6)]] float instanceScale : TEXCOORD2;
[[vk::location(7)]] int instanceTexIndex : TEXCOORD3;
};

struct UBO
{
	float4x4 projection;
	float4x4 modelview;
	float4 lightPos;
	float locSpeed;
	float globSpeed;
};

cbuffer ubo : register(b0) { UBO ubo; }

struct VSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float3 Normal : NORMAL0;
[[vk::location(1)]] float3 Color : COLOR0;
[[vk::location(2)]] float3 UV : TEXCOORD0;
[[vk::location(3)]] float3 ViewVec : TEXCOORD1;
[[vk::location(4)]] float3 LightVec : TEXCOORD2;
};

VSOutput main(VSInput input)
{
	VSOutput output = (VSOutput)0;
	output.Color = input.Color;
	output.UV = float3(input.UV, input.instanceTexIndex);

	// rotate around x
	float s = sin(input.instanceRot.x + ubo.locSpeed);
	float c = cos(input.instanceRot.x + ubo.locSpeed);

	float3x3 mx = { c, -s, 0.0,
					s, c, 0.0,
					0.0, 0.0, 1.0 };

	// rotate around y
	s = sin(input.instanceRot.y + ubo.locSpeed);
	c = cos(input.instanceRot.y + ubo.locSpeed);

	float3x3 my = { c, 0.0, -s,
					0.0, 1.0, 0.0,
					s, 0.0, c };

	// rot around z
	s = sin(input.instanceRot.z + ubo.locSpeed);
	c = cos(input.instanceRot.z + ubo.locSpeed);

	float3x3 mz = { 1.0, 0.0, 0.0,
					0.0, c, -s,
					0.0, s, c };

	float3x3 rotMat = mul(mz, mul(my, mx));

	float4x4 gRotMat;
	s = sin(input.instanceRot.y + ubo.globSpeed);
	c = cos(input.instanceRot.y + ubo.globSpeed);
	gRotMat[0] = float4(c, 0.0, -s, 0.0);
	gRotMat[1] = float4(0.0, 1.0, 0.0, 0.0);
	gRotMat[2] = float4(s, 0.0, c, 0.0);
	gRotMat[3] = float4(0.0, 0.0, 0.0, 1.0);

	float4 locPos = float4(mul(rotMat, input.Pos.xyz), 1.0);
	float4 pos = float4((locPos.xyz * input.instanceScale) + input.instancePos, 1.0);

	output.Pos = mul(ubo.projection, mul(ubo.modelview, mul(gRotMat, pos)));
	output.Normal = mul((float3x3)mul(ubo.modelview, gRotMat), mul(rotMat, input.Normal));

	pos = mul(ubo.modelview, float4(input.Pos.xyz + input.instancePos, 1.0));
	float3 lPos = mul((float3x3)ubo.modelview, ubo.lightPos.xyz);
	output.LightVec = lPos - pos.xyz;
	output.ViewVec = -pos.xyz;
	return output;
}
