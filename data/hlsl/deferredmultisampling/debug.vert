// Copyright 2020 Google LLC

struct UBO
{
	float4x4 projection;
	float4x4 model;
};

cbuffer ubo : register(b0) { UBO ubo; }

struct VSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float3 UV : TEXCOORD0;
};

VSOutput main(uint VertexIndex : SV_VertexID)
{
	VSOutput output = (VSOutput)0;
	output.UV = float3((VertexIndex << 1) & 2, VertexIndex & 2, 0.0);
	output.Pos = float4(output.UV.xy * 2.0f - 1.0f, 0.0f, 1.0f);
	return output;
}
