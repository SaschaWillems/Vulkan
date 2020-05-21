// Copyright 2020 Google LLC

Texture2DArray textureView : register(t1);
SamplerState samplerView : register(s1);

struct UBO
{
	[[vk::offset(272)]] float distortionAlpha;
};

cbuffer ubo : register(b0) { UBO ubo; }

[[vk::constant_id(0)]] const float VIEW_LAYER = 0.0f;

float4 main([[vk::location(0)]] float2 inUV : TEXCOORD0) : SV_TARGET
{
	const float alpha = ubo.distortionAlpha;

	float2 p1 = float2(2.0 * inUV - 1.0);
	float2 p2 = p1 / (1.0 - alpha * length(p1));
	p2 = (p2 + 1.0) * 0.5;

	bool inside = ((p2.x >= 0.0) && (p2.x <= 1.0) && (p2.y >= 0.0 ) && (p2.y <= 1.0));
	return inside ? textureView.Sample(samplerView, float3(p2, VIEW_LAYER)) : float4(0.0, 0.0, 0.0, 0.0);
}