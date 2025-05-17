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

ConstantsHSOutput ConstantsHS(InputPatch<VSOutput, 3> patch)
{
    ConstantsHSOutput output;
    output.TessLevelInner[0] = 1;
    output.TessLevelInner[1] = 1;
    output.TessLevelOuter[0] = 1;
    output.TessLevelOuter[1] = 1;
    output.TessLevelOuter[2] = 1;
    return output;
}

[shader("hull")]
[domain("tri")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("ConstantsHS")]
[maxtessfactor(20.0f)]
HSOutput hullMain(InputPatch<VSOutput, 3> patch, uint InvocationID : SV_OutputControlPointID)
{
    HSOutput output;
    output.Pos = patch[InvocationID].Pos;
	output.Normal = patch[InvocationID].Normal;
	output.UV = patch[InvocationID].UV;
    return output;
}

[shader("domain")]
[domain("tri")]
DSOutput domainMain(ConstantsHSOutput input, float3 TessCoord: SV_DomainLocation, const OutputPatch<HSOutput, 3> patch)
{
    DSOutput output = (DSOutput)0;
    output.Pos = (TessCoord.x * patch[0].Pos) +
                 (TessCoord.y * patch[1].Pos) +
                 (TessCoord.z * patch[2].Pos);
    output.Pos = mul(ubo.projection, mul(ubo.model, output.Pos));

    output.Normal = TessCoord.x * patch[0].Normal + TessCoord.y * patch[1].Normal + TessCoord.z * patch[2].Normal;
    output.UV = TessCoord.x * patch[0].UV + TessCoord.y * patch[1].UV + TessCoord.z * patch[2].UV;
    return output;
}