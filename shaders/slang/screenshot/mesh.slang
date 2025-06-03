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
	float3 Color;
	float3 Normal;
	float3 ViewVec;
	float3 LightVec;
 };

struct UBO
{
	float4x4 projection;
	float4x4 view;
	float4x4 model;
};
ConstantBuffer<UBO> ubo;

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.Normal = input.Normal;
    output.Color = input.Color;

    output.Pos = mul(ubo.projection, mul(ubo.view, mul(ubo.model, float4(input.Pos.xyz, 1.0))));

    output.Normal = mul((float3x3)ubo.model, input.Normal);
    float4 pos = mul(ubo.model, float4(input.Pos, 1.0));

	const float3 lightPos = float3(1.0, -1.0, 1.0);
    output.LightVec = lightPos.xyz - pos.xyz;
    output.ViewVec = -pos.xyz;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
    const float ambient = 0.1;
	float3 N = normalize(input.Normal);
    float3 L = normalize(input.LightVec);
    float3 V = normalize(input.ViewVec);
    float3 R = reflect(-L, N);
    float3 diffuse = max(dot(N, L), 0.0).rrr;
    float3 specular = pow(max(dot(R, V), 0.0), 32.0);
    return float4((ambient + diffuse) * input.Color.rgb + specular, 1.0);
}