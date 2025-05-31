/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSOutput
{
	float4 Pos : SV_POSITION;
	float3 Normal;
	float2 UV;
};

struct HSOutput
{
	float4 Pos : SV_POSITION;
	float3 Normal;
	float2 UV;
	float pnPatch[10];
};

struct DSOutput
{
    float4 Pos : SV_POSITION;
    float3 Normal;
    float2 UV;
};

struct ConstantsHSOutput
{
    float TessLevelOuter[3] : SV_TessFactor;
    float TessLevelInner[2] : SV_InsideTessFactor;
};

struct UBO
{
    float4x4 projection;
    float4x4 model;
    float tessAlpha;
    float tessLevel;
};
ConstantBuffer<UBO> ubo;

#define uvw TessCoord

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

PnPatch GetPnPatch(float pnPatch[10])
{
    PnPatch output;
    output.b210 = pnPatch[0];
    output.b120 = pnPatch[1];
    output.b021 = pnPatch[2];
    output.b012 = pnPatch[3];
    output.b102 = pnPatch[4];
    output.b201 = pnPatch[5];
    output.b111 = pnPatch[6];
    output.n110 = pnPatch[7];
    output.n011 = pnPatch[8];
    output.n101 = pnPatch[9];
    return output;
}

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
	float3 Pj_minus_Pi = jPos.xyz - iPos.xyz;
	float3 Ni_plus_Nj  = iNormal + jNormal;
	return 2.0 * dot(Pj_minus_Pi, Ni_plus_Nj) / dot(Pj_minus_Pi, Pj_minus_Pi);
}

ConstantsHSOutput ConstantsHS(InputPatch<VSOutput, 3> patch)
{
    ConstantsHSOutput output;
	output.TessLevelOuter[0] = ubo.tessLevel;
	output.TessLevelOuter[1] = ubo.tessLevel;
	output.TessLevelOuter[2] = ubo.tessLevel;
	output.TessLevelInner[0] = ubo.tessLevel;
	output.TessLevelInner[1] = ubo.tessLevel;
    return output;
}

[shader("domain")]
[domain("tri")]
DSOutput domainMain(ConstantsHSOutput input, float3 TessCoord: SV_DomainLocation, const OutputPatch<HSOutput, 3> patch)
{
    PnPatch pnPatch[3];
    pnPatch[0] = GetPnPatch(patch[0].pnPatch);
    pnPatch[1] = GetPnPatch(patch[1].pnPatch);
    pnPatch[2] = GetPnPatch(patch[2].pnPatch);

    DSOutput output = (DSOutput)0;
    float3 uvwSquared = uvw * uvw;
    float3 uvwCubed = uvwSquared * uvw;

    // extract control points
    float3 b210 = float3(pnPatch[0].b210, pnPatch[1].b210, pnPatch[2].b210);
    float3 b120 = float3(pnPatch[0].b120, pnPatch[1].b120, pnPatch[2].b120);
    float3 b021 = float3(pnPatch[0].b021, pnPatch[1].b021, pnPatch[2].b021);
    float3 b012 = float3(pnPatch[0].b012, pnPatch[1].b012, pnPatch[2].b012);
    float3 b102 = float3(pnPatch[0].b102, pnPatch[1].b102, pnPatch[2].b102);
    float3 b201 = float3(pnPatch[0].b201, pnPatch[1].b201, pnPatch[2].b201);
    float3 b111 = float3(pnPatch[0].b111, pnPatch[1].b111, pnPatch[2].b111);

    // extract control normals
    float3 n110 = normalize(float3(pnPatch[0].n110, pnPatch[1].n110, pnPatch[2].n110));
    float3 n011 = normalize(float3(pnPatch[0].n011, pnPatch[1].n011, pnPatch[2].n011));
    float3 n101 = normalize(float3(pnPatch[0].n101, pnPatch[1].n101, pnPatch[2].n101));

    // compute texcoords
    output.UV = TessCoord[2] * patch[0].UV + TessCoord[0] * patch[1].UV + TessCoord[1] * patch[2].UV;

    // normal
    // Barycentric normal
    float3 barNormal = TessCoord[2] * patch[0].Normal + TessCoord[0] * patch[1].Normal + TessCoord[1] * patch[2].Normal;
    float3 pnNormal = patch[0].Normal * uvwSquared[2] + patch[1].Normal * uvwSquared[0] + patch[2].Normal * uvwSquared[1]
                   + n110 * uvw[2] * uvw[0] + n011 * uvw[0] * uvw[1] + n101 * uvw[2] * uvw[1];
    output.Normal = ubo.tessAlpha * pnNormal + (1.0 - ubo.tessAlpha) * barNormal;

    // compute interpolated pos
    float3 barPos = TessCoord[2] * patch[0].Pos.xyz
                + TessCoord[0] * patch[1].Pos.xyz
                + TessCoord[1] * patch[2].Pos.xyz;

    // save some computations
    uvwSquared *= 3.0;

    // compute PN position
    float3 pnPos = patch[0].Pos.xyz * uvwCubed[2]
                + patch[1].Pos.xyz * uvwCubed[0]
                + patch[2].Pos.xyz * uvwCubed[1]
                + b210 * uvwSquared[2] * uvw[0]
                + b120 * uvwSquared[0] * uvw[2]
                + b201 * uvwSquared[2] * uvw[1]
                + b021 * uvwSquared[0] * uvw[1]
                + b102 * uvwSquared[1] * uvw[2]
                + b012 * uvwSquared[1] * uvw[0]
                + b111 * 6.0 * uvw[0] * uvw[1] * uvw[2];

    // final position and normal
    float3 finalPos = (1.0 - ubo.tessAlpha) * barPos + ubo.tessAlpha * pnPos;
    output.Pos = mul(ubo.projection, mul(ubo.model, float4(finalPos, 1.0)));
    return output;
}

[shader("hull")]
[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("ConstantsHS")]
[maxtessfactor(20.0f)]
HSOutput hullMain(InputPatch<VSOutput, 3> patch, uint InvocationID : SV_OutputControlPointID)
{
	HSOutput output;
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
	float E = ( pnPatch.b210 + pnPatch.b120 + pnPatch.b021 + pnPatch.b012 + pnPatch.b102 + pnPatch.b201 ) / 6.0;
	float V = (P0 + P1 + P2)/3.0;
	pnPatch.b111 = E + (E - V)*0.5;
	pnPatch.n110 = N0+N1-vij(patch[0].Pos, patch[0].Normal, patch[1].Pos, patch[1].Normal)*(P1-P0);
	pnPatch.n011 = N1+N2-vij(patch[1].Pos, patch[1].Normal, patch[2].Pos, patch[2].Normal)*(P2-P1);
	pnPatch.n101 = N2+N0-vij(patch[2].Pos, patch[2].Normal, patch[0].Pos, patch[0].Normal)*(P0-P2);
	SetPnPatch(output.pnPatch, pnPatch);

	return output;
}