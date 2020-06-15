// Copyright 2020 Google LLC

struct VSInput
{
[[vk::location(0)]] float3 Pos : POSITION0;
[[vk::location(1)]] float2 UV : TEXCOORD0;
};

struct UBO
{
	float4x4 projection;
	float4x4 modelview;
};

cbuffer ubo : register(b0) { UBO ubo; }

struct VSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float3 UV : TEXCOORD0;
};

VSOutput main(VSInput input, uint InstanceIndex : SV_InstanceID)
{
	VSOutput output = (VSOutput)0;
	output.UV = float3(input.UV.xy, InstanceIndex);
	float4 tmpPos = float4(input.Pos, 1.0);
	tmpPos.y += InstanceIndex;
	tmpPos.xy *= float2(1.0/4.0, 1.0/3.0);
	output.Pos = mul(ubo.projection, mul(ubo.modelview, tmpPos));
	return output;
}
