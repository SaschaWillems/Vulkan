// Copyright 2020 Google LLC

float4 main(uint VertexIndex : SV_VertexID) : SV_POSITION
{
	return float4(float2((VertexIndex << 1) & 2, VertexIndex & 2) * 2.0f - 1.0f, 0.0f, 1.0f);
}