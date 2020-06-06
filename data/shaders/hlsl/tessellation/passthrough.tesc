// Copyright 2020 Google LLC

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
[[vk::location(1)]] float2 UV : TEXCOORD0;
};

struct ConstantsHSOutput
{
    float TessLevelOuter[3] : SV_TessFactor;
    float TessLevelInner : SV_InsideTessFactor;
};

ConstantsHSOutput ConstantsHS(InputPatch<VSOutput, 3> patch, uint InvocationID : SV_PrimitiveID)
{
    ConstantsHSOutput output = (ConstantsHSOutput)0;
    output.TessLevelInner = 1;
    output.TessLevelOuter[0] = 1;
    output.TessLevelOuter[1] = 1;
    output.TessLevelOuter[2] = 1;
    return output;
}

[domain("tri")]
[partitioning("integer")]
[outputtopology("triangle_ccw")]
[outputcontrolpoints(3)]
[patchconstantfunc("ConstantsHS")]
[maxtessfactor(20.0f)]
HSOutput main(InputPatch<VSOutput, 3> patch, uint InvocationID : SV_OutputControlPointID)
{
    HSOutput output = (HSOutput)0;
    output.Pos = patch[InvocationID].Pos;
	output.Normal = patch[InvocationID].Normal;
	output.UV = patch[InvocationID].UV;
    return output;
}