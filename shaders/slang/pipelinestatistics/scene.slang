/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSInput
{
    float3 Pos;
    float3 Normal;
    float3 Color;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float3 Normal;
    float3 Color;
    float3 ViewVec;
    float3 LightVec;
};

struct HSOutput
{
    float4 Pos : SV_POSITION;
    float3 Normal;
    float3 Color;
    float3 ViewVec;
    float3 LightVec;
};

struct DSOutput
{
    float4 Pos : SV_POSITION;
    float3 Normal;
    float3 Color;
    float3 ViewVec;
    float3 LightVec;
};

struct ConstantsHSOutput
{
    float TessLevelOuter[3] : SV_TessFactor;
    float TessLevelInner[2] : SV_InsideTessFactor;
};

struct UBO
{
    float4x4 projection;
    float4x4 modelview;
    float4 lightPos;
};
ConstantBuffer<UBO> ubo;

struct PushConsts {
    float3 objPos;
};
[[vk::push_constant]] PushConsts pushConsts;

ConstantsHSOutput ConstantsHS(InputPatch<VSOutput, 3> patch)
{
    ConstantsHSOutput output;
    output.TessLevelInner[0] = 2.0;
    output.TessLevelInner[1] = 2.0;
    output.TessLevelOuter[0] = 1.0;
    output.TessLevelOuter[1] = 1.0;
    output.TessLevelOuter[2] = 1.0;
    return output;
}

[shader("hull")]
[domain("tri")]
[partitioning("integer")]
[outputtopology("triangle_ccw")]
[outputcontrolpoints(3)]
[patchconstantfunc("ConstantsHS")]
[maxtessfactor(20.0f)]
HSOutput hullMain(InputPatch<VSOutput, 3> patch, uint InvocationID: SV_OutputControlPointID)
{
    HSOutput output;
    output.Pos = patch[InvocationID].Pos;
    output.Normal = patch[InvocationID].Normal;
    output.Color = patch[InvocationID].Color;
    output.ViewVec = patch[InvocationID].ViewVec;
    output.LightVec = patch[InvocationID].LightVec;
    return output;
}

[shader("domain")]
[domain("tri")]
DSOutput domainMain(ConstantsHSOutput input, float3 TessCoord: SV_DomainLocation, const OutputPatch<HSOutput, 3> patch)
{
    DSOutput output;
    output.Pos = (TessCoord.x * patch[2].Pos) + (TessCoord.y * patch[1].Pos) + (TessCoord.z * patch[0].Pos);
    output.Normal = TessCoord.x * patch[2].Normal + TessCoord.y * patch[1].Normal + TessCoord.z * patch[0].Normal;
    output.ViewVec = TessCoord.x * patch[2].ViewVec + TessCoord.y * patch[1].ViewVec + TessCoord.z * patch[0].ViewVec;
    output.LightVec = TessCoord.x * patch[2].LightVec + TessCoord.y * patch[1].LightVec + TessCoord.z * patch[0].LightVec;
    output.Color = patch[0].Color;
    return output;
}

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.Color = input.Color;
    float3 locPos = mul(ubo.modelview, float4(input.Pos, 1.0)).xyz;
    float3 worldPos = mul(ubo.modelview, float4(input.Pos + pushConsts.objPos, 1.0)).xyz;
    output.Pos = mul(ubo.projection, float4(worldPos, 1.0));
    float4 pos = mul(ubo.modelview, float4(worldPos, 1.0));
    output.Normal = mul((float3x3)ubo.modelview, input.Normal);
    output.LightVec = ubo.lightPos.xyz - pos.xyz;
    output.ViewVec = -pos.xyz;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
	float3 N = normalize(input.Normal);
	float3 L = normalize(input.LightVec);
	float3 V = normalize(input.ViewVec);
	float3 R = reflect(-L, N);
	float3 diffuse = max(dot(N, L), 0.0) * input.Color;
	float3 specular = pow(max(dot(R, V), 0.0), 8.0) * float3(0.75, 0.75, 0.75);
	return float4(diffuse + specular, 0.5);
}