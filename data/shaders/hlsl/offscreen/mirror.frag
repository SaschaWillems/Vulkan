// Copyright 2020 Google LLC

Texture2D textureColor : register(t1);
SamplerState samplerColor : register(s1);

struct VSOutput
{
[[vk::location(0)]] float2 UV : TEXCOORD0;
[[vk::location(1)]] float4 ProjCoord : POSITION0;
};

float4 main(VSOutput input, bool FrontFacing : SV_IsFrontFace) : SV_TARGET
{
	float4 tmp = (1.0 / input.ProjCoord.w).xxxx;
	float4 projCoord = input.ProjCoord * tmp;

	// Scale and bias
	projCoord += float4(1.0, 1.0, 1.0, 1.0);
	projCoord *= float4(0.5, 0.5, 0.5, 0.5);

	// Slow single pass blur
	// For demonstration purposes only
	const float blurSize = 1.0 / 512.0;

	float4 color = float4(float3(0.0, 0.0, 0.0), 1.);

	if (FrontFacing)
	{
		// Only render mirrored scene on front facing (upper) side of mirror surface
		float4 reflection = float4(0.0, 0.0, 0.0, 0.0);
		for (int x = -3; x <= 3; x++)
		{
			for (int y = -3; y <= 3; y++)
			{
				reflection += textureColor.Sample(samplerColor, float2(projCoord.x + x * blurSize, projCoord.y + y * blurSize)) / 49.0;
			}
		}
		color += reflection;
	}

	return color;
}