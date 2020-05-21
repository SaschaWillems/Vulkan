// Copyright 2020 Google LLC

struct UBO
{
	float4x4 projection;
	float4x4 model;
};

cbuffer ubo : register(b1) { UBO ubo; }

struct VSOutput
{
[[vk::location(0)]] float4 Pos : POSITION0;
[[vk::location(1)]] float3 Normal : NORMAL0;
};

struct GSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float3 Color : COLOR0;
};

[maxvertexcount(6)]
void main(triangle VSOutput input[3], inout LineStream<GSOutput> outStream)
{
	float normalLength = 0.02;
	for(int i=0; i<3; i++)
	{
		float3 pos = input[i].Pos.xyz;
		float3 normal = input[i].Normal.xyz;

		GSOutput output = (GSOutput)0;
		output.Pos = mul(ubo.projection, mul(ubo.model, float4(pos, 1.0)));
		output.Color = float3(1.0, 0.0, 0.0);
		outStream.Append( output );

		output.Pos = mul(ubo.projection, mul(ubo.model, float4(pos + normal * normalLength, 1.0)));
		output.Color = float3(0.0, 0.0, 1.0);
		outStream.Append( output );

		outStream.RestartStrip();
	}
}