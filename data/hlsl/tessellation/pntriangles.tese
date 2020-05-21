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

struct UBO
{
    float4x4 projection;
    float4x4 model;
    float tessAlpha;
};

cbuffer ubo : register(b1) { UBO ubo; }

struct HSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float3 Normal : NORMAL0;
[[vk::location(3)]] float2 UV : TEXCOORD0;
[[vk::location(6)]] float pnPatch[10] : TEXCOORD6;
};

struct ConstantsHSOutput
{
    float TessLevelOuter[3] : SV_TessFactor;
    float TessLevelInner : SV_InsideTessFactor;
};

struct DSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float3 Normal : NORMAL0;
[[vk::location(1)]] float2 UV : TEXCOORD0;
};

#define uvw TessCoord

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

[domain("tri")]
DSOutput main(ConstantsHSOutput input, float3 TessCoord : SV_DomainLocation, const OutputPatch<HSOutput, 3> patch)
{
    PnPatch pnPatch[3];
    pnPatch[0] = GetPnPatch(patch[0].pnPatch);
    pnPatch[1] = GetPnPatch(patch[1].pnPatch);
    pnPatch[2] = GetPnPatch(patch[2].pnPatch);

    DSOutput output = (DSOutput)0;
    float3 uvwSquared = uvw * uvw;
    float3 uvwCubed   = uvwSquared * uvw;

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
    output.UV  = TessCoord[2]*patch[0].UV + TessCoord[0]*patch[1].UV + TessCoord[1]*patch[2].UV;

    // normal
    // Barycentric normal
    float3 barNormal = TessCoord[2]*patch[0].Normal + TessCoord[0]*patch[1].Normal + TessCoord[1]*patch[2].Normal;
    float3 pnNormal  = patch[0].Normal*uvwSquared[2] + patch[1].Normal*uvwSquared[0] + patch[2].Normal*uvwSquared[1]
                   + n110*uvw[2]*uvw[0] + n011*uvw[0]*uvw[1]+ n101*uvw[2]*uvw[1];
    output.Normal = ubo.tessAlpha*pnNormal + (1.0-ubo.tessAlpha) * barNormal;

    // compute interpolated pos
    float3 barPos = TessCoord[2]*patch[0].Pos.xyz
                + TessCoord[0]*patch[1].Pos.xyz
                + TessCoord[1]*patch[2].Pos.xyz;

    // save some computations
    uvwSquared *= 3.0;

    // compute PN position
    float3 pnPos  = patch[0].Pos.xyz*uvwCubed[2]
                + patch[1].Pos.xyz*uvwCubed[0]
                + patch[2].Pos.xyz*uvwCubed[1]
                + b210*uvwSquared[2]*uvw[0]
                + b120*uvwSquared[0]*uvw[2]
                + b201*uvwSquared[2]*uvw[1]
                + b021*uvwSquared[0]*uvw[1]
                + b102*uvwSquared[1]*uvw[2]
                + b012*uvwSquared[1]*uvw[0]
                + b111*6.0*uvw[0]*uvw[1]*uvw[2];

    // final position and normal
    float3 finalPos = (1.0-ubo.tessAlpha)*barPos + ubo.tessAlpha*pnPos;
    output.Pos = mul(ubo.projection, mul(ubo.model, float4(finalPos,1.0)));
    return output;
}