// Copyright 2020 Google LLC

struct VSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] int InstanceIndex : TEXCOORD0;
};

VSOutput main([[vk::location(0)]] float4 Pos : POSITION0, uint InstanceIndex : SV_InstanceID)
{
	VSOutput output = (VSOutput)0;
	output.InstanceIndex = InstanceIndex;
	output.Pos = Pos;
	return output;
}