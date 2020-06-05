// Copyright 2020 Google LLC

Texture2D textureColorMap : register(t1);
SamplerState samplerColorMap : register(s1);

struct VSOutput
{
[[vk::location(0)]] float2 UV : TEXCOORD0;
[[vk::location(1)]] float3 Normal : NORMAL0;
[[vk::location(2)]] float3 ViewVec : TEXCOORD1;
[[vk::location(3)]] float3 LightVec : TEXCOORD2;
};

float4 main(VSOutput input) : SV_TARGET
{
	float4 color = textureColorMap.Sample(samplerColorMap, input.UV);

	float distSqr = dot(input.LightVec, input.LightVec);
	float3 lVec = input.LightVec * rsqrt(distSqr);

	const float attInvRadius = 1.0/5000.0;
	float atten = max(clamp(1.0 - attInvRadius * sqrt(distSqr), 0.0, 1.0), 0.0);

	// Fake drop shadow
	const float shadowInvRadius = 1.0/2500.0;
	float dropshadow = max(clamp(1.0 - shadowInvRadius * sqrt(distSqr), 0.0, 1.0), 0.0);

	float4 outFragColor = float4(color.rgba * (1.0 - dropshadow));
	outFragColor.rgb *= atten;
	return outFragColor;
}