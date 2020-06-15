// Copyright 2020 Google LLC

struct PushConsts
{
	float4x4 mvp;
};
[[vk::push_constant]]PushConsts pushConsts;

struct VSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float3 UVW : TEXCOORD0;
};

VSOutput main([[vk::location(0)]] float3 Pos : POSITION0)
{
	VSOutput output = (VSOutput)0;
	output.UVW = Pos;
	output.Pos = mul(pushConsts.mvp, float4(Pos.xyz, 1.0));
	return output;
}
