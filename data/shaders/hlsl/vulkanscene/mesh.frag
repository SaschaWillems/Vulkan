// Copyright 2020 Google LLC

Texture2D tex : register(t1);
SamplerState samp : register(s1);

struct VSOutput
{
[[vk::location(0)]] float2 UV : TEXCOORD0;
[[vk::location(1)]] float3 Normal : NORMAL0;
[[vk::location(2)]] float3 Color : COLOR0;
[[vk::location(3)]] float3 EyePos : POSITION0;
[[vk::location(4)]] float3 LightVec : TEXCOORD2;
};

float specpart(float3 L, float3 N, float3 H)
{
	if (dot(N, L) > 0.0)
	{
		return pow(clamp(dot(H, N), 0.0, 1.0), 64.0);
	}
	return 0.0;
}

float4 main(VSOutput input) : SV_TARGET
{
	float3 Eye = normalize(-input.EyePos);
	float3 Reflected = normalize(reflect(-input.LightVec, input.Normal));

	float3 halfVec = normalize(input.LightVec + input.EyePos);
	float diff = clamp(dot(input.LightVec, input.Normal), 0.0, 1.0);
	float spec = specpart(input.LightVec, input.Normal, halfVec);
	float intensity = 0.1 + diff + spec;

	float4 IAmbient = float4(0.2, 0.2, 0.2, 1.0);
	float4 IDiffuse = float4(0.5, 0.5, 0.5, 0.5) * max(dot(input.Normal, input.LightVec), 0.0);
	float shininess = 0.75;
	float4 ISpecular = float4(0.5, 0.5, 0.5, 1.0) * pow(max(dot(Reflected, Eye), 0.0), 2.0) * shininess;

	float4 outFragColor = float4((IAmbient + IDiffuse) * float4(input.Color, 1.0) + ISpecular);

	// Some manual saturation
	if (intensity > 0.95)
		outFragColor *= 2.25;
	if (intensity < 0.15)
		outFragColor = float4(0.1, 0.1, 0.1, 0.1);

	return outFragColor;
}