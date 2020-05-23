// Copyright 2020 Google LLC

struct UBO
{
	float4x4 depthMVP;
};

cbuffer ubo : register(b0) { UBO ubo; }

float4 main([[vk::location(0)]] float3 Pos : POSITION0) : SV_POSITION
{
	return mul(ubo.depthMVP, float4(Pos, 1.0));
}