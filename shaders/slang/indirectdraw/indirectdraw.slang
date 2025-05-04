/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSInput
{
    float4 Pos;
    float3 Normal;
    float2 UV;
    float3 Color;
    float3 instancePos;
    float3 instanceRot;
    float instanceScale;
    int instanceTexIndex;
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
};
ConstantBuffer<UBO> ubo;

Sampler2DArray samplerArray;

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.Color = input.Color;
    output.UV = float3(input.UV, input.instanceTexIndex);

    float4x4 mx, my, mz;

    // rotate around x
    float s = sin(input.instanceRot.x);
    float c = cos(input.instanceRot.x);

    mx[0] = float4(c, s, 0.0, 0.0);
    mx[1] = float4(-s, c, 0.0, 0.0);
    mx[2] = float4(0.0, 0.0, 1.0, 0.0);
    mx[3] = float4(0.0, 0.0, 0.0, 1.0);

    // rotate around y
    s = sin(input.instanceRot.y);
    c = cos(input.instanceRot.y);

    my[0] = float4(c, 0.0, s, 0.0);
    my[1] = float4(0.0, 1.0, 0.0, 0.0);
    my[2] = float4(-s, 0.0, c, 0.0);
    my[3] = float4(0.0, 0.0, 0.0, 1.0);

    // rot around z
    s = sin(input.instanceRot.z);
    c = cos(input.instanceRot.z);

    mz[0] = float4(1.0, 0.0, 0.0, 0.0);
    mz[1] = float4(0.0, c, s, 0.0);
    mz[2] = float4(0.0, -s, c, 0.0);
    mz[3] = float4(0.0, 0.0, 0.0, 1.0);

    float4x4 rotMat = mul(mz, mul(my, mx));

    output.Normal = mul((float4x3)rotMat, input.Normal).xyz;

    float4 pos = mul(rotMat, float4((input.Pos.xyz * input.instanceScale) + input.instancePos, 1.0));

    output.Pos = mul(ubo.projection, mul(ubo.modelview, pos));

    float4 wPos = mul(ubo.modelview, float4(pos.xyz, 1.0));
    float4 lPos = float4(0.0, -5.0, 0.0, 1.0);
    output.LightVec = lPos.xyz - pos.xyz;
    output.ViewVec = -pos.xyz;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
    float4 color = samplerArray.Sample(input.UV);

	if (color.a < 0.5)
	{
		clip(-1);
	}

	float3 N = normalize(input.Normal);
	float3 L = normalize(input.LightVec);
	float3 ambient = float3(0.65, 0.65, 0.65);
	float3 diffuse = max(dot(N, L), 0.0) * input.Color;
	return float4((ambient + diffuse) * color.rgb, 1.0);
}
