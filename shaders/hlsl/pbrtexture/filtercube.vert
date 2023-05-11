// Copyright 2020 Google LLC

struct VSInput
{
[[vk::location(0)]] float3 Pos : POSITION0;
};

struct PushConsts {
[[vk::offset(0)]] float4x4 mvp;
};
[[vk::push_constant]] PushConsts pushConsts;

struct VSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float3 UVW : TEXCOORD0;
};

VSOutput main(VSInput input)
{
	VSOutput output = (VSOutput)0;
	output.UVW = input.Pos;
	output.Pos = mul(pushConsts.mvp, float4(input.Pos.xyz, 1.0));
	return output;
}
