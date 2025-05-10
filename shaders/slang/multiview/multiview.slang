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

struct UBO
{
    float4x4 projection[2];
    float4x4 modelview[2];
    float4 lightPos;
};
ConstantBuffer<UBO> ubo;

[shader("vertex")]
VSOutput vertexMain(VSInput input, uint ViewIndex: SV_ViewID)
{
    VSOutput output;
    output.Color = input.Color;
    output.Normal = mul((float3x3)ubo.modelview[ViewIndex], input.Normal);

    float4 pos = float4(input.Pos.xyz, 1.0);
    float4 worldPos = mul(ubo.modelview[ViewIndex], pos);

    float3 lPos = mul(ubo.modelview[ViewIndex], ubo.lightPos).xyz;
    output.LightVec = lPos - worldPos.xyz;
    output.ViewVec = -worldPos.xyz;

    output.Pos = mul(ubo.projection[ViewIndex], worldPos);
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
	float3 N = normalize(input.Normal);
	float3 L = normalize(input.LightVec);
	float3 V = normalize(input.ViewVec);
	float3 R = reflect(-L, N);
	float3 ambient = float3(0.1, 0.1, 0.1);
	float3 diffuse = max(dot(N, L), 0.0) * float3(1.0, 1.0, 1.0);
	float3 specular = pow(max(dot(R, V), 0.0), 16.0) * float3(0.75, 0.75, 0.75);
	return float4((ambient + diffuse) * input.Color.rgb + specular, 1.0);
}