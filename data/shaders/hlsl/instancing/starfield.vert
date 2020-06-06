// Copyright 2020 Google LLC

struct VSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float3 UVW : TEXCOORD0;
};

VSOutput main(uint VertexIndex : SV_VertexID)
{
	VSOutput output = (VSOutput)0;
	output.UVW = float3((VertexIndex << 1) & 2, VertexIndex & 2, VertexIndex & 2);
	output.Pos = float4(output.UVW.xy * 2.0f - 1.0f, 0.0f, 1.0f);
	return output;
}