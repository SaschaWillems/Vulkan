// Copyright 2020 Google LLC

Texture2DArray matCapTexture : register(t1);
SamplerState matCapSampler : register(s1);

struct VSOutput
{
[[vk::location(0)]] float3 Color : COLOR0;
[[vk::location(1)]] float3 EyePos : POSITION0;
[[vk::location(2)]] float3 Normal : NORMAL0;
[[vk::location(3)]] int TexIndex : TEXCOORD1;
};

float4 main(VSOutput input) : SV_TARGET
{
	float3 r = reflect( input.EyePos, input.Normal );
	float3 r2 = float3( r.x, r.y, r.z + 1.0 );
	float m = 2.0 * length( r2 );
	float2 vN = r.xy / m + .5;
	return float4( matCapTexture.Sample( matCapSampler, float3(vN, input.TexIndex)).rgb * (clamp(input.Color.r * 2, 0.0, 1.0)), 1.0 );
}
