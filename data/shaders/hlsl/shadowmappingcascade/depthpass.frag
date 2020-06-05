// Copyright 2020 Google LLC

Texture2D colorMapTexture : register(t0, space1);
SamplerState colorMapSampler : register(s0, space1);

void main([[vk::location(0)]] float2 inUV : TEXCOORD0)
{
	float alpha = colorMapTexture.Sample(colorMapSampler, inUV).a;
	if (alpha < 0.5) {
		clip(-1);
	}
}