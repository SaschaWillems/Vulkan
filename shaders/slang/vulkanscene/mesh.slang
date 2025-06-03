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
    output.UV = input.TexCoord.xy;
    output.Normal = normalize(mul((float3x3)ubo.normal, input.Normal));
    output.Color = input.Color;
    float4x4 modelView = mul(ubo.view, ubo.model);
    float4 pos = mul(modelView, input.Pos);
    output.Pos = mul(ubo.projection, pos);
    output.EyePos = mul(modelView, pos).xyz;
    float4 lightPos = mul(modelView, float4(ubo.lightpos, 1.0));
    output.LightVec = normalize(lightPos.xyz - output.EyePos);
    return output;
}

float specpart(float3 L, float3 N, float3 H)
{
	if (dot(N, L) > 0.0)
	{
		return pow(clamp(dot(H, N), 0.0, 1.0), 64.0);
	}
	return 0.0;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
	float3 Eye = normalize(-input.EyePos);
	float3 Reflected = normalize(reflect(-input.LightVec, input.Normal));

	float3 halfVec = normalize(input.LightVec + input.EyePos);
	float diff = clamp(dot(input.LightVec, input.Normal), 0.0, 1.0);
	float spec = specpart(input.LightVec, input.Normal, halfVec);
	float intensity = 0.1 + diff + spec;

	float4 IAmbient = float4(0.2, 0.2, 0.2, 1.0);
	float4 IDiffuse = float4(0.5, 0.5, 0.5, 0.5) * max(dot(input.Normal, input.LightVec), 0.0);
	float shininess = 0.75;
	float4 ISpecular = float4(0.5, 0.5, 0.5, 1.0) * pow(max(dot(Reflected, Eye), 0.0), 2.0) * shininess;

	float4 outFragColor = float4((IAmbient + IDiffuse) * float4(input.Color, 1.0) + ISpecular);

	// Some manual saturation
	if (intensity > 0.95)
		outFragColor *= 2.25;
	if (intensity < 0.15)
		outFragColor = float4(0.1, 0.1, 0.1, 0.1);

	return outFragColor;
}