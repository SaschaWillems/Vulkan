// Copyright 2020 Google LLC

Texture2D textureSSAO : register(t0);
SamplerState samplerSSAO : register(s0);

float4 main([[vk::location(0)]] float2 inUV : TEXCOORD0) : SV_TARGET
{
	const int blurRange = 2;
	int n = 0;
	int2 texDim;
	textureSSAO.GetDimensions(texDim.x, texDim.y);
	float2 texelSize = 1.0 / (float2)texDim;
	float result = 0.0;
	for (int x = -blurRange; x < blurRange; x++)
	{
		for (int y = -blurRange; y < blurRange; y++)
		{
			float2 offset = float2(float(x), float(y)) * texelSize;
			result += textureSSAO.Sample(samplerSSAO, inUV + offset).r;
			n++;
		}
	}
	return result / (float(n));
}