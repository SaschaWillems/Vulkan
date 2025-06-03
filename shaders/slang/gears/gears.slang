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
  float3 EyePos;
  float3 LightVec;
};

struct UBO
{
  float4x4 projection;
  float4x4 view;
  float4 lightpos;
  float4x4 model[3];
};
ConstantBuffer<UBO> ubo;

[shader("vertex")]
VSOutput vertexMain(VSInput input, uint InstanceIndex: SV_StartInstanceLocation)
{
	VSOutput output;
	output.Normal = normalize(mul((float3x3)transpose(ubo.model[InstanceIndex]), input.Normal).xyz);
	output.Color = input.Color;
	float4x4 modelView = mul(ubo.view, ubo.model[InstanceIndex]);
	float4 pos = mul(modelView, float4(input.Pos, 1.0));
	output.EyePos = mul(modelView, pos).xyz;
	float4 lightPos = mul(float4(ubo.lightpos.xyz, 1.0), modelView);
	output.LightVec = normalize(lightPos.xyz - output.EyePos);
	output.Pos = mul(ubo.projection, pos);
	return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
  float3 eye = normalize(-input.EyePos);
  float3 reflected = normalize(reflect(-input.LightVec, input.Normal));

  float4 ambient = float4(0.2, 0.2, 0.2, 1.0);
  float4 diffuse = float4(0.5, 0.5, 0.5, 0.5) * max(dot(input.Normal, input.LightVec), 0.0);
  float4 specular = float4(0.5, 0.5, 0.5, 1.0) * pow(max(dot(reflected, eye), 0.0), 0.8) * 0.25;

  return float4((ambient + diffuse) * float4(input.Color, 1.0) + specular);
}