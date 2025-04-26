/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSInput
{
   float3 Pos : POSITION0;
   float3 Normal : NORMAL0;
   float2 UV : TEXCOORD0;
   float3 Color : COLOR0;
   // Instanced attributes
   float3 instancePos : POSITION1;
   float3 instanceRot : TEXCOORD1;
   float instanceScale : TEXCOORD2;
   int instanceTexIndex : TEXCOORD3;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;	
	float3 Normal;
	float3 Color;
	float3 UV;
	float3 ViewVec;
	float3 LightVec;
};

struct UBO
{
    float4x4 projection;
    float4x4 modelview;
    float4 lightPos;
    float locSpeed;
    float globSpeed;
};
ConstantBuffer<UBO> ubo;
Sampler2DArray samplerArray;

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.Color = input.Color;
    output.UV = float3(input.UV, input.instanceTexIndex);

    // rotate around x
    float s = sin(input.instanceRot.x + ubo.locSpeed);
    float c = cos(input.instanceRot.x + ubo.locSpeed);

    float3x3 mx = { c, -s, 0.0,
                    s, c, 0.0,
                    0.0, 0.0, 1.0 };

    // rotate around y
    s = sin(input.instanceRot.y + ubo.locSpeed);
    c = cos(input.instanceRot.y + ubo.locSpeed);

    float3x3 my = { c, 0.0, -s,
                    0.0, 1.0, 0.0,
                    s, 0.0, c };

    // rot around z
    s = sin(input.instanceRot.z + ubo.locSpeed);
    c = cos(input.instanceRot.z + ubo.locSpeed);

    float3x3 mz = { 1.0, 0.0, 0.0,
                    0.0, c, -s,
                    0.0, s, c };

    float3x3 rotMat = mul(mz, mul(my, mx));

    float4x4 gRotMat;
    s = sin(input.instanceRot.y + ubo.globSpeed);
    c = cos(input.instanceRot.y + ubo.globSpeed);
    gRotMat[0] = float4(c, 0.0, -s, 0.0);
    gRotMat[1] = float4(0.0, 1.0, 0.0, 0.0);
    gRotMat[2] = float4(s, 0.0, c, 0.0);
    gRotMat[3] = float4(0.0, 0.0, 0.0, 1.0);

    float4 locPos = float4(mul(rotMat, input.Pos.xyz), 1.0);
    float4 pos = float4((locPos.xyz * input.instanceScale) + input.instancePos, 1.0);

    output.Pos = mul(ubo.projection, mul(ubo.modelview, mul(gRotMat, pos)));
    output.Normal = mul((float3x3)mul(ubo.modelview, gRotMat), mul(rotMat, input.Normal));

    pos = mul(ubo.modelview, float4(input.Pos.xyz + input.instancePos, 1.0));
    float3 lPos = mul((float3x3)ubo.modelview, ubo.lightPos.xyz);
    output.LightVec = lPos - pos.xyz;
    output.ViewVec = -pos.xyz;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
    float4 color = samplerArray.Sample(input.UV) * float4(input.Color, 1.0);
	float3 N = normalize(input.Normal);
	float3 L = normalize(input.LightVec);
	float3 V = normalize(input.ViewVec);
	float3 R = reflect(-L, N);
	float3 diffuse = max(dot(N, L), 0.1) * input.Color;
	float3 specular = (dot(N,L) > 0.0) ? pow(max(dot(R, V), 0.0), 16.0) * float3(0.75, 0.75, 0.75) * color.r : float3(0.0, 0.0, 0.0);
	return float4(diffuse * color.rgb + specular, 1.0);
}