/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

#define LIGHT_COUNT 3

struct VSInput
{
    float4 Pos;
}

struct VSOutput
{
	float4 Pos : SV_POSITION;
	int InstanceIndex;
};

struct GSOutput
{
	float4 Pos : SV_POSITION;
	int Layer : SV_RenderTargetArrayIndex;
};

struct UBO
{
    float4x4 mvp[LIGHT_COUNT];
    float4 instancePos[3];
};
ConstantBuffer<UBO> ubo;

[shader("vertex")]
VSOutput vertexMain(VSInput input, uint InstanceIndex: SV_InstanceID)
{
    VSOutput output;
    output.InstanceIndex = InstanceIndex;
    output.Pos = input.Pos;
    return output;
}

[shader("geometry")]
[maxvertexcount(3)]
[instance(3)]
void geometryMain(triangle VSOutput input[3], uint InvocationID : SV_GSInstanceID, inout TriangleStream<GSOutput> outStream)
{
	float4 instancedPos = ubo.instancePos[input[0].InstanceIndex];
	for (int i = 0; i < 3; i++)
	{
		float4 tmpPos = input[i].Pos + instancedPos;
		GSOutput output;
		output.Pos = mul(ubo.mvp[InvocationID], tmpPos);
		output.Layer = InvocationID;
		outStream.Append( output );
	}
	outStream.RestartStrip();
}