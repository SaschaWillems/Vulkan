// Copyright 2020 Google LLC

[[vk::input_attachment_index(0)]][[vk::binding(1)]] SubpassInput samplerPositionDepth;
Texture2D textureTexture : register(t2);
SamplerState samplerTexture : register(s2);

struct VSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float3 Color : COLOR0;
[[vk::location(1)]] float2 UV : TEXCOORD0;
};

[[vk::constant_id(0)]] const float NEAR_PLANE = 0.1f;
[[vk::constant_id(1)]] const float FAR_PLANE = 256.0f;

float linearDepth(float depth)
{
	float z = depth * 2.0f - 1.0f;
	return (2.0f * NEAR_PLANE * FAR_PLANE) / (FAR_PLANE + NEAR_PLANE - z * (FAR_PLANE - NEAR_PLANE));
}

float4 main (VSOutput input) : SV_TARGET
{
	// Sample depth from deferred depth buffer and discard if obscured
	float depth = samplerPositionDepth.SubpassLoad().a;
	if ((depth != 0.0) && (linearDepth(input.Pos.z) > depth))
	{
		clip(-1);
	};

	return textureTexture.Sample(samplerTexture, input.UV);
}
