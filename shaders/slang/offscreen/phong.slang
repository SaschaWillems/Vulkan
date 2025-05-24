/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSInput
{
    float3 Pos;
    float3 Color;
    float3 Normal;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float ClipDistance : SV_ClipDistance0;
    float3 Normal;
    float3 Color;
    float3 EyePos;
    float3 LightVec;
};

struct UBO
{
    float4x4 projection;
    float4x4 view;
    float4x4 model;
    float4 lightPos;
};
ConstantBuffer<UBO> ubo;

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.Normal = input.Normal;
    output.Color = input.Color;
    output.Pos = mul(ubo.projection, mul(ubo.view, mul(ubo.model, float4(input.Pos, 1.0))));
    output.EyePos = mul(ubo.view, mul(ubo.model, float4(input.Pos, 1.0))).xyz;
    output.LightVec = normalize(ubo.lightPos.xyz - output.EyePos);
    // Clip against reflection plane
    float4 clipPlane = float4(0.0, 0.0, 0.0, 0.0);
    output.ClipDistance = dot(float4(input.Pos, 1.0), clipPlane);
    return output;
}


[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
	float3 Eye = normalize(-input.EyePos);
	float3 Reflected = normalize(reflect(-input.LightVec, input.Normal));
	float4 IAmbient = float4(0.1, 0.1, 0.1, 1.0);
	float4 IDiffuse = max(dot(input.Normal, input.LightVec), 0.0).xxxx;
	float specular = 0.75;
	float4 ISpecular = float4(0.0, 0.0, 0.0, 0.0);
	if (dot(input.EyePos, input.Normal) < 0.0)
	{
		ISpecular = float4(0.5, 0.5, 0.5, 1.0) * pow(max(dot(Reflected, Eye), 0.0), 16.0) * specular;
	}
	return float4((IAmbient + IDiffuse) * float4(input.Color, 1.0) + ISpecular);
}