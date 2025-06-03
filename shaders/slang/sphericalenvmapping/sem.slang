/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSInput
{
    float4 Pos;
    float3 Normal;
    float3 Color;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float3 Color;
    float3 EyePos;
    float3 Normal;
    int TexIndex;
};

struct UBO
{
    float4x4 projection;
    float4x4 model;
    float4x4 normal;
    float4x4 view;
    int texIndex;
};
ConstantBuffer<UBO> ubo;

Sampler2DArray matCapSampler;

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.Color = input.Color;
    float4x4 modelView = mul(ubo.view, ubo.model);
    output.EyePos = normalize(mul(modelView, input.Pos).xyz);
    output.TexIndex = ubo.texIndex;
    output.Normal = normalize(mul((float3x3)ubo.normal, input.Normal));
    float3 r = reflect(output.EyePos, output.Normal);
    float m = 2.0 * sqrt(pow(r.x, 2.0) + pow(r.y, 2.0) + pow(r.z + 1.0, 2.0));
    output.Pos = mul(ubo.projection, mul(modelView, input.Pos));
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
	float3 r = reflect( input.EyePos, input.Normal );
	float3 r2 = float3( r.x, r.y, r.z + 1.0 );
	float m = 2.0 * length( r2 );
	float2 vN = r.xy / m + .5;
	return float4(matCapSampler.Sample(float3(vN, input.TexIndex)).rgb * (clamp(input.Color.r * 2, 0.0, 1.0)), 1.0);
}
