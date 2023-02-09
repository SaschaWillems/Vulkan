// Copyright 2020 Google LLC

Texture2D textureColor : register(t1);
SamplerState samplerColor : register(s1);

struct UBO
{
	float radialBlurScale;
	float radialBlurStrength;
	float2 radialOrigin;
};

cbuffer ubo : register(b0) { UBO ubo; }

float4 main([[vk::location(0)]] float2 inUV : TEXCOORD0) : SV_TARGET
{
	int2 texDim;
	textureColor.GetDimensions(texDim.x, texDim.y);
	float2 radialSize = float2(1.0 / texDim.x, 1.0 / texDim.y);

	float2 UV = inUV;

	float4 color = float4(0.0, 0.0, 0.0, 0.0);
	UV += radialSize * 0.5 - ubo.radialOrigin;

	#define samples 32

	for (int i = 0; i < samples; i++)
	{
		float scale = 1.0 - ubo.radialBlurScale * (float(i) / float(samples-1));
		color += textureColor.Sample(samplerColor, UV * scale + ubo.radialOrigin);
	}

	return (color / samples) * ubo.radialBlurStrength;
}