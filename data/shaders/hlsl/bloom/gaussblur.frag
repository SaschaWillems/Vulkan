// Copyright 2020 Google LLC

Texture2D textureColor : register(t1);
SamplerState samplerColor : register(s1);

cbuffer UBO : register(b0)
{
	float blurScale;
	float blurStrength;
};

[[vk::constant_id(0)]] const int blurdirection = 0;

float4 main([[vk::location(0)]] float2 inUV : TEXCOORD0) : SV_TARGET
{
	float weight[5];
	weight[0] = 0.227027;
	weight[1] = 0.1945946;
	weight[2] = 0.1216216;
	weight[3] = 0.054054;
	weight[4] = 0.016216;

	float2 textureSize;
	textureColor.GetDimensions(textureSize.x, textureSize.y);
	float2 tex_offset = 1.0 / textureSize * blurScale; // gets size of single texel
	float3 result = textureColor.Sample(samplerColor, inUV).rgb * weight[0]; // current fragment's contribution
	for(int i = 1; i < 5; ++i)
	{
		if (blurdirection == 1)
		{
			// H
			result += textureColor.Sample(samplerColor, inUV + float2(tex_offset.x * i, 0.0)).rgb * weight[i] * blurStrength;
			result += textureColor.Sample(samplerColor, inUV - float2(tex_offset.x * i, 0.0)).rgb * weight[i] * blurStrength;
		}
		else
		{
			// V
			result += textureColor.Sample(samplerColor, inUV + float2(0.0, tex_offset.y * i)).rgb * weight[i] * blurStrength;
			result += textureColor.Sample(samplerColor, inUV - float2(0.0, tex_offset.y * i)).rgb * weight[i] * blurStrength;
		}
	}
	return float4(result, 1.0);
}