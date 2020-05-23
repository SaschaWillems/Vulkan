// Copyright 2020 Google LLC

Texture2D textureColor0 : register(t0);
SamplerState samplerColor0 : register(s0);
Texture2D textureColor1 : register(t1);
SamplerState samplerColor1 : register(s1);

[[vk::constant_id(0)]] const int dir = 0;

float4 main([[vk::location(0)]] float2 inUV : TEXCOORD0) : SV_TARGET
{
	// From the OpenGL Super bible
	const float weights[] = {	0.0024499299678342,
								0.0043538453346397,
								0.0073599963704157,
								0.0118349786570722,
								0.0181026699707781,
								0.0263392293891488,
								0.0364543006660986,
								0.0479932050577658,
								0.0601029809166942,
								0.0715974486241365,
								0.0811305381519717,
								0.0874493212267511,
								0.0896631113333857,
								0.0874493212267511,
								0.0811305381519717,
								0.0715974486241365,
								0.0601029809166942,
								0.0479932050577658,
								0.0364543006660986,
								0.0263392293891488,
								0.0181026699707781,
								0.0118349786570722,
								0.0073599963704157,
								0.0043538453346397,
								0.0024499299678342};


	const float blurScale = 0.003;
	const float blurStrength = 1.0;

	float ar = 1.0;
	// Aspect ratio for vertical blur pass
	if (dir == 1)
	{
		float2 ts;
		textureColor1.GetDimensions(ts.x, ts.y);
		ar = ts.y / ts.x;
	}

	float2 P = inUV.yx - float2(0, (25 >> 1) * ar * blurScale);

	float4 color = float4(0.0, 0.0, 0.0, 0.0);
	for (int i = 0; i < 25; i++)
	{
		float2 dv = float2(0.0, i * blurScale) * ar;
		color += textureColor1.Sample(samplerColor1, P + dv) * weights[i] * blurStrength;
	}

	return color;
}