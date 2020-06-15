// Copyright 2020 Google LLC

// PN patch data
struct PnPatch
{
	float b210;
	float b120;
	float b021;
	float b012;
	float b102;
	float b201;
	float b111;
	float n110;
	float n011;
	float n101;
};

// tessellation levels
struct UBO
{
	float tessLevel;
};

cbuffer ubo : register(b0) { UBO ubo; }

struct VSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float3 Normal : NORMAL0;
[[vk::location(1)]] float2 UV : TEXCOORD0;
};

struct HSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float3 Normal : NORMAL0;
[[vk::location(3)]] float2 UV : TEXCOORD0;
[[vk::location(6)]]	float pnPatch[10] : TEXCOORD6;
};

struct ConstantsHSOutput
{
    float TessLevelOuter[3] : SV_TessFactor;
    float TessLevelInner : SV_InsideTessFactor;
};

void SetPnPatch(out float output[10], PnPatch patch)
{
	output[0] = patch.b210;
	output[1] = patch.b120;
	output[2] = patch.b021;
	output[3] = patch.b012;
	output[4] = patch.b102;
	output[5] = patch.b201;
	output[6] = patch.b111;
	output[7] = patch.n110;
	output[8] = patch.n011;
	output[9] = patch.n101;
}

float wij(float4 iPos, float3 iNormal, float4 jPos)
{
	return dot(jPos.xyz - iPos.xyz, iNormal);
}

float vij(float4 iPos, float3 iNormal, float4 jPos, float3 jNormal)
{
	float3 Pj_minus_Pi = jPos.xyz
					- iPos.xyz;
	float3 Ni_plus_Nj  = iNormal+jNormal;
	return 2.0*dot(Pj_minus_Pi, Ni_plus_Nj)/dot(Pj_minus_Pi, Pj_minus_Pi);
}

ConstantsHSOutput ConstantsHS(InputPatch<VSOutput, 3> patch, uint InvocationID : SV_PrimitiveID)
{
    ConstantsHSOutput output = (ConstantsHSOutput)0;
	output.TessLevelOuter[0] = ubo.tessLevel;
	output.TessLevelOuter[1] = ubo.tessLevel;
	output.TessLevelOuter[2] = ubo.tessLevel;
	output.TessLevelInner = ubo.tessLevel;
    return output;
}

[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_ccw")]
[outputcontrolpoints(3)]
[patchconstantfunc("ConstantsHS")]
[maxtessfactor(20.0f)]
HSOutput main(InputPatch<VSOutput, 3> patch, uint InvocationID : SV_OutputControlPointID)
{
	HSOutput output = (HSOutput)0;
	// get data
	output.Pos = patch[InvocationID].Pos;
	output.Normal = patch[InvocationID].Normal;
	output.UV = patch[InvocationID].UV;

	// set base
	float P0 = patch[0].Pos[InvocationID];
	float P1 = patch[1].Pos[InvocationID];
	float P2 = patch[2].Pos[InvocationID];
	float N0 = patch[0].Normal[InvocationID];
	float N1 = patch[1].Normal[InvocationID];
	float N2 = patch[2].Normal[InvocationID];

	// compute control points
	PnPatch pnPatch;
	pnPatch.b210 = (2.0*P0 + P1 - wij(patch[0].Pos, patch[0].Normal, patch[1].Pos)*N0)/3.0;
	pnPatch.b120 = (2.0*P1 + P0 - wij(patch[1].Pos, patch[1].Normal, patch[0].Pos)*N1)/3.0;
	pnPatch.b021 = (2.0*P1 + P2 - wij(patch[1].Pos, patch[1].Normal, patch[2].Pos)*N1)/3.0;
	pnPatch.b012 = (2.0*P2 + P1 - wij(patch[2].Pos, patch[2].Normal, patch[1].Pos)*N2)/3.0;
	pnPatch.b102 = (2.0*P2 + P0 - wij(patch[2].Pos, patch[2].Normal, patch[0].Pos)*N2)/3.0;
	pnPatch.b201 = (2.0*P0 + P2 - wij(patch[0].Pos, patch[0].Normal, patch[2].Pos)*N0)/3.0;
	float E = ( pnPatch.b210
			+ pnPatch.b120
			+ pnPatch.b021
			+ pnPatch.b012
			+ pnPatch.b102
			+ pnPatch.b201 ) / 6.0;
	float V = (P0 + P1 + P2)/3.0;
	pnPatch.b111 = E + (E - V)*0.5;
	pnPatch.n110 = N0+N1-vij(patch[0].Pos, patch[0].Normal, patch[1].Pos, patch[1].Normal)*(P1-P0);
	pnPatch.n011 = N1+N2-vij(patch[1].Pos, patch[1].Normal, patch[2].Pos, patch[2].Normal)*(P2-P1);
	pnPatch.n101 = N2+N0-vij(patch[2].Pos, patch[2].Normal, patch[0].Pos, patch[0].Normal)*(P0-P2);
	SetPnPatch(output.pnPatch, pnPatch);

	return output;
}