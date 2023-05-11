// Copyright 2020 Google LLC

struct UBO
{
	float tessLevel;
};

cbuffer ubo : register(b0) { UBO ubo; }

struct VSOutput
{
[[vk::location(2)]]	float4 Pos : POSITION;
[[vk::location(0)]] float3 Normal : NORMAL0;
[[vk::location(1)]] float2 UV : TEXCOORD0;
};

struct HSOutput
{
[[vk::location(2)]]	float4 Pos : POSITION;
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
    output.TessLevelInner = ubo.tessLevel;
    output.TessLevelOuter[0] = ubo.tessLevel;
    output.TessLevelOuter[1] = ubo.tessLevel;
    output.TessLevelOuter[2] = ubo.tessLevel;
    return output;
}

[domain("tri")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
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
