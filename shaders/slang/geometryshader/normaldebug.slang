/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSInput
{
    float3 Pos;
    float3 Normal;
};

struct VSOutput
{
	float4 Pos : POSITION0;
	float3 Normal;
};

struct GSOutput
{
	float4 Pos : SV_POSITION;
	float3 Color;
};

struct UBO
{
    float4x4 projection;
    float4x4 model;
};
ConstantBuffer<UBO> ubo;

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.Normal = input.Normal;
    output.Pos = float4(input.Pos.xyz, 1.0);
    return output;
}

[shader("geometry")]
[maxvertexcount(6)]
void geometryMain(triangle VSOutput input[3], inout LineStream<GSOutput> outStream)
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

[shader("fragment")]
float4 fragmentMain(GSOutput input)
{
    return float4(input.Color, 1.0);
}