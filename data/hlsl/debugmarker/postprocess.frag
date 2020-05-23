// Copyright 2020 Google LLC

Texture2D textureColor : register(t1);
SamplerState samplerColor : register(s1);

float4 main([[vk::location(0)]] float2 inUV : TEXCOORD0) : SV_TARGET
{
	// Single pass gauss blur

	const float2 texOffset = float2(0.01, 0.01);

	float2 tc0 = inUV + float2(-texOffset.x, -texOffset.y);
	float2 tc1 = inUV + float2(         0.0, -texOffset.y);
	float2 tc2 = inUV + float2(+texOffset.x, -texOffset.y);
	float2 tc3 = inUV + float2(-texOffset.x,          0.0);
	float2 tc4 = inUV + float2(         0.0,          0.0);
	float2 tc5 = inUV + float2(+texOffset.x,          0.0);
	float2 tc6 = inUV + float2(-texOffset.x, +texOffset.y);
	float2 tc7 = inUV + float2(         0.0, +texOffset.y);
	float2 tc8 = inUV + float2(+texOffset.x, +texOffset.y);

	float4 col0 = textureColor.Sample(samplerColor, tc0);
	float4 col1 = textureColor.Sample(samplerColor, tc1);
	float4 col2 = textureColor.Sample(samplerColor, tc2);
	float4 col3 = textureColor.Sample(samplerColor, tc3);
	float4 col4 = textureColor.Sample(samplerColor, tc4);
	float4 col5 = textureColor.Sample(samplerColor, tc5);
	float4 col6 = textureColor.Sample(samplerColor, tc6);
	float4 col7 = textureColor.Sample(samplerColor, tc7);
	float4 col8 = textureColor.Sample(samplerColor, tc8);

	float4 sum = (1.0 * col0 + 2.0 * col1 + 1.0 * col2 +
			  2.0 * col3 + 4.0 * col4 + 2.0 * col5 +
			  1.0 * col6 + 2.0 * col7 + 1.0 * col8) / 16.0;
	return float4(sum.rgb, 1.0);
}