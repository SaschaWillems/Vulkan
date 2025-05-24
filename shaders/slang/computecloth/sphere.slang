/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSInput
{
	float3 Pos;
	float2 UV;
	float3 Normal;
};

struct VSOutput
{
	float4 Pos : SV_POSITION;
    float3 Normal;
    float3 ViewVec;
    float3 LightVec;
};

struct UBO
{
    float4x4 projection;
    float4x4 modelview;
    float4 lightPos;
};
ConstantBuffer<UBO> ubo;

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    float4 eyePos = mul(ubo.modelview, float4(input.Pos.x, input.Pos.y, input.Pos.z, 1.0));
    output.Pos = mul(ubo.projection, eyePos);
    float4 pos = float4(input.Pos, 1.0);
    float3 lPos = ubo.lightPos.xyz;
    output.LightVec = lPos - pos.xyz;
    output.ViewVec = -pos.xyz;
    output.Normal = input.Normal;
    return output;
} 

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
	float3 color = float3(0.5, 0.5, 0.5);
	float3 N = normalize(input.Normal);
	float3 L = normalize(input.LightVec);
	float3 V = normalize(input.ViewVec);
	float3 R = reflect(-L, N);
	float3 diffuse = max(dot(N, L), 0.15);
	float3 specular = pow(max(dot(R, V), 0.0), 32.0);
	return float4(diffuse * color.rgb + specular, 1.0);
}
