/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSInput
{
    float3 Pos;
    float3 Normal;
    float2 UV;
};

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
    float3 EyePos;
    float3 LightVec;
};

struct UBO
{
	float4x4 projection;
	float4x4 model;
	float4 lightPos;
	float tessAlpha;
	float tessStrength;
	float tessLevel;
};
ConstantBuffer<UBO> ubo;

Sampler2D samplerColorAndDisplacementMap;

struct ConstantsHSOutput
{
    float TessLevelOuter[3] : SV_TessFactor;
    float TessLevelInner[2] : SV_InsideTessFactor;
};

ConstantsHSOutput ConstantsHS()
{
    ConstantsHSOutput output;
    output.TessLevelInner[0] = ubo.tessLevel;
    output.TessLevelInner[1] = ubo.tessLevel;
    output.TessLevelOuter[0] = ubo.tessLevel;
    output.TessLevelOuter[1] = ubo.tessLevel;
    output.TessLevelOuter[2] = ubo.tessLevel;
    return output;
}

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.Pos = float4(input.Pos.xyz, 1.0);
    output.UV = input.UV;
    output.Normal = input.Normal;
    return output;
}

[shader("hull")]
[domain("tri")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("ConstantsHS")]
[maxtessfactor(20.0f)]
HSOutput hullMain(InputPatch<VSOutput, 3> patch, uint InvocationID: SV_OutputControlPointID)
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
    DSOutput output;
    output.Pos = (TessCoord.x * patch[0].Pos) + (TessCoord.y * patch[1].Pos) + (TessCoord.z * patch[2].Pos);
    output.UV = (TessCoord.x * patch[0].UV) + (TessCoord.y * patch[1].UV) + (TessCoord.z * patch[2].UV);
    output.Normal = TessCoord.x * patch[0].Normal + TessCoord.y * patch[1].Normal + TessCoord.z * patch[2].Normal;
    output.Pos.xyz += normalize(output.Normal) * (max(samplerColorAndDisplacementMap.SampleLevel(output.UV.xy, 0).a, 0.0) * ubo.tessStrength);
    output.EyePos = output.Pos.xyz;
    output.LightVec = normalize(ubo.lightPos.xyz - output.EyePos);
    output.Pos = mul(ubo.projection, mul(ubo.model, output.Pos));
    return output;
}

[shader("fragment")]
float4 fragmentMain(DSOutput input)
{
    float3 N = normalize(input.Normal);
    float3 L = normalize(float3(1.0, 1.0, 1.0));
    float3 Eye = normalize(-input.EyePos);
    float3 Reflected = normalize(reflect(-input.LightVec, input.Normal));
    float4 IAmbient = float4(0.0, 0.0, 0.0, 1.0);
    float4 IDiffuse = float4(1.0, 1.0, 1.0, 1.0) * max(dot(input.Normal, input.LightVec), 0.0);
    return float4((IAmbient + IDiffuse) * float4(samplerColorAndDisplacementMap.Sample(input.UV).rgb, 1.0));
}
