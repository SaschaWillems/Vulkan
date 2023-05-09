// Copyright 2020 Google LLC

Texture2D textureGradientRamp : register(t1);
SamplerState samplerGradientRamp : register(s1);

struct VSOutput
{
[[vk::location(0)]] float3 Normal : NORMAL0;
[[vk::location(1)]] float3 Color : COLOR0;
[[vk::location(2)]] float3 EyePos : POSITION0;
[[vk::location(3)]] float3 LightVec : TEXCOORD2;
[[vk::location(4)]] float2 UV : TEXCOORD0;
};

float4 main(VSOutput input) : SV_TARGET
{
	// No light calculations for glow color
	// Use max. color channel value
	// to detect bright glow emitters
	if ((input.Color.r >= 0.9) || (input.Color.g >= 0.9) || (input.Color.b >= 0.9))
	{
		return float4(textureGradientRamp.Sample(samplerGradientRamp, input.UV).rgb, 1);
	}
	else
	{
		float3 Eye = normalize(-input.EyePos);
		float3 Reflected = normalize(reflect(-input.LightVec, input.Normal));

		float4 IAmbient = float4(0.2, 0.2, 0.2, 1.0);
		float4 IDiffuse = float4(0.5, 0.5, 0.5, 0.5) * max(dot(input.Normal, input.LightVec), 0.0);
		float specular = 0.25;
		float4 ISpecular = float4(0.5, 0.5, 0.5, 1.0) * pow(max(dot(Reflected, Eye), 0.0), 4.0) * specular;
		return float4((IAmbient + IDiffuse) * float4(input.Color, 1.0) + ISpecular);
	}
}