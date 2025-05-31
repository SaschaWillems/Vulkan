/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSInput
{
	float4 Pos;
	float3 Normal;
	float2 TexCoord;
	float3 Color;
};

struct VSOutput
{
	float4 Pos : SV_POSITION;
	float2 UV;
	float3 Normal;
	float3 Color;
	float3 EyePos;
	float3 LightVec;
};

struct UBO
{
	float4x4 projection;
	float4x4 model;
	float4x4 normal;
	float4x4 view;
	float3 lightpos;
};
ConstantBuffer<UBO> ubo;

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
	VSOutput output;
	float4x4 modelView = mul(ubo.view, ubo.model);
	float4 pos = mul(modelView, input.Pos);
	output.UV = input.TexCoord.xy;
	output.Normal = normalize(mul((float3x3)ubo.normal, input.Normal));
	output.Color = input.Color;
	output.Pos = mul(ubo.projection, pos);
	output.EyePos = mul(modelView, pos).xyz;
	float4 lightPos = mul(modelView, float4(1.0, 2.0, 0.0, 1.0));
	output.LightVec = normalize(lightPos.xyz - output.EyePos);
	return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
	float3 Eye = normalize(-input.EyePos);
	float3 Reflected = normalize(reflect(-input.LightVec, input.Normal));
	float4 diff = float4(input.Color, 1.0) * max(dot(input.Normal, input.LightVec), 0.0);
	float shininess = 0.0;
	float4 spec = float4(1.0, 1.0, 1.0, 1.0) * pow(max(dot(Reflected, Eye), 0.0), 2.5) * shininess;
	return float4((diff + spec).rgb, 1);
}