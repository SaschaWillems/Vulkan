// Copyright 2020 Google LLC

#define LIGHT_COUNT 3

struct UBO
{
	float4x4 mvp[LIGHT_COUNT];
	float4 instancePos[3];
};

cbuffer ubo : register(b0) { UBO ubo; }

struct VSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] int InstanceIndex : TEXCOORD0;
};

struct GSOutput
{
	float4 Pos : SV_POSITION;
	int Layer : SV_RenderTargetArrayIndex;
};

[maxvertexcount(3)]
[instance(3)]
void main(triangle VSOutput input[3], uint InvocationID : SV_GSInstanceID, inout TriangleStream<GSOutput> outStream)
{
	float4 instancedPos = ubo.instancePos[input[0].InstanceIndex];
	for (int i = 0; i < 3; i++)
	{
		float4 tmpPos = input[i].Pos + instancedPos;
		GSOutput output = (GSOutput)0;
		output.Pos = mul(ubo.mvp[InvocationID], tmpPos);
		output.Layer = InvocationID;
		outStream.Append( output );
	}
	outStream.RestartStrip();
}