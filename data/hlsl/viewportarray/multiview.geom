// Copyright 2020 Google LLC

struct UBO
{
	float4x4 projection[2];
	float4x4 modelview[2];
	float4 lightPos;
};

cbuffer ubo : register(b0) { UBO ubo; }

struct VSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float3 Normal : NORMAL0;
[[vk::location(1)]] float3 Color : COLOR0;
};

struct GSOutput
{
	float4 Pos : SV_POSITION;
	uint ViewportIndex : SV_ViewportArrayIndex;
	uint PrimitiveID : SV_PrimitiveID;
[[vk::location(0)]] float3 Normal : NORMAL0;
[[vk::location(1)]] float3 Color : COLOR0;
[[vk::location(2)]] float3 ViewVec : TEXCOOR1;
[[vk::location(3)]] float3 LightVec : TEXCOOR2;
};

[maxvertexcount(3)]
[instance(2)]
void main(triangle VSOutput input[3], inout TriangleStream<GSOutput> outStream, uint InvocationID : SV_GSInstanceID, uint PrimitiveID : SV_PrimitiveID)
{
	for(int i = 0; i < 3; i++)
	{
		GSOutput output = (GSOutput)0;
		output.Normal = mul((float3x3)ubo.modelview[InvocationID], input[i].Normal);
		output.Color = input[i].Color;

		float4 pos = input[i].Pos;
		float4 worldPos = mul(ubo.modelview[InvocationID], pos);

		float3 lPos = mul(ubo.modelview[InvocationID], ubo.lightPos).xyz;
		output.LightVec = lPos - worldPos.xyz;
		output.ViewVec = -worldPos.xyz;

		output.Pos = mul(ubo.projection[InvocationID], worldPos);

		// Set the viewport index that the vertex will be emitted to
		output.ViewportIndex = InvocationID;
      	output.PrimitiveID = PrimitiveID;
		outStream.Append( output );
	}

	outStream.RestartStrip();
}